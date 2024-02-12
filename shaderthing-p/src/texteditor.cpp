#include <algorithm>
#include <chrono>
#include <string>
#include <regex>
#include <cmath>
#include <iostream>

#include "shaderthing-p/include/texteditor.h"

#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

template<class InputIt1, class InputIt2, class BinaryPredicate>
bool equals(InputIt1 first1, InputIt1 last1,
    InputIt2 first2, InputIt2 last2, BinaryPredicate p)
{
    for (; first1 != last1 && first2 != last2; ++first1, ++first2)
    {
        if (!p(*first1, *first2))
            return false;
    }
    return first1 == last1 && first2 == last2;
}

namespace ShaderThing
{

TextEditor::TextEditor()
    : lineSpacing_(1.0f)
    , undoIndex_(0)
    , tabSize_(4)
    , overwrite_(false)
    , readOnly_(false)
    , withinRender_(false)
    , scrollToCursor_(false)
    , scrollToTop_(false)
    , textChanged_(false)
    , colorizerEnabled_(true)
    , useSetTextStart_(false)
    , textStart_(20.0f)
    , leftMargin_(10)
    , cursorPositionChanged_(false)
    , colorRangeMin_(0)
    , colorRangeMax_(0)
    , selectionMode_(SelectionMode::Normal)
    , checkComments_(true)
    , lastClick_(-1.0f)
    , handleKeyboardInputs_(true)
    , handleMouseInputs_(true)
    , ignoreImGuiChild_(false)
    , showWhitespaces_(true)
    , startTime_
    (
        std::chrono::duration_cast<std::chrono::milliseconds>
        (
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    )
{
    setPalette(getDarkPalette());
    setLanguageDefinition(LanguageDefinition::GLSL());
    lines_.push_back(Line());
}

TextEditor::~TextEditor()
{
}

void TextEditor::setLanguageDefinition(const LanguageDefinition & aLanguageDef)
{
    languageDefinition_ = aLanguageDef;
    regexList_.clear();
    for (auto& r : languageDefinition_.tokenRegexStrings)
        regexList_.push_back
        (
            std::make_pair
            (
                std::regex(r.first, std::regex_constants::optimize), 
                r.second
            )
        );
    colorize();
}

void TextEditor::setPalette(const Palette & aValue)
{
    paletteBase_ = aValue;
}

std::string TextEditor::getText
(
    const Coordinates & aStart, 
    const Coordinates & aEnd
) const
{
    std::string result;

    auto lstart = aStart.line;
    auto lend = aEnd.line;
    auto istart = getCharacterIndex(aStart);
    auto iend = getCharacterIndex(aEnd);
    size_t s = 0;

    for (size_t i = lstart; i < lend; i++)
        s += lines_[i].size();

    result.reserve(s + s / 8);

    while (istart < iend || lstart < lend)
    {
        if (lstart >= (int)lines_.size())
            break;

        auto& line = lines_[lstart];
        if (istart < (int)line.size())
        {
            result += line[istart].character;
            istart++;
        }
        else
        {
            istart = 0;
            ++lstart;
            result += '\n';
        }
    }

    if (!result.empty() && result[result.length()-1] == '\n')
        result.erase(result.length()-1);

    return result;
}

TextEditor::Coordinates TextEditor::getActualCursorCoordinates() const
{
    return sanitizeCoordinates(state_.cursorPosition);
}

TextEditor::Coordinates TextEditor::sanitizeCoordinates
(
    const Coordinates & aValue
) const
{
    auto line = aValue.line;
    auto column = aValue.column;
    if (line >= (int)lines_.size())
    {
        if (lines_.empty())
        {
            line = 0;
            column = 0;
        }
        else
        {
            line = (int)lines_.size() - 1;
            column = getLineMaxColumn(line);
        }
        return Coordinates(line, column);
    }
    else
    {
        column = lines_.empty() ? 0 : std::min(column, getLineMaxColumn(line));
        return Coordinates(line, column);
    }
}

// https://en.wikipedia.org/wiki/UTF-8
// We assume that the char is a standalone character (<128) or a leading byte of
// an UTF-8 code sequence (non-10xxxxxx code)
static int UTF8CharLength(TextEditor::Char c)
{
    if ((c & 0xFE) == 0xFC)
        return 6;
    if ((c & 0xFC) == 0xF8)
        return 5;
    if ((c & 0xF8) == 0xF0)
        return 4;
    if ((c & 0xF0) == 0xE0)
        return 3;
    if ((c & 0xE0) == 0xC0)
        return 2;
    return 1;
}

// "Borrowed" from ImGui source
static inline int ImTextCharToUtf8(char* buf, int buf_size, unsigned int c)
{
    if (c < 0x80)
    {
        buf[0] = (char)c;
        return 1;
    }
    if (c < 0x800)
    {
        if (buf_size < 2) return 0;
        buf[0] = (char)(0xc0 + (c >> 6));
        buf[1] = (char)(0x80 + (c & 0x3f));
        return 2;
    }
    if (c >= 0xdc00 && c < 0xe000)
        return 0;
    if (c >= 0xd800 && c < 0xdc00)
    {
        if (buf_size < 4) return 0;
        buf[0] = (char)(0xf0 + (c >> 18));
        buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
        buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[3] = (char)(0x80 + ((c) & 0x3f));
        return 4;
    }
    if (buf_size < 3) 
        return 0;
    buf[0] = (char)(0xe0 + (c >> 12));
    buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
    buf[2] = (char)(0x80 + ((c) & 0x3f));
    return 3;
}

void TextEditor::advance(Coordinates & aCoordinates) const
{
    if (aCoordinates.line < (int)lines_.size())
    {
        auto& line = lines_[aCoordinates.line];
        auto cindex = getCharacterIndex(aCoordinates);
        if (cindex + 1 < (int)line.size())
        {
            auto delta = UTF8CharLength(line[cindex].character);
            cindex = std::min(cindex + delta, (int)line.size() - 1);
        }
        else
        {
            ++aCoordinates.line;
            cindex = 0;
        }
        aCoordinates.column = getCharacterColumn(aCoordinates.line, cindex);
    }
}

void TextEditor::deleteRange
(
    const Coordinates & aStart, 
    const Coordinates & aEnd
)
{
    assert(aEnd >= aStart);
    assert(!readOnly_);

    if (aEnd == aStart)
        return;

    auto start = getCharacterIndex(aStart);
    auto end = getCharacterIndex(aEnd);

    if (aStart.line == aEnd.line)
    {
        auto& line = lines_[aStart.line];
        auto n = getLineMaxColumn(aStart.line);
        if (aEnd.column >= n)
            line.erase(line.begin() + start, line.end());
        else
            line.erase(line.begin() + start, line.begin() + end);
    }
    else
    {
        auto& firstLine = lines_[aStart.line];
        auto& lastLine = lines_[aEnd.line];

        firstLine.erase(firstLine.begin() + start, firstLine.end());
        lastLine.erase(lastLine.begin(), lastLine.begin() + end);

        if (aStart.line < aEnd.line)
            firstLine.insert(firstLine.end(), lastLine.begin(), lastLine.end());

        if (aStart.line < aEnd.line)
            removeLine(aStart.line + 1, aEnd.line + 1);
    }

    textChanged_ = true;
}

int TextEditor::insertTextAt
(
    Coordinates& aWhere, 
    const char* aValue, 
    bool aRedo
)
{
    assert(!readOnly_);

    int cindex = getCharacterIndex(aWhere);
    int totalLines = 0;
    while (*aValue != '\0')
    {
        assert(!lines_.empty());
        if (*aValue == '\r')
            ++aValue;
        else if (*aValue == '\n')
        {
            if (aRedo)
            {
                auto cursor0 = getCursorPosition();
                setCursorPosition(aWhere);
                enterCharacter('\n', false, false);
                setCursorPosition(cursor0);
            }
            else
            {
                if (cindex < (int)lines_[aWhere.line].size())
                {
                    auto& newLine = insertLine(aWhere.line + 1);
                    auto& line = lines_[aWhere.line];
                    newLine.insert
                    (
                        newLine.begin(), 
                        line.begin() + cindex, 
                        line.end()
                    );
                    line.erase(line.begin() + cindex, line.end());
                }
                else
                    insertLine(aWhere.line + 1);
            }        
               cindex = 0;
            ++aWhere.line;
            aWhere.column = 0;
            ++totalLines;
            ++aValue;
        }
        else
        {
            auto& line = lines_[aWhere.line];
            auto d = UTF8CharLength(*aValue);
            while (d-- > 0 && *aValue != '\0')
                line.insert
                (
                    line.begin() + cindex++, 
                    Glyph(*aValue++, PaletteIndex::Default)
                );
            ++aWhere.column;
        }
        textChanged_ = true;
    }
    return totalLines;
}

void TextEditor::addUndo(UndoRecord& aValue)
{
    assert(!readOnly_);
    undoBuffer_.resize((size_t)(undoIndex_ + 1));
    undoBuffer_.back() = aValue;
    ++undoIndex_;
}

TextEditor::Coordinates TextEditor::screenPosToCoordinates
(
    const ImVec2& aPosition
) const
{
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 local(aPosition.x - origin.x, aPosition.y - origin.y);
    int lineNo = std::max(0, (int)floor(local.y / charAdvance_.y));
    int columnCoord = 0;
    if (lineNo >= 0 && lineNo < (int)lines_.size())
    {
        auto& line = lines_.at(lineNo);
        int columnIndex = 0;
        float columnX = 0.0f;
        while ((size_t)columnIndex < line.size())
        {
            float columnWidth = 0.0f;
            if (line[columnIndex].character == '\t')
            {
                float spaceSize = 
                    ImGui::GetFont()->CalcTextSizeA
                    (
                        ImGui::GetFontSize(), 
                        FLT_MAX, 
                        -1.0f, 
                        " "
                    ).x;
                float oldX = columnX;
                float newColumnX = 
                (
                    1.0f + 
                    std::floor((1.0f + columnX)/(float(tabSize_)*spaceSize))
                )*(float(tabSize_)*spaceSize);
                columnWidth = newColumnX - oldX;
                if (textStart_ + columnX + columnWidth * 0.5f > local.x)
                    break;
                columnX = newColumnX;
                columnCoord = (columnCoord / tabSize_) * tabSize_ + tabSize_;
                columnIndex++;
            }
            else
            {
                char buf[7];
                auto d = UTF8CharLength(line[columnIndex].character);
                int i = 0;
                while (i < 6 && d-- > 0)
                    buf[i++] = line[columnIndex++].character;
                buf[i] = '\0';
                columnWidth = 
                    ImGui::GetFont()->CalcTextSizeA
                    (
                        ImGui::GetFontSize(), 
                        FLT_MAX, 
                        -1.0f, 
                        buf
                    ).x;
                if (textStart_ + columnX + columnWidth * 0.5f > local.x)
                    break;
                columnX += columnWidth;
                columnCoord++;
            }
        }
    }

    return sanitizeCoordinates(Coordinates(lineNo, columnCoord));
}

TextEditor::Coordinates TextEditor::findWordStartPos
(
    const Coordinates & aFrom
) const
{
    Coordinates at = aFrom;
    if (at.line >= (int)lines_.size())
        return at;

    auto& line = lines_[at.line];
    auto cindex = getCharacterIndex(at);

    if (cindex >= (int)line.size())
        return at;

    while (cindex > 0 && isspace(line[cindex].character))
        --cindex;

    auto cstart = (PaletteIndex)line[cindex].colorIndex;
    while (cindex > 0)
    {
        auto c = line[cindex].character;
        if ((c & 0xC0) != 0x80)	// not UTF code sequence 10xxxxxx
        {
            if (c <= 32 && isspace(c))
            {
                cindex++;
                break;
            }
            if (cstart != (PaletteIndex)line[size_t(cindex - 1)].colorIndex)
                break;
        }
        --cindex;
    }
    return Coordinates(at.line, getCharacterColumn(at.line, cindex));
}

TextEditor::Coordinates TextEditor::findWordEndPos
(
    const Coordinates & aFrom
) const
{
    Coordinates at = aFrom;
    if (at.line >= (int)lines_.size())
        return at;

    auto& line = lines_[at.line];
    auto cindex = getCharacterIndex(at);

    if (cindex >= (int)line.size())
        return at;

    bool prevspace = (bool)isspace(line[cindex].character);
    auto cstart = (PaletteIndex)line[cindex].colorIndex;
    while (cindex < (int)line.size())
    {
        auto c = line[cindex].character;
        auto d = UTF8CharLength(c);
        if (cstart != (PaletteIndex)line[cindex].colorIndex)
            break;

        if (prevspace != !!isspace(c))
        {
            if (isspace(c))
                while (cindex < (int)line.size() && isspace(line[cindex].character))
                    ++cindex;
            break;
        }
        cindex += d;
    }
    return Coordinates(aFrom.line, getCharacterColumn(aFrom.line, cindex));
}

TextEditor::Coordinates TextEditor::findNextWordPos
(
    const Coordinates & aFrom
) const
{
    Coordinates at = aFrom;
    if (at.line >= (int)lines_.size())
        return at;

    // skip to the next non-word character
    auto cindex = getCharacterIndex(aFrom);
    bool isword = false;
    bool skip = false;
    if (cindex < (int)lines_[at.line].size())
    {
        auto& line = lines_[at.line];
        isword = isalnum(line[cindex].character);
        skip = isword;
    }

    while (!isword || skip)
    {
        if (at.line >= lines_.size())
        {
            auto l = std::max(0, (int) lines_.size() - 1);
            return Coordinates(l, getLineMaxColumn(l));
        }

        auto& line = lines_[at.line];
        if (cindex < (int)line.size())
        {
            isword = isalnum(line[cindex].character);

            if (isword && !skip)
                return 
                    Coordinates(at.line, getCharacterColumn(at.line, cindex));

            if (!isword)
                skip = false;

            cindex++;
        }
        else
        {
            cindex = 0;
            ++at.line;
            skip = false;
            isword = false;
        }
    }

    return at;
}

int TextEditor::getCharacterIndex(const Coordinates& aCoordinates) const
{
    if (aCoordinates.line >= lines_.size())
        return -1;
    auto& line = lines_[aCoordinates.line];
    int c = 0;
    int i = 0;
    for (; i < line.size() && c < aCoordinates.column;)
    {
        if (line[i].character == '\t')
            c = (c / tabSize_) * tabSize_ + tabSize_;
        else
            ++c;
        i += UTF8CharLength(line[i].character);
    }
    return i;
}

int TextEditor::getCharacterColumn(int aLine, int aIndex) const
{
    if (aLine >= lines_.size())
        return 0;
    auto& line = lines_[aLine];
    int col = 0;
    int i = 0;
    while (i < aIndex && i < (int)line.size())
    {
        auto c = line[i].character;
        i += UTF8CharLength(c);
        if (c == '\t')
            col = (col / tabSize_) * tabSize_ + tabSize_;
        else
            col++;
    }
    return col;
}

int TextEditor::getLineCharacterCount(int aLine) const
{
    if (aLine >= lines_.size())
        return 0;
    auto& line = lines_[aLine];
    int c = 0;
    for (unsigned i = 0; i < line.size(); c++)
        i += UTF8CharLength(line[i].character);
    return c;
}

int TextEditor::getLineMaxColumn(int aLine) const
{
    if (aLine >= lines_.size())
        return 0;
    auto& line = lines_[aLine];
    int col = 0;
    for (unsigned i = 0; i < line.size(); )
    {
        auto c = line[i].character;
        if (c == '\t')
            col = (col / tabSize_) * tabSize_ + tabSize_;
        else
            col++;
        i += UTF8CharLength(c);
    }
    return col;
}

bool TextEditor::isOnWordBoundary(const Coordinates & aAt) const
{
    if (aAt.line >= (int)lines_.size() || aAt.column == 0)
        return true;

    auto& line = lines_[aAt.line];
    auto cindex = getCharacterIndex(aAt);
    if (cindex >= (int)line.size())
        return true;

    if (colorizerEnabled_)
        return line[cindex].colorIndex != line[size_t(cindex - 1)].colorIndex;

    return isspace(line[cindex].character) != isspace(line[cindex - 1].character);
}

void TextEditor::removeLine(int aStart, int aEnd)
{
    assert(!readOnly_);
    assert(aEnd >= aStart);
    assert(lines_.size() > (size_t)(aEnd - aStart));

    ErrorMarkers etmp;
    for (auto& i : errorMarkers_)
    {
        ErrorMarkers::value_type e
        (
            i.first >= aStart ? i.first - 1 : i.first, i.second
        );
        if (e.first >= aStart && e.first <= aEnd)
            continue;
        etmp.insert(e);
    }
    errorMarkers_ = std::move(etmp);

    Breakpoints btmp;
    for (auto i : breakpoints_)
    {
        if (i >= aStart && i <= aEnd)
            continue;
        btmp.insert(i >= aStart ? i - 1 : i);
    }
    breakpoints_ = std::move(btmp);

    lines_.erase(lines_.begin() + aStart, lines_.begin() + aEnd);
    assert(!lines_.empty());

    textChanged_ = true;
}

void TextEditor::removeLine(int aIndex)
{
    assert(!readOnly_);
    assert(lines_.size() > 1);

    ErrorMarkers etmp;
    for (auto& i : errorMarkers_)
    {
        ErrorMarkers::value_type e
        (
            i.first > aIndex ? i.first - 1 : i.first, i.second
        );
        if (e.first - 1 == aIndex)
            continue;
        etmp.insert(e);
    }
    errorMarkers_ = std::move(etmp);

    Breakpoints btmp;
    for (auto i : breakpoints_)
    {
        if (i == aIndex)
            continue;
        btmp.insert(i >= aIndex ? i - 1 : i);
    }
    breakpoints_ = std::move(btmp);

    lines_.erase(lines_.begin() + aIndex);
    assert(!lines_.empty());

    textChanged_ = true;
}

TextEditor::Line& TextEditor::insertLine(int aIndex)
{
    assert(!readOnly_);

    auto& result = *lines_.insert(lines_.begin() + aIndex, Line());

    ErrorMarkers etmp;
    for (auto& i : errorMarkers_)
        etmp.insert
        (
            ErrorMarkers::value_type
            (
                i.first >= aIndex ? i.first + 1 : i.first, i.second
            )
        );
    errorMarkers_ = std::move(etmp);

    Breakpoints btmp;
    for (auto i : breakpoints_)
        btmp.insert(i >= aIndex ? i + 1 : i);
    breakpoints_ = std::move(btmp);

    return result;
}

std::string TextEditor::getWordUnderCursor() const
{
    auto c = getCursorPosition();
    return getWordAt(c);
}

std::string TextEditor::getWordAt(const Coordinates & aCoords) const
{
    auto start = findWordStartPos(aCoords);
    auto end = findWordEndPos(aCoords);

    std::string r;

    auto istart = getCharacterIndex(start);
    auto iend = getCharacterIndex(end);

    for (auto it = istart; it < iend; ++it)
        r.push_back(lines_[aCoords.line][it].character);

    return r;
}

ImU32 TextEditor::getGlyphColor(const Glyph & aGlyph) const
{
    if (!colorizerEnabled_)
        return palette_[(int)PaletteIndex::Default];
    if (aGlyph.comment)
        return palette_[(int)PaletteIndex::Comment];
    if (aGlyph.multiLineComment)
        return palette_[(int)PaletteIndex::MultiLineComment];
    auto const color = palette_[(int)aGlyph.colorIndex];
    if (aGlyph.preprocessor)
    {
        const auto ppcolor = palette_[(int)PaletteIndex::Preprocessor];
        const int c0 = ((ppcolor & 0xff) + (color & 0xff)) / 2;
        const int c1 = (((ppcolor >> 8) & 0xff) + ((color >> 8) & 0xff)) / 2;
        const int c2 = (((ppcolor >> 16) & 0xff) + ((color >> 16) & 0xff)) / 2;
        const int c3 = (((ppcolor >> 24) & 0xff) + ((color >> 24) & 0xff)) / 2;
        return ImU32(c0 | (c1 << 8) | (c2 << 16) | (c3 << 24));
    }
    return color;
}

void TextEditor::handleKeyboardInputs()
{
    ImGuiIO& io = ImGui::GetIO();
    auto shift = io.KeyShift;
    auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
    auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

    if (ImGui::IsWindowFocused())
    {
        if (ImGui::IsWindowHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);

        io.WantCaptureKeyboard = true;
        io.WantTextInput = true;

        if (!isReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Z))
            undo();
        else if (!isReadOnly() && !ctrl && !shift && alt && ImGui::IsKeyPressed(ImGuiKey_Backspace))
            undo();
        else if (!isReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Y))
            redo();
        else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            moveUp(1, shift);
        else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            moveDown(1, shift);
        else if (!alt && ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
            moveLeft(1, shift, ctrl);
        else if (!alt && ImGui::IsKeyPressed(ImGuiKey_RightArrow))
            moveRight(1, shift, ctrl);
        else if (!alt && ImGui::IsKeyPressed(ImGuiKey_PageUp))
            moveUp(getPageSize() - 4, shift);
        else if (!alt && ImGui::IsKeyPressed(ImGuiKey_PageDown))
            moveDown(getPageSize() - 4, shift);
        else if (!alt && ctrl && ImGui::IsKeyPressed(ImGuiKey_Home))
            moveTop(shift);
        else if (ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_End))
            moveBottom(shift);
        else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_Home))
            moveHome(shift);
        else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_End))
            moveEnd(shift);
        else if (!isReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Delete))
            remove();
        else if (!isReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Backspace))
            backspace();
        else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Insert))
            overwrite_ ^= true;
        else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Insert))
            copy();
        else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_C))
            copy();
        else if (!isReadOnly() && !ctrl && shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Insert))
            paste();
        else if (!isReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_V))
            paste();
        else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_X))
            cut();
        else if (!ctrl && shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Delete))
            cut();
        else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_A))
            selectAll();
        else if (!isReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Enter))
            enterCharacter('\n', false);
        else if (!isReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_Tab))
            enterCharacter('\t', shift);

        if (!isReadOnly() && !io.InputQueueCharacters.empty())
        {
            for (int i = 0; i < io.InputQueueCharacters.Size; i++)
            {
                auto c = io.InputQueueCharacters[i];
                if (c != 0 && (c == '\n' || c >= 32))
                    enterCharacter(c, shift);
            }
            io.InputQueueCharacters.resize(0);
        }
    }
}

