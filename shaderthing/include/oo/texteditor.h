/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2025 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

/*
    This class is mostly a copy of the ImGuiColorTextEditor by Jako Balazs at
    https://github.com/BalazsJako/ImGuiColorTextEdit under the MIT license terms
    and conditions. The reason why this has been included in shaderthing instead
    of in the third-party libraries is a series of modifications I made:
   
    - integrated text find/replace tool (coded in TextEditor::FindReplaceTool);
    - auto-propagating undo/redo actions for text cut/copy/paste operations, as
      well as for text replace operations
    - removed tab character support, all tabs now always converted to spaces;
    - custom formatting behavior on pressing Tab;
    - custom horizontal scrolling behavior;
    - improved GLSL syntax highlighting;

    Most of these aspects were not additive in nature, and required to re-design
    certain aspects of the logic. Nonetheless, I want to stress that ~90% of the
    credit for this class goes to Jako Balazs. Please note that the overall
    class style (variable names, formatting, etc.) has been modified to match
    that of shaderthing.
*/

#pragma once

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <regex>
#include "thirdparty/imgui/imgui.h"

namespace ShaderThing
{

class TextEditor
{
public:
    enum class PaletteIndex
    {
        Default,
        Keyword,
        Number,
        String,
        CharLiteral,
        Punctuation,
        Preprocessor,
        Identifier,
        KnownIdentifier,
        PreprocIdentifier,
        Comment,
        MultiLineComment,
        Background,
        Cursor,
        Selection,
        ErrorMarker,
        Breakpoint,
        LineNumber,
        CurrentLineFill,
        CurrentLineFillInactive,
        CurrentLineEdge,
        Max
    };

    enum class SelectionMode
    {
        Normal,
        Word,
        Line
    };

    struct Breakpoint
    {
        int         line      = -1;
        bool        enabled   = false;
        std::string condition;
        Breakpoint():line(-1), enabled(false){}
    };

    // Represents a character coordinate from the user's point of view
    struct Coordinates
    {
        int line, column;
        Coordinates() : line(0), column(0) {}
        Coordinates(int aLine, int aColumn) : line(aLine), column(aColumn)
        {
            assert(aLine >= 0);
            assert(aColumn >= 0);
        }
        static Coordinates Invalid() 
        {
            static Coordinates invalid(-1, -1); 
            return invalid;
        }
        bool operator ==(const Coordinates& o) const
        {
            return
                line == o.line &&
                column == o.column;
        }
        bool operator !=(const Coordinates& o) const
        {
            return
                line != o.line ||
                column != o.column;
        }
        bool operator <(const Coordinates& o) const
        {
            if (line != o.line)
                return line < o.line;
            return column < o.column;
        }
        bool operator >(const Coordinates& o) const
        {
            if (line != o.line)
                return line > o.line;
            return column > o.column;
        }
        bool operator <=(const Coordinates& o) const
        {
            if (line != o.line)
                return line < o.line;
            return column <= o.column;
        }
        bool operator >=(const Coordinates& o) const
        {
            if (line != o.line)
                return line > o.line;
            return column >= o.column;
        }
    };

    struct Identifier
    {
        Coordinates location;
        std::string declaration;
    };

    typedef std::string String;
    typedef std::unordered_map<std::string, Identifier> Identifiers;
    typedef std::unordered_set<std::string> Keywords;
    typedef std::map<int, std::string> ErrorMarkers;
    typedef std::unordered_set<int> Breakpoints;
    typedef std::array<ImU32, (unsigned)PaletteIndex::Max> Palette;
    typedef uint8_t Char;

    struct Glyph
    {
        Char character;
        PaletteIndex colorIndex = PaletteIndex::Default;
        bool comment : 1;
        bool multiLineComment : 1;
        bool preprocessor : 1;

        Glyph(Char aChar, PaletteIndex aColorIndex) : 
            character(aChar), 
            colorIndex(aColorIndex),
            comment(false), 
            multiLineComment(false), 
            preprocessor(false) {}
    };

    typedef std::vector<Glyph> Line;
    typedef std::vector<Line> Lines;

    struct LanguageDefinition
    {
        typedef std::pair<std::string, PaletteIndex> TokenRegexString;
        typedef std::vector<TokenRegexString> TokenRegexStrings;
        typedef bool (*TokenizeCallback)
        (
            const char * inBegin, 
            const char * inEnd, 
            const char *& outBegin, 
            const char *& outEnd, 
            PaletteIndex & paletteIndex
        );

        std::string name;
        Keywords keywords;
        Identifiers identifiers;
        Identifiers preprocIdentifiers;
        std::string commentStart, commentEnd, singleLineComment;
        char preprocChar;
        bool autoIndentation;

        TokenizeCallback tokenize;

        TokenRegexStrings tokenRegexStrings;

        bool caseSensitive;

        LanguageDefinition() : 
            preprocChar('#'), 
            autoIndentation(true), 
            tokenize(nullptr), 
            caseSensitive(true)
        {
        }

        static const LanguageDefinition& GLSL();
    };