void TextEditor::handleMouseInputs()
{
    ImGuiIO& io = ImGui::GetIO();
    auto shift = io.KeyShift;
    auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
    auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

    if (ImGui::IsWindowHovered())
    {
        if (!shift && !alt)
        {
            auto click = ImGui::IsMouseClicked(0);
            auto doubleClick = ImGui::IsMouseDoubleClicked(0);
            auto t = ImGui::GetTime();
            auto tripleClick = 
                click && 
                !doubleClick && 
                (
                    lastClick_ != -1.0f && 
                    (t - lastClick_) < io.MouseDoubleClickTime
                );
            if (tripleClick)
            {
                if (!ctrl)
                {
                    state_.cursorPosition = 
                    interactiveStart_ = 
                    interactiveEnd_ = 
                        screenPosToCoordinates(ImGui::GetMousePos());
                    selectionMode_ = SelectionMode::Line;
                    setSelection
                    (
                        interactiveStart_, 
                        interactiveEnd_, 
                        selectionMode_
                    );
                }
                lastClick_ = -1.0f;
            }
            else if (doubleClick)
            {
                if (!ctrl)
                {
                    state_.cursorPosition = 
                    interactiveStart_ = 
                    interactiveEnd_ = 
                        screenPosToCoordinates(ImGui::GetMousePos());
                    if (selectionMode_ == SelectionMode::Line)
                        selectionMode_ = SelectionMode::Normal;
                    else
                        selectionMode_ = SelectionMode::Word;
                    setSelection
                    (
                        interactiveStart_, 
                        interactiveEnd_, 
                        selectionMode_
                    );
                }
                lastClick_ = (float)ImGui::GetTime();
            }
            else if (click)
            {
                state_.cursorPosition = 
                interactiveStart_ = 
                interactiveEnd_ = 
                    screenPosToCoordinates(ImGui::GetMousePos());
                if (ctrl)
                    selectionMode_ = SelectionMode::Word;
                else
                    selectionMode_ = SelectionMode::Normal;
                setSelection
                (
                    interactiveStart_, 
                    interactiveEnd_, 
                    selectionMode_
                );
                lastClick_ = (float)ImGui::GetTime();
            }
            // Mouse left button dragging (=> update selection)
            else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0))
            {
                io.WantCaptureMouse = true;
                state_.cursorPosition = 
                interactiveEnd_ = 
                    screenPosToCoordinates(ImGui::GetMousePos());
                setSelection
                (
                    interactiveStart_, 
                    interactiveEnd_, 
                    selectionMode_
                );
            }
        }
    }
}

void TextEditor::render()
{
    // Compute charAdvance_ regarding to scaled font size (Ctrl + mouse wheel)
    const float fontSize = 
        ImGui::GetFont()->CalcTextSizeA
        (
            ImGui::GetFontSize(),
            FLT_MAX, 
            -1.0f,
            "#",
            nullptr,
            nullptr
        ).x;
    charAdvance_ = ImVec2
    (
        fontSize, 
        ImGui::GetTextLineHeightWithSpacing()*lineSpacing_
    );

    /* Update palette with the current alpha from style */
    for (int i = 0; i < (int)PaletteIndex::Max; ++i)
    {
        auto color = ImGui::ColorConvertU32ToFloat4(paletteBase_[i]);
        color.w *= ImGui::GetStyle().Alpha;
        palette_[i] = ImGui::ColorConvertFloat4ToU32(color);
    }

    assert(lineBuffer_.empty());

    auto contentSize = ImGui::GetWindowContentRegionMax();
    auto drawList = ImGui::GetWindowDrawList();
    float longest(textStart_);

    if (scrollToTop_)
    {
        scrollToTop_ = false;
        ImGui::SetScrollY(0.f);
    }

    ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
    auto scrollX = ImGui::GetScrollX();
    auto scrollY = ImGui::GetScrollY();

    auto lineNo = (int)floor(scrollY / charAdvance_.y);
    auto globalLineMax = (int)lines_.size();
    auto lineMax = 
        std::max
        (
            0, 
            std::min
            (
                (int)lines_.size() - 1, 
                lineNo + (int)floor((scrollY + contentSize.y)/charAdvance_.y)
            )
        );

    char buf[16];
    if (!useSetTextStart_)
        textStart_ = getLineIndexColumnWidth();

    if (!lines_.empty())
    {
        float spaceSize = 
            ImGui::GetFont()->CalcTextSizeA
            (
                ImGui::GetFontSize(), 
                FLT_MAX, 
                -1.0f, 
                " ", 
                nullptr, 
                nullptr
            ).x;

        while (lineNo <= lineMax)
        {
            ImVec2 lineStartScreenPos = 
                {cursorScreenPos.x, cursorScreenPos.y + lineNo*charAdvance_.y};
            ImVec2 textScreenPos = 
                {lineStartScreenPos.x + textStart_, lineStartScreenPos.y};

            auto& line = lines_[lineNo];
            longest = std::max
            (
                textStart_ + textDistanceToLineStart
                (
                    Coordinates(lineNo, getLineMaxColumn(lineNo))
                ), 
                longest
            );
            auto columnNo = 0;
            Coordinates lineStartCoord(lineNo, 0);
            Coordinates lineEndCoord(lineNo, getLineMaxColumn(lineNo));

            // Draw selection for the current line
            float sstart = -1.0f;
            float ssend = -1.0f;

            assert(state_.selectionStart <= state_.selectionEnd);
            if (state_.selectionStart <= lineEndCoord)
                sstart = 
                    state_.selectionStart > lineStartCoord ? 
                    textDistanceToLineStart(state_.selectionStart) : 
                    0.0f;
            if (state_.selectionEnd > lineStartCoord)
                ssend = textDistanceToLineStart
                (
                    state_.selectionEnd < lineEndCoord ? 
                    state_.selectionEnd : lineEndCoord
                );

            if (state_.selectionEnd.line > lineNo)
                ssend += charAdvance_.x;

            if (sstart != -1 && ssend != -1 && sstart < ssend)
            {
                ImVec2 vstart
                (
                    lineStartScreenPos.x + textStart_ + sstart, 
                    lineStartScreenPos.y
                );
                ImVec2 vend
                (
                    lineStartScreenPos.x + textStart_ + ssend, 
                    lineStartScreenPos.y + charAdvance_.y
                );
                drawList->AddRectFilled
                (
                    vstart, 
                    vend, 
                    palette_[(int)PaletteIndex::Selection]
                );
            }

            // Draw breakpoints
            auto start = ImVec2
            (
                lineStartScreenPos.x + scrollX,
                lineStartScreenPos.y
            );
            if (breakpoints_.count(lineNo + 1) != 0)
            {
                auto end = ImVec2
                (
                    lineStartScreenPos.x + contentSize.x + 2.0f*scrollX,
                    lineStartScreenPos.y + charAdvance_.y
                );
                drawList->AddRectFilled
                (
                    start, 
                    end, 
                    palette_[(int)PaletteIndex::Breakpoint]
                );
            }

            // Draw error markers
            auto errorIt = errorMarkers_.find(lineNo + 1);
            if (errorIt != errorMarkers_.end())
            {
                auto end = ImVec2
                (
                    lineStartScreenPos.x + contentSize.x + 2.0f*scrollX,
                    lineStartScreenPos.y + charAdvance_.y
                );
                drawList->AddRectFilled
                (
                    start, 
                    end, 
                    palette_[(int)PaletteIndex::ErrorMarker]
                );

                if (ImGui::IsMouseHoveringRect(lineStartScreenPos, end))
                {
                    ImGui::BeginTooltip();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,.2f,.2f,1));
                    ImGui::Text("Error at line %d:", errorIt->first);
                    ImGui::PopStyleColor();
                    ImGui::Separator();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,.2f,1));
                    ImGui::Text("%s", errorIt->second.c_str());
                    ImGui::PopStyleColor();
                    ImGui::EndTooltip();
                }
            }

            // Draw line number (right aligned)
            snprintf(buf, 16, "%d  ", lineNo + 1);
            auto lineNoWidth = 
                ImGui::GetFont()->CalcTextSizeA
                (
                    ImGui::GetFontSize(), 
                    FLT_MAX, 
                    -1.0f, 
                    buf, 
                    nullptr, 
                    nullptr
                ).x;
            drawList->AddText
            (
                ImVec2
                (
                    lineStartScreenPos.x + textStart_ - lineNoWidth,
                    lineStartScreenPos.y
                ), 
                palette_[(int)PaletteIndex::LineNumber],
                buf
            );

            if (state_.cursorPosition.line == lineNo)
            {
                auto focused = ImGui::IsWindowFocused();

                // Highlight the current line (where the cursor is)
                if (!hasSelection())
                {
                    auto end = ImVec2
                    (
                        start.x + contentSize.x + scrollX, 
                        start.y + charAdvance_.y
                    );
                    drawList->AddRectFilled
                    (
                        start, 
                        end, 
                        palette_
                        [
                            (int)
                            (
                                focused ? 
                                PaletteIndex::CurrentLineFill : 
                                PaletteIndex::CurrentLineFillInactive
                            )
                        ]
                    );
                    drawList->AddRect
                    (
                        start, 
                        end, 
                        palette_[(int)PaletteIndex::CurrentLineEdge], 1.0f
                    );
                }

                // Render the cursor
                if (focused)
                {
                    auto timeEnd = 
                        std::chrono::duration_cast<std::chrono::milliseconds>
                        (
                            std::chrono::system_clock::now().time_since_epoch()
                        ).count();
                    auto elapsed = timeEnd - startTime_;
                    if (elapsed > 400)
                    {
                        float width = 1.0f;
                        auto cindex = 
                            getCharacterIndex(state_.cursorPosition);
                        float cx = 
                            textDistanceToLineStart(state_.cursorPosition);

                        if (overwrite_ && cindex < (int)line.size())
                        {
                            auto c = line[cindex].character;
                            if (c == '\t')
                            {
                                auto x = 
                                (
                                    1.0f + std::floor
                                    (
                                        (1.0f+cx)/(float(tabSize_)*spaceSize)
                                    )
                                )*(float(tabSize_)*spaceSize);
                                width = x - cx;
                            }
                            else
                            {
                                char buf2[2];
                                buf2[0] = line[cindex].character;
                                buf2[1] = '\0';
                                width = 
                                    ImGui::GetFont()->CalcTextSizeA
                                    (
                                        ImGui::GetFontSize(), 
                                        FLT_MAX, 
                                        -1.0f, 
                                        buf2
                                    ).x;
                            }
                        }
                        ImVec2 cstart(textScreenPos.x+cx, lineStartScreenPos.y);
                        ImVec2 cend
                        (
                            textScreenPos.x+cx+width, 
                            lineStartScreenPos.y+charAdvance_.y
                        );
                        drawList->AddRectFilled
                        (
                            cstart, 
                            cend, 
                            palette_[(int)PaletteIndex::Cursor]
                        );
                        if (elapsed > 800)
                            startTime_ = timeEnd;
                    }
                }
            }

            // Render colorized text
            auto prevColor = 
                line.empty() ? 
                palette_[(int)PaletteIndex::Default] : 
                getGlyphColor(line[0]);
            ImVec2 bufferOffset;
            for (int i = 0; i < line.size();)
            {
                auto& glyph = line[i];
                auto color = getGlyphColor(glyph);

                if 
                (
                    (
                        color != prevColor || 
                        glyph.character == '\t' || 
                        glyph.character == ' '
                    ) && !lineBuffer_.empty()
                )
                {
                    const ImVec2 newOffset
                    (
                        textScreenPos.x + bufferOffset.x, 
                        textScreenPos.y + bufferOffset.y
                    );
                    drawList->AddText
                    (
                        newOffset, 
                        prevColor, 
                        lineBuffer_.c_str()
                    );
                    auto textSize = 
                        ImGui::GetFont()->CalcTextSizeA
                        (
                            ImGui::GetFontSize(), 
                            FLT_MAX, 
                            -1.0f, 
                            lineBuffer_.c_str(), 
                            nullptr, 
                            nullptr
                        );
                    bufferOffset.x += textSize.x;
                    lineBuffer_.clear();
                }
                prevColor = color;

                if (glyph.character == '\t')
                {
                    auto oldX = bufferOffset.x;
                    bufferOffset.x = 
                    (
                        1.0f + std::floor
                        (
                            (1.0f+bufferOffset.x)/(float(tabSize_)*spaceSize)
                        )
                    )*(float(tabSize_)*spaceSize);
                    ++i;

                    if (showWhitespaces_)
                    {
                        const auto s = ImGui::GetFontSize();
                        const auto x1 = textScreenPos.x+oldX+1.0f;
                        const auto x2 = textScreenPos.x+bufferOffset.x-1.0f;
                        const auto y = textScreenPos.y+bufferOffset.y+s*0.5f;
                        const ImVec2 p1(x1, y);
                        const ImVec2 p2(x2, y);
                        const ImVec2 p3(x2-s*0.2f, y-s*0.2f);
                        const ImVec2 p4(x2-s*0.2f, y+s*0.2f);
                        drawList->AddLine(p1, p2, 0x90909090);
                        drawList->AddLine(p2, p3, 0x90909090);
                        drawList->AddLine(p2, p4, 0x90909090);
                    }
                }
                else if (glyph.character == ' ')
                {
                    if (showWhitespaces_)
                    {
                        const auto s = ImGui::GetFontSize();
                        const auto x = 
                            textScreenPos.x+bufferOffset.x+spaceSize*0.5f;
                        const auto y = textScreenPos.y+bufferOffset.y+s*0.5f;
                        drawList->AddCircleFilled
                        (
                            ImVec2(x, y), 
                            1.5f, 
                            0x80808080, 
                            4
                        );
                    }
                    bufferOffset.x += spaceSize;
                    i++;
                }
                else
                {
                    auto l = UTF8CharLength(glyph.character);
                    while (l-- > 0)
                        lineBuffer_.push_back(line[i++].character);
                }
                ++columnNo;
            }

            if (!lineBuffer_.empty())
            {
                const ImVec2 newOffset
                (
                    textScreenPos.x + bufferOffset.x, 
                    textScreenPos.y + bufferOffset.y
                );
                drawList->AddText(newOffset, prevColor, lineBuffer_.c_str());
                lineBuffer_.clear();
            }

            ++lineNo;
        }

        // Draw a tooltip on known identifiers/preprocessor symbols
        if (ImGui::IsMousePosValid())
        {
            auto id = getWordAt(screenPosToCoordinates(ImGui::GetMousePos()));
            if (!id.empty())
            {
                auto it = languageDefinition_.identifiers.find(id);
                if (it != languageDefinition_.identifiers.end())
                {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(it->second.declaration.c_str());
                    ImGui::EndTooltip();
                }
                else
                {
                    auto pi = 
                        languageDefinition_.preprocIdentifiers.find(id);
                    if (pi != languageDefinition_.preprocIdentifiers.end())
                    {
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(pi->second.declaration.c_str());
                        ImGui::EndTooltip();
                    }
                }
            }
        }
    }

    ImGui::Dummy(ImVec2((longest + 2), lines_.size() * charAdvance_.y));

    if (scrollToCursor_)
    {
        ensureCursorVisible();
        ImGui::SetWindowFocus();
        scrollToCursor_ = false;
    }

    useSetTextStart_ = false;
}

void TextEditor::render
(
    const char* aTitle, 
    const ImVec2& aSize, 
    bool aBorder
)
{
    withinRender_ = true;
    textChanged_ = false;
    cursorPositionChanged_ = false;

    ImVec4 bgCol = 
        ImGui::ColorConvertU32ToFloat4(palette_[(int)PaletteIndex::Background]);
    bgCol.w = 0.25f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bgCol);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    // Flags edited by virmodoetiae to prevent ImGui-handled scrolling when
    // pressing keyboard, since that should be handled by the TextEditor
    // only
    static ImGuiWindowFlags windowFlags = 
        ImGuiWindowFlags_HorizontalScrollbar | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoNavInputs;
    if (!ignoreImGuiChild_)
        ImGui::BeginChild(aTitle, aSize, aBorder, windowFlags);

    if (handleKeyboardInputs_)
    {
        handleKeyboardInputs();
        ImGui::PushAllowKeyboardFocus(true);
    }

    if (handleMouseInputs_)
        handleMouseInputs();

    colorizeInternal();
    render();

    if (handleKeyboardInputs_)
        ImGui::PopAllowKeyboardFocus();

    if (!ignoreImGuiChild_)
        ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    withinRender_ = false;
}

void TextEditor::setText(const std::string & aText)
{
    lines_.clear();
    lines_.emplace_back(Line());
    for (auto chr : aText)
    {
        if (chr == '\r')
            continue; // Ignore carriage return
        else if (chr == '\n')
            lines_.emplace_back(Line());
        else if (chr == '\t')
        {
            for (int i=0; i<tabSize_; i++)
                lines_.back().emplace_back
                (
                    Glyph(' ', PaletteIndex::Default)
                );
        }
        else
            lines_.back().emplace_back(Glyph(chr, PaletteIndex::Default));
    }

    textChanged_ = true;
    scrollToTop_ = true;

    undoBuffer_.clear();
    undoIndex_ = 0;

    colorize();
}