    class FindReplaceTool
    {
        enum class Mode
        {
            Find,
            FindAndReplace
        };

        Mode                     mode_                 = Mode::Find;
        bool                     isGuiOpen_            = false;
        bool                     isFocusOnSearchField_ = false;
        int                      foundTextCounter_     = 0;
        int                      foundTextCounter0_    = 0;
        std::string              textToBeFound_        = "";
        std::string              textToBeFound0_       = "";
        std::string              replaceTextWith_      = "";
        std::vector<Coordinates> foundTextLineCols_    = {};

        void toggleGui(Mode mode);

    public:

        bool                     forceSearch           = false;

        // Check if tool should open or close in response to pressing Ctrl+F or
        // Ctrl+H. Retruns true if said shortcuts have been pressed
        bool checkShortcuts();

        // Render menu bar with toggleable Find / Find & replace entries
        void renderMenuGui();
        
        void reset();

        // Runs open/close check logic, GUI render, and find/replace logic if
        // required
        bool renderGui(TextEditor& editor);
    };

    TextEditor();
    TextEditor(const std::string& initialText);

    void setLanguageDefinition(const LanguageDefinition& aLanguageDef);
    const LanguageDefinition& getLanguageDefinition() const 
    {
        return languageDefinition_;
    }

    const Palette& getPalette() const {return paletteBase_;}
    void setPalette(const Palette& aValue);

    void setErrorMarkers(const ErrorMarkers& aMarkers) 
    {
        errorMarkers_ = aMarkers;
    }
    const ErrorMarkers& getErrorMarkers() const {return errorMarkers_;}
    
    void setBreakpoints(const Breakpoints& aMarkers) {breakpoints_ = aMarkers;}

    void renderGui
    (
        const char* aTitle, 
        const ImVec2& aSize = ImVec2(), 
        bool aBorder = false
    );
    bool renderFindReplaceToolGui(){return findReplaceTool_.renderGui(*this);}
    static void renderFindReplaceToolMenuGui();

    void setText(const std::string& aText);
    std::string getText() const;

    void setTextLines(const std::vector<std::string>& aLines);
    std::vector<std::string> getTextLines() const;

    std::string getSelectedText() const;
    std::string getCurrentLineText()const;

    int getTotalLines() const { return (int)lines_.size(); }
    bool isOverwrite() const { return overwrite_; }

    void setReadOnly(bool aValue);
    bool isReadOnly() const { return readOnly_; }
    bool isTextChanged() const { return textChanged_; }
    void resetTextChanged() {textChanged_ = false;}
    bool isCursorPositionChanged() const {return cursorPositionChanged_;}

    bool isColorizerEnabled() const {return colorizerEnabled_;}
    void setColorizerEnable(bool aValue);

    Coordinates getCursorPosition() const {return getActualCursorCoordinates();}
    void setCursorPosition(const Coordinates& aPosition);

    inline void setHandleMouseInputs(bool aValue){handleMouseInputs_ = aValue;}
    inline bool isHandleMouseInputsEnabled() const {return handleMouseInputs_;}

    inline void setHandleKeyboardInputs(bool aValue)
    {
        handleKeyboardInputs_ = aValue;
    }
    inline bool isHandleKeyboardInputsEnabled() const 
    {
        return handleKeyboardInputs_;
    }

    inline void setImGuiChildIgnored(bool aValue){ignoreImGuiChild_ = aValue;}
    inline bool isImGuiChildIgnored() const {return ignoreImGuiChild_;}

    inline void setShowWhitespaces(bool aValue){showWhitespaces_ = aValue;}
    inline bool isShowingWhitespaces() const {return showWhitespaces_;}

    void setTabSize(int aValue);
    inline int getTabSize() const { return tabSize_; }

    void setTextStart(float aTextStart)
    {
        useSetTextStart_ = true; 
        textStart_ = aTextStart;
    }
    float getLineIndexColumnWidth() const;

    void insertText
    (
        const std::string& aValue,
        bool aRegisterUndo=false,
        bool aPropagateUndo=false
    );
    void insertText
    (
        const char* aValue,
        bool aRegisterUndo=false,
        bool aPropagateUndo=false
    );

    void moveUp(int aAmount = 1, bool aSelect = false);
    void moveDown(int aAmount = 1, bool aSelect = false);
    void moveLeft(int aAmount = 1, bool aSelect = false, bool aWordMode =false);
    void moveRight(int aAmount = 1, bool aSelect = false, bool aWordMode=false);
    void moveTop(bool aSelect = false);
    void moveBottom(bool aSelect = false);
    void moveHome(bool aSelect = false);
    void moveEnd(bool aSelect = false);

    void setSelectionStart(const Coordinates& aPosition);
    void setSelectionEnd(const Coordinates& aPosition);
    void setSelection
    (
        const Coordinates& aStart, 
        const Coordinates& aEnd, 
        SelectionMode aMode = SelectionMode::Normal
    );
    void selectWordUnderCursor();
    void selectAll();
    bool hasSelection() const;