void TextEditor::setTextLines(const std::vector<std::string> & aLines)
{
    lines_.clear();

    if (aLines.empty())
    {
        lines_.emplace_back(Line());
    }
    else
    {
        lines_.resize(aLines.size());

        for (size_t i = 0; i < aLines.size(); ++i)
        {
            const std::string & aLine = aLines[i];

            lines_[i].reserve(aLine.size());
            for (size_t j = 0; j < aLine.size(); ++j)
            {
                const char& c(aLine[j]);
                if (c == '\t')
                {
                    for (int k=0; k<tabSize_; k++)
                        lines_[i].emplace_back
                        (
                            Glyph(' ', PaletteIndex::Default)
                        );
                }
                else
                    lines_[i].emplace_back(Glyph(c, PaletteIndex::Default));
            }
        }
    }

    textChanged_ = true;
    scrollToTop_ = true;

    undoBuffer_.clear();
    undoIndex_ = 0;
    colorize();
}

void TextEditor::enterCharacter(ImWchar aChar, bool aShift, bool aaddUndo)
{
    assert(!readOnly_);

    UndoRecord u;
    u.propagate = false;
    u.before = state_;

    if (hasSelection())
    {
        if 
        (
            aChar == '\t' && 
            state_.selectionStart.line != state_.selectionEnd.line
        )
        {
            auto start = state_.selectionStart;
            auto end = state_.selectionEnd;
            auto originalEnd = end;
            if (start > end)
                std::swap(start, end);
            static ImGuiIO& io = ImGui::GetIO();
            bool shift = io.KeyShift;
            if (!shift)
            {
                for (int i = start.line; i <= end.line; i++)
                {
                    UndoRecord uu;
                    uu.addedStart.line = i;
                    uu.addedStart.column = 0;
                    uu.propagate = i > start.line;
                    uu.before = state_;
                    std::string spaces(tabSize_, ' ');
                    auto lineStart = Coordinates(i, 0);
                    insertTextAt(lineStart, spaces.c_str());
                    uu.added = spaces;
                    uu.addedEnd.line = i;
                    uu.addedEnd.column = tabSize_;
                    uu.after = state_;
                    addUndo(uu);
                }
                state_.selectionStart = 
                    Coordinates(start.line, start.column+4);
                state_.selectionEnd = Coordinates(end.line, end.column+4);
            }
            else
            {
                int deltaStartCol, deltaEndCol;
                for (int i = start.line; i <= end.line; i++)
                {
                    int nRemovedSpaces = 0;
                    auto& line = lines_[i];
                    int maxStep = std::min((int)line.size(), tabSize_);
                    while(nRemovedSpaces<maxStep)
                    {
                        if (line[nRemovedSpaces].character != ' ')
                            break;
                        nRemovedSpaces++;
                    }
                    if (i == start.line)
                        deltaStartCol = nRemovedSpaces;
                    else if (i == end.line)
                        deltaEndCol = nRemovedSpaces;
                    if (nRemovedSpaces == 0)
                        continue;
                    std::string spaces(nRemovedSpaces, ' ');
                    UndoRecord uu;
                    uu.removed = spaces;
                    uu.removedStart.line = i;
                    uu.removedStart.column = 0;
                    uu.propagate = i > start.line;
                    uu.before = state_;
                    auto lineStart = Coordinates(i, 0);
                    auto lineEnd = Coordinates(i, nRemovedSpaces);
                    deleteRange(lineStart, lineEnd);
                    uu.removedEnd.line = i;
                    uu.removedEnd.column = nRemovedSpaces;
                    uu.after = state_;
                    addUndo(uu);
                }
                state_.selectionStart = 
                    Coordinates(start.line, start.column-deltaStartCol);
                state_.selectionEnd = 
                    Coordinates(end.line, end.column-deltaEndCol);
            }

            textChanged_ = true;
            ensureCursorVisible();
            return;
        }
        else
        {
            u.removed = getSelectedText();
            u.removedStart = state_.selectionStart;
            u.removedEnd = state_.selectionEnd;
            deleteSelection();
        }
    } // hasSelection

    auto coord = getActualCursorCoordinates();
    u.addedStart = coord;

    assert(!lines_.empty());

    if (aChar == '\n')
    {
        insertLine(coord.line + 1);
        auto& line = lines_[coord.line];
        auto& newLine = lines_[coord.line + 1];

        if (languageDefinition_.autoIndentation)
            for 
            (
                size_t it = 0; 
                it < line.size() && 
                isascii(line[it].character) && 
                isblank(line[it].character); 
                ++it
            )
                newLine.push_back(line[it]);

        const size_t whitespaceSize = newLine.size();
        auto cindex = getCharacterIndex(coord);
        newLine.insert(newLine.end(), line.begin() + cindex, line.end());
        line.erase(line.begin() + cindex, line.begin() + line.size());
        setCursorPosition
        (
            Coordinates
            (
                coord.line + 1, 
                getCharacterColumn(coord.line + 1, (int)whitespaceSize)
            )
        );
        u.added = (char)aChar;
    }
    else
    {
        if (aChar == '\t')
        {
            auto coord = getCursorPosition();
            int n = tabSize_-coord.column%tabSize_;
            u.propagate = false;
            u.before = state_;
            for (int i=0;i<n;i++)
                enterCharacter(' ', aShift, false);
            u.added = std::string(n, ' ');
            u.addedEnd = getActualCursorCoordinates();
            u.after = state_;
            addUndo(u);
            return;
        }
        char buf[7];
        int e = ImTextCharToUtf8(buf, 7, aChar);
        if (e > 0)
        {
            buf[e] = '\0';
            auto& line = lines_[coord.line];
            auto cindex = getCharacterIndex(coord);

            if (overwrite_ && cindex < (int)line.size())
            {
                auto d = UTF8CharLength(line[cindex].character);

                u.removedStart = state_.cursorPosition;
                u.removedEnd = Coordinates
                (
                    coord.line, 
                    getCharacterColumn(coord.line, cindex + d)
                );

                while (d-- > 0 && cindex < (int)line.size())
                {
                    u.removed += line[cindex].character;
                    line.erase(line.begin() + cindex);
                }
            }

            for (auto p = buf; *p != '\0'; p++, ++cindex)
                line.insert
                (
                    line.begin() + cindex, 
                    Glyph(*p, PaletteIndex::Default)
                );
            u.added = buf;

            setCursorPosition
            (
                Coordinates
                (
                    coord.line, 
                    getCharacterColumn(coord.line, cindex)
                )
            );
        }
        else
            return;
    }

    textChanged_ = true;

    if (aaddUndo)
    {
        u.addedEnd = getActualCursorCoordinates();
        u.after = state_;
        addUndo(u);
    }

    colorize(coord.line - 1, 3);
    ensureCursorVisible();
}

void TextEditor::setReadOnly(bool aValue)
{
    readOnly_ = aValue;
}

void TextEditor::setColorizerEnable(bool aValue)
{
    colorizerEnabled_ = aValue;
}

void TextEditor::setCursorPosition(const Coordinates & aPosition)
{
    if (state_.cursorPosition != aPosition)
    {
        state_.cursorPosition = aPosition;
        cursorPositionChanged_ = true;
        ensureCursorVisible();
    }
}

void TextEditor::setSelectionStart(const Coordinates & aPosition)
{
    state_.selectionStart = sanitizeCoordinates(aPosition);
    if (state_.selectionStart > state_.selectionEnd)
        std::swap(state_.selectionStart, state_.selectionEnd);
}

void TextEditor::setSelectionEnd(const Coordinates & aPosition)
{
    state_.selectionEnd = sanitizeCoordinates(aPosition);
    if (state_.selectionStart > state_.selectionEnd)
        std::swap(state_.selectionStart, state_.selectionEnd);
}

void TextEditor::setSelection
(
    const Coordinates & aStart, 
    const Coordinates & aEnd, 
    SelectionMode aMode
)
{
    auto oldSelStart = state_.selectionStart;
    auto oldSelEnd = state_.selectionEnd;

    state_.selectionStart = sanitizeCoordinates(aStart);
    state_.selectionEnd = sanitizeCoordinates(aEnd);
    if (state_.selectionStart > state_.selectionEnd)
        std::swap(state_.selectionStart, state_.selectionEnd);

    switch (aMode)
    {
    case TextEditor::SelectionMode::Normal:
        break;
    case TextEditor::SelectionMode::Word:
    {
        state_.selectionStart = findWordStartPos(state_.selectionStart);
        if (!isOnWordBoundary(state_.selectionEnd))
            state_.selectionEnd = 
                findWordEndPos(findWordStartPos(state_.selectionEnd));
        break;
    }
    case TextEditor::SelectionMode::Line:
    {
        const auto lineNo = state_.selectionEnd.line;
        const auto lineSize = 
            (size_t)lineNo < lines_.size() ? lines_[lineNo].size() : 0;
        state_.selectionStart = Coordinates(state_.selectionStart.line, 0);
        state_.selectionEnd = Coordinates(lineNo, getLineMaxColumn(lineNo));
        break;
    }
    default:
        break;
    }

    if (state_.selectionStart != oldSelStart ||
        state_.selectionEnd != oldSelEnd)
        cursorPositionChanged_ = true;
}

void TextEditor::setTabSize(int aValue)
{
    tabSize_ = std::max(0, std::min(32, aValue));
}

void TextEditor::insertText
(
    const std::string & aValue, 
    bool aRegisterUndo, 
    bool aPropagateUndo
)
{
    insertText(aValue.c_str(), aRegisterUndo, aPropagateUndo);
}

void TextEditor::insertText
(
    const char* aValue, 
    bool aRegisterUndo, 
    bool aPropagateUndo
)
{
    if (aValue == nullptr)
        return;

    std::string sAValue = std::string(aValue);
    std::string aValueNoTabs;
    static std::regex const tab(R"(\t)");
    std::regex_replace
    (
        std::back_inserter(aValueNoTabs), 
        std::begin(sAValue), 
        std::end(sAValue), 
        tab,
        "    "
    );

    UndoRecord u;
    if (aRegisterUndo)
    {
        u.propagate = aPropagateUndo;
        u.added = aValueNoTabs;
        u.addedStart = getActualCursorCoordinates();
    }

    auto pos = getActualCursorCoordinates();
    auto start = std::min(pos, state_.selectionStart);
    int totalLines = pos.line - start.line;

    totalLines += insertTextAt(pos, aValueNoTabs.c_str());

    setSelection(pos, pos);
    setCursorPosition(pos);
    colorize(start.line - 1, totalLines + 2);

    if (aRegisterUndo)
    {
        u.addedEnd = getActualCursorCoordinates();
        u.after = state_;
        addUndo(u);
    }
}

void TextEditor::deleteSelection()
{
    assert(state_.selectionEnd >= state_.selectionStart);

    if (state_.selectionEnd == state_.selectionStart)
        return;

    deleteRange(state_.selectionStart, state_.selectionEnd);

    setSelection(state_.selectionStart, state_.selectionStart);
    setCursorPosition(state_.selectionStart);
    colorize(state_.selectionStart.line, 1);
}

void TextEditor::moveUp(int aAmount, bool aSelect)
{
    auto oldPos = state_.cursorPosition;
    state_.cursorPosition.line = 
        std::max(0, state_.cursorPosition.line - aAmount);
    if (oldPos != state_.cursorPosition)
    {
        if (aSelect)
        {
            if (oldPos == interactiveStart_)
                interactiveStart_ = state_.cursorPosition;
            else if (oldPos == interactiveEnd_)
                interactiveEnd_ = state_.cursorPosition;
            else
            {
                interactiveStart_ = state_.cursorPosition;
                interactiveEnd_ = oldPos;
            }
        }
        else
            interactiveStart_ = interactiveEnd_ = state_.cursorPosition;
        setSelection(interactiveStart_, interactiveEnd_);

        ensureCursorVisible();
    }
}

void TextEditor::moveDown(int aAmount, bool aSelect)
{
    assert(state_.cursorPosition.column >= 0);
    auto oldPos = state_.cursorPosition;
    state_.cursorPosition.line = 
        std::max
        (
            0, 
            std::min
            (
                (int)lines_.size() - 1, 
                state_.cursorPosition.line + aAmount
            )
        );

    if (state_.cursorPosition != oldPos)
    {
        if (aSelect)
        {
            if (oldPos == interactiveEnd_)
                interactiveEnd_ = state_.cursorPosition;
            else if (oldPos == interactiveStart_)
                interactiveStart_ = state_.cursorPosition;
            else
            {
                interactiveStart_ = oldPos;
                interactiveEnd_ = state_.cursorPosition;
            }
        }
        else
            interactiveStart_ = interactiveEnd_ = state_.cursorPosition;
        setSelection(interactiveStart_, interactiveEnd_);

        ensureCursorVisible();
    }
}

static bool IsUTFSequence(char c)
{
    return (c & 0xC0) == 0x80;
}

void TextEditor::moveLeft(int aAmount, bool aSelect, bool aWordMode)
{
    if (lines_.empty())
        return;

    auto oldPos = state_.cursorPosition;
    state_.cursorPosition = getActualCursorCoordinates();
    auto line = state_.cursorPosition.line;
    auto cindex = getCharacterIndex(state_.cursorPosition);

    while (aAmount-- > 0)
    {
        if (cindex == 0)
        {
            if (line > 0)
            {
                --line;
                if ((int)lines_.size() > line)
                    cindex = (int)lines_[line].size();
                else
                    cindex = 0;
            }
        }
        else
        {
            --cindex;
            if (cindex > 0)
            {
                if ((int)lines_.size() > line)
                {
                    while 
                    (
                        cindex > 0 && 
                        IsUTFSequence(lines_[line][cindex].character)
                    )
                        --cindex;
                }
            }
        }

        state_.cursorPosition = 
            Coordinates(line, getCharacterColumn(line, cindex));
        if (aWordMode)
        {
            state_.cursorPosition = findWordStartPos(state_.cursorPosition);
            cindex = getCharacterIndex(state_.cursorPosition);
        }
    }

    state_.cursorPosition = 
        Coordinates(line, getCharacterColumn(line, cindex));

    assert(state_.cursorPosition.column >= 0);
    if (aSelect)
    {
        if (oldPos == interactiveStart_)
            interactiveStart_ = state_.cursorPosition;
        else if (oldPos == interactiveEnd_)
            interactiveEnd_ = state_.cursorPosition;
        else
        {
            interactiveStart_ = state_.cursorPosition;
            interactiveEnd_ = oldPos;
        }
    }
    else
        interactiveStart_ = interactiveEnd_ = state_.cursorPosition;
    setSelection
    (
        interactiveStart_, 
        interactiveEnd_, 
        aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal
    );

    ensureCursorVisible();
}

void TextEditor::moveRight(int aAmount, bool aSelect, bool aWordMode)
{
    auto oldPos = state_.cursorPosition;

    if (lines_.empty() || oldPos.line >= lines_.size())
        return;

    auto cindex = getCharacterIndex(state_.cursorPosition);
    while (aAmount-- > 0)
    {
        auto lindex = state_.cursorPosition.line;
        auto& line = lines_[lindex];

        if (cindex >= line.size())
        {
            if (state_.cursorPosition.line < lines_.size() - 1)
            {
                state_.cursorPosition.line = std::max
                (
                    0, 
                    std::min
                    (
                        (int)lines_.size() - 1, 
                        state_.cursorPosition.line + 1
                    )
                );
                state_.cursorPosition.column = 0;
            }
            else
                return;
        }
        else
        {
            cindex += UTF8CharLength(line[cindex].character);
            state_.cursorPosition = 
                Coordinates(lindex, getCharacterColumn(lindex, cindex));
            if (aWordMode)
                state_.cursorPosition = findNextWordPos(state_.cursorPosition);
        }
    }

    if (aSelect)
    {
        if (oldPos == interactiveEnd_)
            interactiveEnd_ = sanitizeCoordinates(state_.cursorPosition);
        else if (oldPos == interactiveStart_)
            interactiveStart_ = state_.cursorPosition;
        else
        {
            interactiveStart_ = oldPos;
            interactiveEnd_ = state_.cursorPosition;
        }
    }
    else
        interactiveStart_ = interactiveEnd_ = state_.cursorPosition;
    setSelection
    (
        interactiveStart_, 
        interactiveEnd_, 
        aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal
    );

    ensureCursorVisible();
}

void TextEditor::moveTop(bool aSelect)
{
    auto oldPos = state_.cursorPosition;
    setCursorPosition(Coordinates(0, 0));

    if (state_.cursorPosition != oldPos)
    {
        if (aSelect)
        {
            interactiveEnd_ = oldPos;
            interactiveStart_ = state_.cursorPosition;
        }
        else
            interactiveStart_ = interactiveEnd_ = state_.cursorPosition;
        setSelection(interactiveStart_, interactiveEnd_);
    }
}

void TextEditor::TextEditor::moveBottom(bool aSelect)
{
    auto oldPos = getCursorPosition();
    auto newPos = Coordinates((int)lines_.size() - 1, 0);
    setCursorPosition(newPos);
    if (aSelect)
    {
        interactiveStart_ = oldPos;
        interactiveEnd_ = newPos;
    }
    else
        interactiveStart_ = interactiveEnd_ = newPos;
    setSelection(interactiveStart_, interactiveEnd_);
}

void TextEditor::moveHome(bool aSelect)
{
    auto oldPos = state_.cursorPosition;
    setCursorPosition(Coordinates(state_.cursorPosition.line, 0));

    if (state_.cursorPosition != oldPos)
    {
        if (aSelect)
        {
            if (oldPos == interactiveStart_)
                interactiveStart_ = state_.cursorPosition;
            else if (oldPos == interactiveEnd_)
                interactiveEnd_ = state_.cursorPosition;
            else
            {
                interactiveStart_ = state_.cursorPosition;
                interactiveEnd_ = oldPos;
            }
        }
        else
            interactiveStart_ = interactiveEnd_ = state_.cursorPosition;
        setSelection(interactiveStart_, interactiveEnd_);
    }
}

void TextEditor::moveEnd(bool aSelect)
{
    auto oldPos = state_.cursorPosition;
    setCursorPosition
    (
        Coordinates
        (
            state_.cursorPosition.line, 
            getLineMaxColumn(oldPos.line)
        )
    );

    if (state_.cursorPosition != oldPos)
    {
        if (aSelect)
        {
            if (oldPos == interactiveEnd_)
                interactiveEnd_ = state_.cursorPosition;
            else if (oldPos == interactiveStart_)
                interactiveStart_ = state_.cursorPosition;
            else
            {
                interactiveStart_ = oldPos;
                interactiveEnd_ = state_.cursorPosition;
            }
        }
        else
            interactiveStart_ = interactiveEnd_ = state_.cursorPosition;
        setSelection(interactiveStart_, interactiveEnd_);
    }
}

void TextEditor::remove(bool aPropagateUndo)
{
    assert(!readOnly_);

    if (lines_.empty())
        return;

    UndoRecord u;
    u.before = state_;
    u.propagate = aPropagateUndo;

    if (hasSelection())
    {
        u.removed = getSelectedText();
        u.removedStart = state_.selectionStart;
        u.removedEnd = state_.selectionEnd;

        deleteSelection();
    }
    else
    {
        auto pos = getActualCursorCoordinates();
        setCursorPosition(pos);
        auto& line = lines_[pos.line];

        if (pos.column == getLineMaxColumn(pos.line))
        {
            if (pos.line == (int)lines_.size() - 1)
                return;

            u.removed = '\n';
            u.removedStart = u.removedEnd = getActualCursorCoordinates();
            advance(u.removedEnd);

            auto& nextLine = lines_[pos.line + 1];
            line.insert(line.end(), nextLine.begin(), nextLine.end());
            removeLine(pos.line + 1);
        }
        else
        {
            auto cindex = getCharacterIndex(pos);
            u.removedStart = u.removedEnd = getActualCursorCoordinates();
            u.removedEnd.column++;
            u.removed = getText(u.removedStart, u.removedEnd);

            auto d = UTF8CharLength(line[cindex].character);
            while (d-- > 0 && cindex < (int)line.size())
                line.erase(line.begin() + cindex);
        }

        textChanged_ = true;

        colorize(pos.line, 1);
    }

    u.after = state_;
    addUndo(u);
}

void TextEditor::backspace()
{
    assert(!readOnly_);

    if (lines_.empty())
        return;

    UndoRecord u;
    u.propagate = false;
    u.before = state_;

    if (hasSelection())
    {
        u.removed = getSelectedText();
        u.removedStart = state_.selectionStart;
        u.removedEnd = state_.selectionEnd;

        deleteSelection();
    }
    else
    {
        auto pos = getActualCursorCoordinates();
        setCursorPosition(pos);

        if (state_.cursorPosition.column == 0)
        {
            if (state_.cursorPosition.line == 0)
                return;

            u.removed = '\n';
            u.removedStart = 
            u.removedEnd = 
                Coordinates
                (
                    pos.line - 1, 
                    getLineMaxColumn(pos.line - 1)
                );
            advance(u.removedEnd);

            auto& line = lines_[state_.cursorPosition.line];
            auto& prevLine = lines_[state_.cursorPosition.line-1];
            auto prevSize = getLineMaxColumn(state_.cursorPosition.line-1);
            prevLine.insert(prevLine.end(), line.begin(), line.end());

            ErrorMarkers etmp;
            for (auto& i : errorMarkers_)
                etmp.insert
                (
                    ErrorMarkers::value_type
                    (
                        i.first - 1 == state_.cursorPosition.line ? 
                        i.first - 1 : 
                        i.first, i.second
                    )
                );
            errorMarkers_ = std::move(etmp);

            removeLine(state_.cursorPosition.line);
            --state_.cursorPosition.line;
            state_.cursorPosition.column = prevSize;
        }
        else
        {
            auto& line = lines_[state_.cursorPosition.line];
            auto cindex = getCharacterIndex(pos) - 1;
            auto cend = cindex + 1;
            while (cindex > 0 && IsUTFSequence(line[cindex].character))
                --cindex;

            u.removedStart = u.removedEnd = getActualCursorCoordinates();
            --u.removedStart.column;
            --state_.cursorPosition.column;

            while (cindex < line.size() && cend-- > cindex)
            {
                u.removed += line[cindex].character;
                line.erase(line.begin() + cindex);
            }
        }

        textChanged_ = true;

        ensureCursorVisible();
        colorize(state_.cursorPosition.line, 1);
    }

    u.after = state_;
    addUndo(u);
}

void TextEditor::selectWordUnderCursor()
{
    auto c = getCursorPosition();
    setSelection(findWordStartPos(c), findWordEndPos(c));
}

void TextEditor::selectAll()
{
    setSelection(Coordinates(0, 0), Coordinates((int)lines_.size(), 0));
}

bool TextEditor::hasSelection() const
{
    return state_.selectionEnd > state_.selectionStart;
}

void TextEditor::copy()
{
    if (hasSelection())
    {
        ImGui::SetClipboardText(getSelectedText().c_str());
    }
    else
    {
        if (!lines_.empty())
        {
            std::string str;
            auto& line = lines_[getActualCursorCoordinates().line];
            for (auto& g : line)
                str.push_back(g.character);
            ImGui::SetClipboardText(str.c_str());
        }
    }
}

void TextEditor::cut()
{
    if (isReadOnly())
    {
        copy();
    }
    else
    {
        if (hasSelection())
        {
            UndoRecord u;
            u.propagate = false;
            u.before = state_;
            u.removed = getSelectedText();
            u.removedStart = state_.selectionStart;
            u.removedEnd = state_.selectionEnd;

            copy();
            deleteSelection();

            u.after = state_;
            addUndo(u);
        }
    }
}

void TextEditor::paste()
{
    if (isReadOnly())
        return;

    auto clipText = ImGui::GetClipboardText();
    if (clipText != nullptr && strlen(clipText) > 0)
    {
        UndoRecord u;
        u.propagate = false;
        u.before = state_;

        if (hasSelection())
        {
            u.removed = getSelectedText();
            u.removedStart = state_.selectionStart;
            u.removedEnd = state_.selectionEnd;
            deleteSelection();
        }

        u.added = clipText;
        u.addedStart = getActualCursorCoordinates();

        insertText(clipText);

        u.addedEnd = getActualCursorCoordinates();
        u.after = state_;
        addUndo(u);
    }
}

bool TextEditor::canUndo() const
{
    return !readOnly_ && undoIndex_ > 0;
}

bool TextEditor::canRedo() const
{
    return !readOnly_ && undoIndex_ < (int)undoBuffer_.size();
}

void TextEditor::undo(int aSteps)
{
    while (canUndo() && aSteps-- > 0)
    {
        auto& u(undoBuffer_[undoIndex_-1]);
        if (undoBuffer_[undoIndex_-1].propagate)
            aSteps++;
        undoBuffer_[--undoIndex_].undo(this);
    }
}

void TextEditor::redo(int aSteps)
{
    while (canRedo() && aSteps-- > 0)
    {
        if (undoIndex_+1 < undoBuffer_.size())
        {
            if (undoBuffer_[undoIndex_+1].propagate)
                aSteps++;
        }
        undoBuffer_[undoIndex_++].redo(this);
    }
}

const TextEditor::Palette & TextEditor::getDarkPalette()
{
    const static Palette p = { {
            0xff7f7f7f,	// Default
            0xffd69c56,	// Keyword	
            0xff00ff00,	// Number
            0xff7070e0,	// String
            0xff70a0e0, // Char literal
            0xffffffff, // Punctuation
            0xff408080,	// Preprocessor
            0xffaaaaaa, // Identifier
            0xff9bc64d, // Known identifier
            0xffc040a0, // Preproc identifier
            0xff206020, // Comment (single line)
            0xff406020, // Comment (multi line)
            0xff101010, // Background
            0xffe0e0e0, // Cursor
            0xc8a06020, // Selection // 80a06020
            0x800020ff, // ErrorMarker
            0x40f08000, // Breakpoint
            0xff707000, // Line number
            0x40000000, // Current line fill
            0x40808080, // Current line fill (inactive)
            0x40a0a0a0, // Current line edge
        } };
    return p;
}