    void copy();
    void cut();
    void paste();
    void remove(bool aPropagateUndo=false);

    bool canUndo() const;
    bool canRedo() const;
    void undo(int aSteps = 1);
    void redo(int aSteps = 1);

    static const Palette& getDarkPalette();
    static const Palette& getLightPalette();
    static const Palette& getRetroBluePalette();

private:
    
    typedef std::vector<std::pair<std::regex, PaletteIndex>> RegexList;

    struct EditorState
    {
        Coordinates selectionStart;
        Coordinates selectionEnd;
        Coordinates cursorPosition;
    };

    struct UndoRecord
    {
        UndoRecord() {}
        UndoRecord
        (
            const std::string& aAdded,
            const TextEditor::Coordinates aAddedStart,
            const TextEditor::Coordinates aAddedEnd,

            const std::string& aRemoved,
            const TextEditor::Coordinates aRemovedStart,
            const TextEditor::Coordinates aRemovedEnd,

            TextEditor::EditorState& aBefore,
            TextEditor::EditorState& aAfter
        );
        ~UndoRecord() {}

        void undo(TextEditor* aEditor);
        void redo(TextEditor* aEditor);

        bool        propagate = false;
        std::string added;
        Coordinates addedStart;
        Coordinates addedEnd;
        std::string removed;
        Coordinates removedStart;
        Coordinates removedEnd;
        EditorState before;
        EditorState after;
    };

    typedef std::vector<UndoRecord> UndoBuffer;

    void processInputs();
    void colorize(int aFroline = 0, int aCount = -1);
    void colorizeRange(int aFroline = 0, int aToLine = 0);
    void colorizeInternal();
    float textDistanceToLineStart(const Coordinates& aFrom) const;
    void ensureCursorVisible();
    int getPageSize() const;
    std::string getText
    (
        const Coordinates& aStart, 
        const Coordinates& aEnd
    ) const;
    Coordinates getActualCursorCoordinates() const;
    Coordinates sanitizeCoordinates(const Coordinates& aValue) const;
    void advance(Coordinates& aCoordinates) const;
    void deleteRange(const Coordinates& aStart, const Coordinates& aEnd);
    int insertTextAt(Coordinates& aWhere, const char* aValue, bool aRedo=false);
    void addUndo(UndoRecord& aValue);
    Coordinates screenPosToCoordinates(const ImVec2& aPosition) const;
    Coordinates findWordStartPos(const Coordinates& aFrom) const;
    Coordinates findWordEndPos(const Coordinates& aFrom) const;
    Coordinates findNextWordPos(const Coordinates& aFrom) const;
    int getCharacterIndex(const Coordinates& aCoordinates) const;
    int getCharacterColumn(int aLine, int aIndex) const;
    int getLineCharacterCount(int aLine) const;
    int getLineMaxColumn(int aLine) const;
    bool isOnWordBoundary(const Coordinates& aAt) const;
    void removeLine(int aStart, int aEnd);
    void removeLine(int aIndex);
    Line& insertLine(int aIndex);
    void enterCharacter(ImWchar aChar, bool aShift, bool aAddUndo=true);
    void backspace();
    void deleteSelection();
    std::string getWordUnderCursor() const;
    std::string getWordAt(const Coordinates& aCoords) const;
    ImU32 getGlyphColor(const Glyph& aGlyph) const;
    
    void handleKeyboardInputs();
    void handleMouseInputs();
    void renderGui();

    bool                   checkComments_         = true;
    bool                   colorizerEnabled_      = true;
    bool                   cursorPositionChanged_ = false;
    bool                   handleKeyboardInputs_  = true;
    bool                   handleMouseInputs_     = true;
    bool                   ignoreImGuiChild_      = false;
    bool                   overwrite_             = false;
    bool                   readOnly_              = false;
    bool                   scrollToCursor_        = false;
    bool                   scrollToTop_           = false;
    bool                   textChanged_;
    bool                   useSetTextStart_       = false;
    bool                   withinRender_          = false;
    bool                   showWhitespaces_       = true;
    int                    colorRangeMin_         = 0;
    int                    colorRangeMax_         = 0;
    int                    leftMargin_            = 10;
    int                    tabSize_               = 4;
    int                    undoIndex_             = 0;
    uint64_t               startTime_;
    float                  lastClick_             = -1.0f;
    float                  lineSpacing_           = 1.f;
    float                  textStart_             = 20.f;
    std::string            lineBuffer_;
    
    Breakpoints            breakpoints_;
    ImVec2                 charAdvance_;
    ErrorMarkers           errorMarkers_;
    static FindReplaceTool findReplaceTool_;
    Coordinates            interactiveStart_;
    Coordinates            interactiveEnd_;
    LanguageDefinition     languageDefinition_;
    Lines                  lines_;
    Palette                palette_;
    Palette                paletteBase_;
    RegexList              regexList_;
    SelectionMode          selectionMode_         = SelectionMode::Normal;
    EditorState            state_;
    UndoBuffer             undoBuffer_;
};

}