const TextEditor::Palette & TextEditor::getLightPalette()
{
    const static Palette p = { {
            0xff7f7f7f,	// None
            0xffff0c06,	// Keyword	
            0xff008000,	// Number
            0xff2020a0,	// String
            0xff304070, // Char literal
            0xff000000, // Punctuation
            0xff406060,	// Preprocessor
            0xff404040, // Identifier
            0xff606010, // Known identifier
            0xffc040a0, // Preproc identifier
            0xff205020, // Comment (single line)
            0xff405020, // Comment (multi line)
            0xffffffff, // Background
            0xff000000, // Cursor
            0x80600000, // Selection
            0xa00010ff, // ErrorMarker
            0x80f08000, // Breakpoint
            0xff505000, // Line number
            0x40000000, // Current line fill
            0x40808080, // Current line fill (inactive)
            0x40000000, // Current line edge
        } };
    return p;
}

const TextEditor::Palette & TextEditor::getRetroBluePalette()
{
    const static Palette p = { {
            0xff00ffff,	// None
            0xffffff00,	// Keyword	
            0xff00ff00,	// Number
            0xff808000,	// String
            0xff808000, // Char literal
            0xffffffff, // Punctuation
            0xff008000,	// Preprocessor
            0xff00ffff, // Identifier
            0xffffffff, // Known identifier
            0xffff00ff, // Preproc identifier
            0xff808080, // Comment (single line)
            0xff404040, // Comment (multi line)
            0xff800000, // Background
            0xff0080ff, // Cursor
            0x80ffff00, // Selection
            0xa00000ff, // ErrorMarker
            0x80ff8000, // Breakpoint
            0xff808000, // Line number
            0x40000000, // Current line fill
            0x40808080, // Current line fill (inactive)
            0x40000000, // Current line edge
        } };
    return p;
}


std::string TextEditor::getText() const
{
    return getText(Coordinates(), Coordinates((int)lines_.size(), 0));
}

std::vector<std::string> TextEditor::getTextLines() const
{
    std::vector<std::string> result;
    result.reserve(lines_.size());
    for (auto & line : lines_)
    {
        std::string text;

        text.resize(line.size());

        for (size_t i = 0; i < line.size(); ++i)
            text[i] = line[i].character;

        result.emplace_back(std::move(text));
    }
    return result;
}

std::string TextEditor::getSelectedText() const
{
    return getText(state_.selectionStart, state_.selectionEnd);
}

std::string TextEditor::getCurrentLineText()const
{
    auto lineLength = getLineMaxColumn(state_.cursorPosition.line);
    return getText(
        Coordinates(state_.cursorPosition.line, 0),
        Coordinates(state_.cursorPosition.line, lineLength));
}

void TextEditor::processInputs()
{
}

void TextEditor::colorize(int aFroline, int aLines)
{
    int toLine = 
        aLines == -1 ? 
        (int)lines_.size() : 
        std::min((int)lines_.size(), aFroline + aLines);
    colorRangeMin_ = std::min(colorRangeMin_, aFroline);
    colorRangeMax_ = std::max(colorRangeMax_, toLine);
    colorRangeMin_ = std::max(0, colorRangeMin_);
    colorRangeMax_ = std::max(colorRangeMin_, colorRangeMax_);
    checkComments_ = true;
}

void TextEditor::colorizeRange(int aFroline, int aToLine)
{
    if (lines_.empty() || aFroline >= aToLine)
        return;

    std::string buffer;
    std::cmatch results;
    std::string id;

    int endLine = std::max(0, std::min((int)lines_.size(), aToLine));
    for (int i = aFroline; i < endLine; ++i)
    {
        auto& line = lines_[i];

        if (line.empty())
            continue;

        buffer.resize(line.size());
        for (size_t j = 0; j < line.size(); ++j)
        {
            auto& col = line[j];
            buffer[j] = col.character;
            col.colorIndex = PaletteIndex::Default;
        }

        const char * bufferBegin = &buffer.front();
        const char * bufferEnd = bufferBegin + buffer.size();

        auto last = bufferEnd;

        for (auto first = bufferBegin; first != last; )
        {
            const char * token_begin = nullptr;
            const char * token_end = nullptr;
            PaletteIndex token_color = PaletteIndex::Default;

            bool hasTokenizeResult = false;

            if (languageDefinition_.tokenize != nullptr)
            {
                if 
                (
                    languageDefinition_.tokenize
                    (
                        first, 
                        last, 
                        token_begin, 
                        token_end, 
                        token_color
                    )
                )
                    hasTokenizeResult = true;
            }

            if (hasTokenizeResult == false)
            {
                for (auto& p : regexList_)
                {
                    if 
                    (
                        std::regex_search
                        (
                            first, 
                            last, 
                            results, 
                            p.first, 
                            std::regex_constants::match_continuous
                        )
                    )
                    {
                        hasTokenizeResult = true;

                        auto& v = *results.begin();
                        token_begin = v.first;
                        token_end = v.second;
                        token_color = p.second;
                        break;
                    }
                }
            }

            if (hasTokenizeResult == false)
            {
                first++;
            }
            else
            {
                const size_t token_length = token_end - token_begin;

                if (token_color == PaletteIndex::Identifier)
                {
                    id.assign(token_begin, token_end);

                    if (!languageDefinition_.caseSensitive)
                        std::transform
                        (
                            id.begin(), 
                            id.end(), 
                            id.begin(), 
                            ::toupper
                        );

                    if (!line[first - bufferBegin].preprocessor)
                    {
                        if (languageDefinition_.keywords.count(id)!=0)
                            token_color = PaletteIndex::Keyword;
                        else if (languageDefinition_.identifiers.count(id)!=0)
                            token_color = PaletteIndex::KnownIdentifier;
                        else if 
                        (
                            languageDefinition_.preprocIdentifiers.count(id)!=0
                        )
                            token_color = PaletteIndex::PreprocIdentifier;
                    }
                    else if 
                    (
                        languageDefinition_.preprocIdentifiers.count(id)!=0
                    )
                    {
                        token_color = PaletteIndex::PreprocIdentifier;
                    }
                }

                for (size_t j = 0; j < token_length; ++j)
                    line[(token_begin - bufferBegin) + j].colorIndex = 
                        token_color;

                first = token_end;
            }
        }
    }
}

void TextEditor::colorizeInternal()
{
    if (lines_.empty() || !colorizerEnabled_)
        return;

    if (checkComments_)
    {
        auto endLine = lines_.size();
        auto endIndex = 0;
        auto commentStartLine = endLine;
        auto commentStartIndex = endIndex;
        auto withinString = false;
        auto withinSingleLineComment = false;
        auto withinPreproc = false;
        auto firstChar = true; // there are no other non-whitespace characters
                               // in the line before
        auto concatenate = false; // '\' on the very end of the line
        auto currentLine = 0;
        auto currentIndex = 0;
        while (currentLine < endLine || currentIndex < endIndex)
        {
            auto& line = lines_[currentLine];

            if (currentIndex == 0 && !concatenate)
            {
                withinSingleLineComment = false;
                withinPreproc = false;
                firstChar = true;
            }

            concatenate = false;

            if (!line.empty())
            {
                auto& g = line[currentIndex];
                auto c = g.character;

                if (c != languageDefinition_.preprocChar && !isspace(c))
                    firstChar = false;

                if 
                (
                    currentIndex == (int)line.size() - 1 && 
                    line[line.size() - 1].character == '\\'
                )
                    concatenate = true;

                bool inComment = 
                    commentStartLine < currentLine || 
                    (
                        commentStartLine == currentLine && 
                        commentStartIndex <= currentIndex
                    );

                if (withinString)
                {
                    line[currentIndex].multiLineComment = inComment;

                    if (c == '\"')
                    {
                        if 
                        (
                            currentIndex + 1 < (int)line.size() && 
                            line[currentIndex + 1].character == '\"'
                        )
                        {
                            currentIndex += 1;
                            if (currentIndex < (int)line.size())
                                line[currentIndex].multiLineComment = 
                                    inComment;
                        }
                        else
                            withinString = false;
                    }
                    else if (c == '\\')
                    {
                        currentIndex += 1;
                        if (currentIndex < (int)line.size())
                            line[currentIndex].multiLineComment = inComment;
                    }
                }
                else
                {
                    if (firstChar && c == languageDefinition_.preprocChar)
                        withinPreproc = true;

                    if (c == '\"')
                    {
                        withinString = true;
                        line[currentIndex].multiLineComment = inComment;
                    }
                    else
                    {
                        auto pred = [](const char& a, const Glyph& b) 
                        {
                            return a == b.character;
                        };
                        auto from = line.begin() + currentIndex;
                        auto& startStr = languageDefinition_.commentStart;
                        auto& singleStartStr = 
                            languageDefinition_.singleLineComment;

                        if 
                        (   singleStartStr.size() > 0 &&
                            currentIndex+singleStartStr.size() <= line.size() &&
                            equals
                            (
                                singleStartStr.begin(), 
                                singleStartStr.end(), 
                                from, 
                                from + singleStartStr.size(), 
                                pred
                            )
                        )
                        {
                            withinSingleLineComment = true;
                        }
                        else if 
                        (
                            !withinSingleLineComment && 
                            currentIndex + startStr.size() <= line.size() &&
                            equals
                            (
                                startStr.begin(), 
                                startStr.end(), 
                                from, 
                                from + startStr.size(), 
                                pred)
                            )
                        {
                            commentStartLine = currentLine;
                            commentStartIndex = currentIndex;
                        }

                        inComment = 
                        (
                            commentStartLine < currentLine || 
                            (
                                commentStartLine == currentLine && 
                                commentStartIndex <= currentIndex
                            )
                        );

                        line[currentIndex].multiLineComment = inComment;
                        line[currentIndex].comment = withinSingleLineComment;

                        auto& endStr = languageDefinition_.commentEnd;
                        if 
                        (
                            currentIndex + 1 >= (int)endStr.size() &&
                            equals
                            (
                                endStr.begin(), 
                                endStr.end(), 
                                from + 1 - endStr.size(), 
                                from + 1, 
                                pred
                            )
                        )
                        {
                            commentStartIndex = endIndex;
                            commentStartLine = endLine;
                        }
                    }
                }
                line[currentIndex].preprocessor = withinPreproc;
                currentIndex += UTF8CharLength(c);
                if (currentIndex >= (int)line.size())
                {
                    currentIndex = 0;
                    ++currentLine;
                }
            }
            else
            {
                currentIndex = 0;
                ++currentLine;
            }
        }
        checkComments_ = false;
    }

    if (colorRangeMin_ < colorRangeMax_)
    {
        const int increment = 
            (languageDefinition_.tokenize == nullptr) ? 10 : 10000;
        const int to = std::min(colorRangeMin_ + increment, colorRangeMax_);
        colorizeRange(colorRangeMin_, to);
        colorRangeMin_ = to;

        if (colorRangeMax_ == colorRangeMin_)
        {
            colorRangeMin_ = std::numeric_limits<int>::max();
            colorRangeMax_ = 0;
        }
        return;
    }
}

float TextEditor::textDistanceToLineStart(const Coordinates& aFrom) const
{
    auto& line = lines_[aFrom.line];
    float distance = 0.0f;
    float spaceSize = ImGui::GetFont()->CalcTextSizeA
    (
        ImGui::GetFontSize(), 
        FLT_MAX, 
        -1.0f, 
        " ", 
        nullptr, 
        nullptr
    ).x;
    int colIndex = getCharacterIndex(aFrom);
    for (size_t it = 0u; it < line.size() && it < colIndex;)
    {
        if (line[it].character == '\t')
        {
            distance = 
            (
                1.0f + std::floor((1.0f+distance)/(float(tabSize_)*spaceSize))
            )*(float(tabSize_)*spaceSize);
            ++it;
        }
        else
        {
            auto d = UTF8CharLength(line[it].character);
            char tempCString[7];
            int i = 0;
            for (; i < 6 && d-- > 0 && it < (int)line.size(); i++, it++)
                tempCString[i] = line[it].character;

            tempCString[i] = '\0';
            distance += ImGui::GetFont()->CalcTextSizeA
            (
                ImGui::GetFontSize(), 
                FLT_MAX, 
                -1.0f, 
                tempCString, 
                nullptr, 
                nullptr
            ).x;
        }
    }

    return distance;
}

void TextEditor::ensureCursorVisible()
{
    if (!withinRender_)
    {
        scrollToCursor_ = true;
        return;
    }

    float scrollX = ImGui::GetScrollX();
    float scrollY = ImGui::GetScrollY();

    auto height = ImGui::GetWindowHeight();
    auto width = ImGui::GetWindowWidth();

    auto top = 1+(int)ceil(scrollY/charAdvance_.y);
    auto bottom = (int)ceil((scrollY+height)/charAdvance_.y);

    auto left = (int)ceil(scrollX/charAdvance_.x);
    auto right = (int)ceil((scrollX+width)/charAdvance_.x);

    auto pos = getActualCursorCoordinates();
    auto len = textDistanceToLineStart(pos);
    
    // Modified horizontal scrolling behaviour
    if (pos.line < top)
        ImGui::SetScrollY(std::max(0.0f, (pos.line-1)*charAdvance_.y));
    if (pos.line > bottom-4)
        ImGui::SetScrollY(std::max(0.0f, (pos.line+4)*charAdvance_.y-height));
    if (pos.column < left)
        ImGui::SetScrollX(std::max(0.0f, (pos.column-1)*charAdvance_.x));
    if (pos.column > right-8)
        ImGui::SetScrollX(std::max(0.0f, (pos.column+8)*charAdvance_.x-width));
}

int TextEditor::getPageSize() const
{
    auto height = ImGui::GetWindowHeight()-20.0f;
    return (int)floor(height / charAdvance_.y);
}

TextEditor::UndoRecord::UndoRecord
(
    const std::string& aAdded,
    const TextEditor::Coordinates aAddedStart,
    const TextEditor::Coordinates aAddedEnd,
    const std::string& aRemoved,
    const TextEditor::Coordinates aRemovedStart,
    const TextEditor::Coordinates aRemovedEnd,
    TextEditor::EditorState& aBefore,
    TextEditor::EditorState& aAfter
)
    : added(aAdded)
    , addedStart(aAddedStart)
    , addedEnd(aAddedEnd)
    , removed(aRemoved)
    , removedStart(aRemovedStart)
    , removedEnd(aRemovedEnd)
    , before(aBefore)
    , after(aAfter)
{
    assert(addedStart <= addedEnd);
    assert(removedStart <= removedEnd);
}

void TextEditor::UndoRecord::undo(TextEditor * aEditor)
{
    if (!added.empty())
    {
        aEditor->deleteRange(addedStart, addedEnd);
        aEditor->colorize
        (
            addedStart.line-1,
            addedEnd.line-addedStart.line+2
        );
    }

    if (!removed.empty())
    {
        auto start = removedStart;
        aEditor->insertTextAt(start, removed.c_str());
        aEditor->colorize
        (
            removedStart.line-1, 
            removedEnd.line-removedStart.line+2
        );
    }

    aEditor->state_ = before;
    aEditor->ensureCursorVisible();

}

void TextEditor::UndoRecord::redo(TextEditor * aEditor)
{
    if (!removed.empty())
    {
        aEditor->deleteRange(removedStart, removedEnd);
        aEditor->colorize
        (
            removedStart.line-1, 
            removedEnd.line-removedStart.line+1
        );
    }

    if (!added.empty())
    {
        auto start = addedStart;
        aEditor->insertTextAt(start, added.c_str(), true);
        aEditor->colorize
        (
            addedStart.line-1, 
            addedEnd.line-addedStart.line+1
        );
    }

    aEditor->state_ = after;
    aEditor->ensureCursorVisible();
}

static bool TokenizeCStyleString
(
    const char * inBegin, 
    const char * inEnd, 
    const char *& outBegin, 
    const char *& outEnd
)
{
    const char * p = inBegin;

    if (*p == '"')
    {
        p++;

        while (p < inEnd)
        {
            // handle end of string
            if (*p == '"')
            {
                outBegin = inBegin;
                outEnd = p + 1;
                return true;
            }
            // handle escape character for "
            if (*p == '\\' && p + 1 < inEnd && p[1] == '"')
                p++;
            p++;
        }
    }

    return false;
}

static bool TokenizeCStyleCharacterLiteral
(
    const char * inBegin, 
    const char * inEnd, 
    const char *& outBegin, 
    const char *& outEnd
)
{
    const char * p = inBegin;

    if (*p == '\'')
    {
        p++;

        // handle escape characters
        if (p < inEnd && *p == '\\')
            p++;

        if (p < inEnd)
            p++;

        // handle end of character literal
        if (p < inEnd && *p == '\'')
        {
            outBegin = inBegin;
            outEnd = p + 1;
            return true;
        }
    }

    return false;
}

static bool TokenizeCStyleIdentifier
(
    const char * inBegin, 
    const char * inEnd, 
    const char *& outBegin, 
    const char *& outEnd
)
{
    const char * p = inBegin;

    if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')
    {
        p++;
        while 
        (
            (p < inEnd) && 
            ((*p >= 'a' && *p <= 'z') || 
            (*p >= 'A' && *p <= 'Z') || 
            (*p >= '0' && *p <= '9') || 
            *p == '_')
        )
            p++;
        outBegin = inBegin;
        outEnd = p;
        return true;
    }

    return false;
}

static bool TokenizeCStyleNumber
(
    const char * inBegin, 
    const char * inEnd, 
    const char *& outBegin, 
    const char *& outEnd
)
{
    const char * p = inBegin;
    const bool startsWithNumber = *p >= '0' && *p <= '9';
    if (*p != '+' && *p != '-' && !startsWithNumber)
        return false;
    p++;
    bool hasNumber = startsWithNumber;
    while (p < inEnd && (*p >= '0' && *p <= '9'))
    {
        hasNumber = true;
        p++;
    }
    if (hasNumber == false)
        return false;
    bool isFloat = false;
    bool isHex = false;
    bool isBinary = false;
    if (p < inEnd)
    {
        if (*p == '.')
        {
            isFloat = true;
            p++;
            while (p < inEnd && (*p >= '0' && *p <= '9'))
                p++;
        }
        else if (*p == 'x' || *p == 'X')
        {
            isHex = true;
            p++;
            while 
            (
                p < inEnd && 
                (
                    (*p >= '0' && *p <= '9') || 
                    (*p >= 'a' && *p <= 'f') || 
                    (*p >= 'A' && *p <= 'F')
                )
            )
                p++;
        }
        else if (*p == 'b' || *p == 'B')
        {
            isBinary = true;
            p++;
            while (p < inEnd && (*p >= '0' && *p <= '1'))
                p++;
        }
    }

    if (isHex == false && isBinary == false)
    {
        // floating point exponent
        if (p < inEnd && (*p == 'e' || *p == 'E'))
        {
            isFloat = true;
            p++;
            if (p < inEnd && (*p == '+' || *p == '-'))
                p++;
            bool hasDigits = false;
            while (p < inEnd && (*p >= '0' && *p <= '9'))
            {
                hasDigits = true;
                p++;
            }
            if (hasDigits == false)
                return false;
        }

        // single precision floating point type
        if (p < inEnd && *p == 'f')
            p++;
    }

    if (isFloat == false)
    {
        // integer size type
        while 
        (
            p < inEnd && 
            (
                *p == 'u' || 
                *p == 'U' || 
                *p == 'l' || 
                *p == 'L'
            )
        )
            p++;
    }

    outBegin = inBegin;
    outEnd = p;
    return true;
}

static bool TokenizeCStylePunctuation
(
    const char * inBegin, 
    const char * inEnd, 
    const char *& outBegin, 
    const char *& outEnd
)
{
    (void)inEnd;

    switch (*inBegin)
    {
    case '[':
    case ']':
    case '{':
    case '}':
    case '!':
    case '%':
    case '^':
    case '&':
    case '*':
    case '(':
    case ')':
    case '-':
    case '+':
    case '=':
    case '~':
    case '|':
    case '<':
    case '>':
    case '?':
    case ':':
    case '/':
    case ';':
    case ',':
    case '.':
        outBegin = inBegin;
        outEnd = inBegin + 1;
        return true;
    }

    return false;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::HLSL()
{
    static bool inited = false;
    static LanguageDefinition langDef;
    if (!inited)
    {
        static const char* const keywords[] = {
            "AppendStructuredBuffer", "asm", "asm_fragment", "BlendState", "bool", "break", "Buffer", "ByteAddressBuffer", "case", "cbuffer", "centroid", "class", "column_major", "compile", "compile_fragment",
            "CompileShader", "const", "continue", "ComputeShader", "ConsumeStructuredBuffer", "default", "DepthStencilState", "DepthStencilView", "discard", "do", "double", "DomainShader", "dword", "else",
            "export", "extern", "false", "float", "for", "fxgroup", "GeometryShader", "groupshared", "half", "Hullshader", "if", "in", "inline", "inout", "InputPatch", "int", "interface", "line", "lineadj",
            "linear", "LineStream", "matrix", "min16float", "min10float", "min16int", "min12int", "min16uint", "namespace", "nointerpolation", "noperspective", "NULL", "out", "OutputPatch", "packoffset",
            "pass", "pixelfragment", "PixelShader", "point", "PointStream", "precise", "RasterizerState", "RenderTargetView", "return", "register", "row_major", "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer",
            "RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D", "sample", "sampler", "SamplerState", "SamplerComparisonState", "shared", "snorm", "stateblock", "stateblock_state",
            "static", "string", "struct", "switch", "StructuredBuffer", "tbuffer", "technique", "technique10", "technique11", "texture", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture2DMS",
            "Texture2DMSArray", "Texture3D", "TextureCube", "TextureCubeArray", "true", "typedef", "triangle", "triangleadj", "TriangleStream", "uint", "uniform", "unorm", "unsigned", "vector", "vertexfragment",
            "VertexShader", "void", "volatile", "while",
            "bool1","bool2","bool3","bool4","double1","double2","double3","double4", "float1", "float2", "float3", "float4", "int1", "int2", "int3", "int4", "in", "out", "inout",
            "uint1", "uint2", "uint3", "uint4", "dword1", "dword2", "dword3", "dword4", "half1", "half2", "half3", "half4",
            "float1x1","float2x1","float3x1","float4x1","float1x2","float2x2","float3x2","float4x2",
            "float1x3","float2x3","float3x3","float4x3","float1x4","float2x4","float3x4","float4x4",
            "half1x1","half2x1","half3x1","half4x1","half1x2","half2x2","half3x2","half4x2",
            "half1x3","half2x3","half3x3","half4x3","half1x4","half2x4","half3x4","half4x4",
        };
        for (auto& k : keywords)
            langDef.keywords.insert(k);

        static const char* const identifiers[] = {
            "abort", "abs", "acos", "all", "AllMemoryBarrier", "AllMemoryBarrierWithGroupSync", "any", "asdouble", "asfloat", "asin", "asint", "asint", "asuint",
            "asuint", "atan", "atan2", "ceil", "CheckAccessFullyMapped", "clamp", "clip", "cos", "cosh", "countbits", "cross", "D3DCOLORtoUBYTE4", "ddx",
            "ddx_coarse", "ddx_fine", "ddy", "ddy_coarse", "ddy_fine", "degrees", "determinant", "DeviceMemoryBarrier", "DeviceMemoryBarrierWithGroupSync",
            "distance", "dot", "dst", "errorf", "EvaluateAttributeAtCentroid", "EvaluateAttributeAtSample", "EvaluateAttributeSnapped", "exp", "exp2",
            "f16tof32", "f32tof16", "faceforward", "firstbithigh", "firstbitlow", "floor", "fma", "fmod", "frac", "frexp", "fwidth", "GetRenderTargetSampleCount",
            "GetRenderTargetSamplePosition", "GroupMemoryBarrier", "GroupMemoryBarrierWithGroupSync", "InterlockedAdd", "InterlockedAnd", "InterlockedCompareExchange",
            "InterlockedCompareStore", "InterlockedExchange", "InterlockedMax", "InterlockedMin", "InterlockedOr", "InterlockedXor", "isfinite", "isinf", "isnan",
            "ldexp", "length", "lerp", "lit", "log", "log10", "log2", "mad", "max", "min", "modf", "msad4", "mul", "noise", "normalize", "pow", "printf",
            "Process2DQuadTessFactorsAvg", "Process2DQuadTessFactorsMax", "Process2DQuadTessFactorsMin", "ProcessIsolineTessFactors", "ProcessQuadTessFactorsAvg",
            "ProcessQuadTessFactorsMax", "ProcessQuadTessFactorsMin", "ProcessTriTessFactorsAvg", "ProcessTriTessFactorsMax", "ProcessTriTessFactorsMin",
            "radians", "rcp", "reflect", "refract", "reversebits", "round", "rsqrt", "saturate", "sign", "sin", "sincos", "sinh", "smoothstep", "sqrt", "step",
            "tan", "tanh", "tex1D", "tex1D", "tex1Dbias", "tex1Dgrad", "tex1Dlod", "tex1Dproj", "tex2D", "tex2D", "tex2Dbias", "tex2Dgrad", "tex2Dlod", "tex2Dproj",
            "tex3D", "tex3D", "tex3Dbias", "tex3Dgrad", "tex3Dlod", "tex3Dproj", "texCUBE", "texCUBE", "texCUBEbias", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc"
        };
        for (auto& k : identifiers)
        {
            Identifier id;
            id.declaration = "Built-in function";
            langDef.identifiers.insert(std::make_pair(std::string(k), id));
        }

        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", PaletteIndex::Preprocessor));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::CharLiteral));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

        langDef.commentStart = "/*";
        langDef.commentEnd = "*/";
        langDef.singleLineComment = "//";

        langDef.caseSensitive = true;
        langDef.autoIndentation = true;

        langDef.name = "HLSL";

        inited = true;
    }
    return langDef;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::GLSL()
{
    static bool inited = false;
    static LanguageDefinition langDef;
    if (!inited)
    {
        static const char* const keywords[] = {
            "attribute", "bool", "break", "bvec2", "bvec3", "bvec4",
            "case", "cast", "centroid", "const", "continue", "dmat2",
            "dmat2x2", "dmat2x3", "dmat2x4", "dmat3", "dmat3x2", "dmat3x3",
            "dmat3x4", "dmat4", "dmat4x2", "dmat4x3", "dmat4x4", "discard",
            "default", "do", "double", "dvec2", "dvec3", "dvec4", "else",
            "false", "flat", "float", "for", "highp", "if", "in", "inout", "int",
            "invariant", "isampler1D", "isampler1DArray", "isampler2D",
            "isampler2DArray", "isampler2DMS", "isampler2DMSArray", "isampler2DRect",
            "isampler3D", "isamplerBuffer", "isamplerCube", "isamplerCubeArray", "ivec2",
            "ivec3", "ivec4", "layout", "lowp", "mat2", "mat2x2", "mat2x3", "mat2x4",
            "mat3", "mat3x2", "mat3x3", "mat3x4", "mat4", "mat4x2", "mat4x3",
            "mat4x4", "mediump", "namespace", "noperspective", "out", "patch", "precision", "return",
            "sampler1D", "sampler1DArray", "sampler1DArrayShadow", "sampler1DShadow", "sampler2D",
            "sampler2DArray", "sampler2DArrayShadow", "sampler2DMS", "sampler2DMSArray", "sampler2DRect",
            "sampler2DRectShadow", "sampler2DShadow", "sampler3D", "samplerBuffer", "samplerCube",
            "samplerCubeArray", "samplerCubeArrayShadow", "samplerCubeShadow", "sizeof", "smooth",
            "struct", "subroutine", "switch", "true", "uimage1D", "uimage1DArray", "uimage2D",
            "uimage2DArray", "uimage2DMS", "uimage2DMSArray", "uimage2DRect", "uimage3D", "uimageBuffer",
            "uimageCube", "uimageCubeArray", "uint", "uniform", "usampler1D", "usampler1DArray",
            "usampler2D", "usampler2DArray", "usampler2DMS", "usampler2DMSArray", "usampler2DRect", "usampler3D",
            "usamplerBuffer", "usamplerCube", "usamplerCubeArray", "using", "uvec2", "uvec3", "uvec4",
            "varying", "vec2", "vec3", "vec4", "void", "while"
        };
        for (auto& k : keywords)
            langDef.keywords.insert(k);

        static const char* const identifiers[] = {
            "abs", "acos", "acosh", "all", "any", "asin", "asinh", "atan", "atanh",
            "ceil", "clamp", "cos", "cosh", "cross", "determinant", "degrees", "distance",
            "dot", "equal", "exp", "exp2", "faceforward", "floor", "fma", "fract", "frexp",
            "greaterThan", "greaterThanEqual", "inversesqrt", "length", "lessThan", "lessThanEqual",
            "log", "log2", "matrixCompMult", "max", "min", "mix", "mod", "modf", "normalize",
            "not", "notEqual", "outerProduct", "packSnorm2x16", "packSnorm4x8", "packUnorm2x16",
            "packUnorm4x8", "pow", "radians", "reflect", "refract", "round", "roundEven",
            "sign", "sin", "sinh", "smoothstep", "sqrt", "step", "tan", "tanh", "texelFetch",
            "texelFetchOffset", "texture", "textureGather", "textureGatherOffset",
            "textureGatherOffsets", "textureGrad", "textureGradOffset", "textureLod",
            "textureLodOffset", "textureProj", "textureProjGrad", "textureProjGradOffset",
            "textureProjLod", "textureProjLodOffset", "textureSize", "transpose",
            "trunc", "unpackSnorm2x16", "unpackSnorm4x8", "unpackUnorm2x16",
            "unpackUnorm4x8", "uaddCarry", "umulExtended", "usubBorrow"
        };
        for (auto& k : identifiers)
        {
            Identifier id;
            id.declaration = "Built-in function";
            langDef.identifiers.insert(std::make_pair(std::string(k), id));
        }

        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", PaletteIndex::Preprocessor));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::CharLiteral));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
        langDef.tokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

        langDef.commentStart = "/*";
        langDef.commentEnd = "*/";
        langDef.singleLineComment = "//";

        langDef.caseSensitive = true;
        langDef.autoIndentation = true;

        langDef.name = "GLSL";

        inited = true;
    }
    return langDef;
}

float TextEditor::getLineIndexColumnWidth() const
{
    static char buf[16];
    snprintf(buf, 16, " %d ", (int)lines_.size());
    return 
        ImGui::GetFont()->CalcTextSizeA
        (
            ImGui::GetFontSize(), 
            FLT_MAX, 
            -1.0f, 
            buf, 
            nullptr, 
            nullptr
        ).x + leftMargin_;
}

//----------------------------------------------------------------------------//

TextEditor::FindReplaceTool TextEditor::findReplaceTool_;

//----------------------------------------------------------------------------//

void TextEditor::FindReplaceTool::reset()
{
    mode_ = Mode::Find;
    isGuiOpen_ = false;
    isFocusOnSearchField_ = false;
    foundTextCounter_ = 0;
    foundTextCounter0_ = 0;
    textToBeFound_.clear();
    textToBeFound0_.clear();
    foundTextLineCols_.clear();
}

//----------------------------------------------------------------------------//

void TextEditor::FindReplaceTool::toggleGui(Mode mode)
{
    if (!isGuiOpen_)
    {
        isGuiOpen_ = true;
        mode_ = mode;
    }
    else if (mode_ == Mode::FindAndReplace)
    {
        if (mode == Mode::FindAndReplace)
            isGuiOpen_ = false;
        else 
            mode_ = Mode::Find;
    }
    else
    {
        if (mode == Mode::Find)
            isGuiOpen_ = false;
        else
            mode_ = Mode::FindAndReplace;
    }
}

//----------------------------------------------------------------------------//

bool TextEditor::FindReplaceTool::checkShortcuts()
{
    auto io = ImGui::GetIO();
    bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
    bool ctrlF(ctrl && ImGui::IsKeyPressed(ImGuiKey_F, false));
    bool ctrlH(ctrl && ImGui::IsKeyPressed(ImGuiKey_H, false));
    if (ctrlF || ctrlH)
    {
        toggleGui(ctrlF ? Mode::Find : Mode::FindAndReplace);
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------//

void TextEditor::FindReplaceTool::renderMenu()
{
    // These isFindOpen/isFindAndReplaceOpen flags are purely for aesthetic
    // purposes to enable checkmars showing correctly next to the menu items
    bool isFindOpen = isGuiOpen_ && mode_ == Mode::Find;
    bool isFindAndReplaceOpen = isGuiOpen_ && mode_ == Mode::FindAndReplace;

    bool find = ImGui::MenuItem("Find text", "Ctrl+F", &isFindOpen);
    bool findAndReplace = ImGui::MenuItem
    (
        "Find & replace text", 
        "Ctrl+H", 
        &isFindAndReplaceOpen
    );
    if (find || findAndReplace)
        toggleGui(find ? Mode::Find : Mode::FindAndReplace);
}

//----------------------------------------------------------------------------//

bool TextEditor::FindReplaceTool::render(TextEditor& editor)
{
    // Check if tool has been just opened or closed or if editor changed, and --
    // thus check it tool should run at all or if cache should be cleaned ------
    auto clearCache = [&]()
    {
        textToBeFound0_.clear();
        foundTextLineCols_.clear();
        foundTextCounter_ = 0;
        foundTextCounter0_ = foundTextCounter_;
        if (!isGuiOpen_)
        {
            editor.setHandleKeyboardInputs(true);
            editor.setSelection({0,0},{0,0});
        }
    };
    auto editorChanged = [](const TextEditor& editor)
    {
        static const TextEditor* editor0(&editor);
        if (&editor != editor0)
        {
            editor0 = &editor;
            return true;
        }
        return false;
    };
    bool isShortcutPressed = checkShortcuts();
    if (!isGuiOpen_)
    {
        if (isShortcutPressed)
            clearCache();
        return false; 
    }
    else if (editorChanged(editor))
        clearCache();

    // Render GUI --------------------------------------------------------------
    ImGui::Dummy(ImVec2(0, 0.05f*ImGui::GetFontSize()));
    float x0 = ImGui::GetCursorPosX();
    ImGui::Text("Find text ");
    ImGui::SameLine();
    
    bool searchedByClickingArrows(false);
    if (ImGui::SmallButton("<"))
    {
        foundTextCounter_ = std::max(foundTextCounter_-1, 0);
        searchedByClickingArrows = true;
    }
    ImGui::SameLine();
    int nFound(foundTextLineCols_.size());
    std::string counter
    (
        std::to_string
        (
            nFound > 0 ? 
            foundTextCounter_+1 : 
            foundTextCounter_
        )+"/"+std::to_string(nFound)
    );
    ImGui::Text(counter.c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton(">"))
    {
        foundTextCounter_ = std::min
        (
            foundTextCounter_+1, 
            nFound > 0 ? nFound-1 : 0
        );
        searchedByClickingArrows = true;
    }
    ImGui::SameLine();
    float x1 = ImGui::GetCursorPosX();
    ImGui::PushItemWidth(-1);

    if (isFocusOnSearchField_)
        ImGui::SetKeyboardFocusHere();
    bool searchedByPressingEnter = ImGui::InputText
    (
        "##findText", 
        &textToBeFound_,
        ImGuiInputTextFlags_NoUndoRedo | 
        ImGuiInputTextFlags_EnterReturnsTrue
        //ImGuiInputTextFlags_AllowTabInput
    );
    bool textToBeFoundChanged(textToBeFound_ != textToBeFound0_);
    if (textToBeFoundChanged)
        editor.setSelection({0,0},{0,0});
    if (searchedByPressingEnter)
    {
        foundTextCounter_+=1;
        if (foundTextCounter_ >= nFound > 0 ? nFound-1 : 0)
            foundTextCounter_ = 0;
    }
    isFocusOnSearchField_ = 
        isShortcutPressed        ||
        searchedByPressingEnter  || 
        searchedByClickingArrows ||
        textToBeFoundChanged;
    bool replaceText(false);
    if (mode_ == Mode::FindAndReplace)
    {
        replaceText = ImGui::Button
        (
            "Replace all with", 
            ImVec2(x1-x0-0.5*ImGui::GetFontSize(), 0)
        );
        ImGui::SameLine();
        ImGui::SetCursorPosX(x1);
        ImGui::InputText
        (
            "##replaceTextWith", 
            &replaceTextWith_, 
            ImGuiInputTextFlags_NoUndoRedo | 
            ImGuiInputTextFlags_EnterReturnsTrue
            //ImGuiInputTextFlags_AllowTabInput
        );
    }

    ImGui::PopItemWidth();
    ImGui::Dummy(ImVec2(0, 0.05f*ImGui::GetFontSize()));
    ImGui::Separator();

    // Actual text find/replace logic ------------------------------------------
    bool madeReplacements(false);
    if (isFocusOnSearchField_)
        editor.setHandleKeyboardInputs(false);
    else 
        editor.setHandleKeyboardInputs(true);
    int n(textToBeFound_.size());
    if (!replaceText)
    {
        if 
        (
            foundTextCounter_ != foundTextCounter0_ && 
            foundTextLineCols_.size() > 0
        )
        {
            foundTextCounter_ = std::min
            (
                foundTextCounter_, 
                (int)foundTextLineCols_.size()-1
            );
            auto lc = foundTextLineCols_[foundTextCounter_];
            Coordinates c0(lc.line, lc.column);
            Coordinates c1(lc.line, lc.column+n);
            editor.setCursorPosition(c0);
            editor.setSelection(c0, c1);
            foundTextCounter0_ = foundTextCounter_;
            return false;
        }
        else if (!textToBeFoundChanged)
        {
            if (textToBeFound_.size() == 0)
                clearCache();
            return false;
        }
    }
    else
    {
        if 
        (
            foundTextLineCols_.size() > 0 && 
            textToBeFound_ != replaceTextWith_
        )
        {
            Coordinates s0, s1;
            for (auto fcp : foundTextLineCols_)
            {
                s0 = {fcp.line, fcp.column};
                s1 = {fcp.line, fcp.column+n};
                editor.setSelection(s0,s1);
                editor.remove(true);
                editor.setCursorPosition(s0);
                editor.insertText(replaceTextWith_, true, true);
                if (!madeReplacements)
                    madeReplacements = true;
            }
            foundTextLineCols_.clear();
        }
    }
    std::string editorText = editor.getText();
    foundTextLineCols_.clear();
    nFound = 0;
    int line = 0;
    int column = 0;
    int range(editorText.size()-n+1);
    for (int i=0; i<range; i++)
    {
        bool found = true;
        char& ci0(editorText[i]);
        for (int j=0; j<n; j++)
        {
            char& ci(editorText[i+j]);
            char& cj(textToBeFound_[j]);
            if (cj != ci)
            {
                found = false;
                break;
            }
        }
        if (ci0 == '\n')
        {
            line+=1;
            column=0;
        }
        //else if (ci0 == '\t') // Got rid of tabs, so...
        //    column += editor.getTabSize();
        else
            column += 1;
        if (!found)
            continue;
        foundTextLineCols_.emplace_back
        (
            Coordinates{line, std::max(column-1, 0)}
        );
        nFound++;
    }
    if (nFound > 0)
    {
        auto lc = foundTextLineCols_[foundTextCounter_];
        Coordinates c0(lc.line, lc.column);
        Coordinates c1(lc.line, lc.column+n);
        editor.setCursorPosition(c0);
        editor.setSelection(c0, c1);
    }
    textToBeFound0_ = textToBeFound_;
    foundTextCounter_ = std::min(foundTextCounter_, nFound);
    foundTextCounter0_ = foundTextCounter_;
    return madeReplacements;
}

}