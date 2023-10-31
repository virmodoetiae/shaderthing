/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a, virmodoetiae).
|  |\  \|\__    __\   |  For more information, visit:
|  \ \  \|__|\  \_|   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \|__|\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2023 Stefan Radman
|  Ↄ|C    \|__|\|__|  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "tools/findreplacetexttool.h"

#include <iostream>

namespace ShaderThing
{

//----------------------------------------------------------------------------//
// Public functions ----------------------------------------------------------//

FindReplaceTextTool::FindReplaceTextTool() :
isGuiOpen_(false),
isReplaceModeOn_(false),
isTextInFocus_(false),
isArrowClicked_(false),
isEnterPressed_(false),
focusOnText_(false),
replaceText_(false),
foundTextCounter_(0),
foundTextCounter0_(0),
textToBeFound_(""),
textToBeFound0_(""),
replaceTextWith_(""),
foundTextLineCols_(0){}

//----------------------------------------------------------------------------//

void FindReplaceTextTool::reset()
{
    isGuiOpen_ = false;
    isReplaceModeOn_ = false;
    isTextInFocus_ = false;
    isArrowClicked_ = false;
    isEnterPressed_ = false;
    focusOnText_ = false;
    foundTextCounter_ = 0;
    foundTextCounter0_ = 0;
    textToBeFound_ = "";
    textToBeFound0_ = "";
    foundTextLineCols_.clear();
}

//----------------------------------------------------------------------------//

void FindReplaceTextTool::update()
{
    bool isFPressed(ImGui::IsKeyPressed(ImGuiKey_F, false));
    bool isHPressed(ImGui::IsKeyPressed(ImGuiKey_H, false));
    if // Ctrl+F to open finder tool
    (
        ImGui::IsKeyDown(ImGuiKey_LeftCtrl) &&
        (isFPressed || isHPressed) &&
        ImGui::GetIO().WantCaptureKeyboard
    )
    {
        if (!isGuiOpen_)
        {
            isGuiOpen_ = true;
            if (isHPressed)
                isReplaceModeOn_ = true;
            else 
                isReplaceModeOn_ = false;
        }
        else if (isReplaceModeOn_)
        {
            if (isHPressed)
                isGuiOpen_ = false;
            else 
                isReplaceModeOn_ = false;
        }
        else
        {
            if (isFPressed)
                isGuiOpen_ = false;
            else
                isReplaceModeOn_ = true;
        }
        focusOnText_ = isGuiOpen_;
    }
    else if (!(isEnterPressed_ || isArrowClicked_))
        focusOnText_ = false;
    else if (isGuiOpen_)
        focusOnText_ = true;
}

//----------------------------------------------------------------------------//

bool FindReplaceTextTool::findReplaceTextInEditor
(
    ImGuiExtd::TextEditor& editor
)
{
    bool madeReplacements(false);
    if (isTextInFocus_)
        editor.SetHandleKeyboardInputs(false);
    else 
        editor.SetHandleKeyboardInputs(true);
    if (!isGuiOpen_)
    {
        if (foundTextLineCols_.size() > 0)
        {
            foundTextLineCols_.clear();
            foundTextCounter_ = 0;
            foundTextCounter0_ = foundTextCounter_;
        }
        return false;
    }
    int n(textToBeFound_.size());
    if (!replaceText_)
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
            ImGuiExtd::TextEditor::Coordinates c0(lc.line, lc.column);
            ImGuiExtd::TextEditor::Coordinates c1(lc.line, lc.column+n);
            editor.SetCursorPosition(c0);
            editor.SetSelection(c0, c1);
            foundTextCounter0_ = foundTextCounter_;
            return false;
        }
        else if (textToBeFound_ == "" || textToBeFound_ == textToBeFound0_)
        {
            if (textToBeFound_ == "")
            {
                if (foundTextLineCols_.size() > 0)
                {
                    foundTextLineCols_.clear();
                    foundTextCounter_ = 0;
                    foundTextCounter0_ = foundTextCounter_;
                }
            }
            return false;
        }
    }
    else
    {
        if 
        (
            foundTextLineCols_.size() > 0 && textToBeFound_ != replaceTextWith_
        )
        {
            ImGuiExtd::TextEditor::Coordinates s0, s1;
            for (auto fcp : foundTextLineCols_)
            {
                s0 = {fcp.line, fcp.column};
                s1 = {fcp.line, fcp.column+n};
                editor.SetSelection(s0,s1);
                editor.Delete(true);
                editor.SetCursorPosition(s0);
                editor.InsertText(replaceTextWith_, true, true);
                if (!madeReplacements)
                    madeReplacements = true;
            }
            foundTextLineCols_.clear();
        }
        replaceText_ = false;
    }
    std::string editorText = editor.GetText();
    foundTextLineCols_.clear();
    int nFound(0);
    int line(0);
    int column(0);
    for (int i=0; i<editorText.size()-n; i++)
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
        else if (ci0 == '\t')
            column += editor.GetTabSize();
        else
            column += 1;
        if (!found)
            continue;
        foundTextLineCols_.emplace_back
        (
            FirstCharCoordinates{line, column-1, i}
        );
        nFound++;
    }
    if (nFound > 0)
    {
        auto lc = foundTextLineCols_[foundTextCounter_];
        ImGuiExtd::TextEditor::Coordinates c0(lc.line, lc.column);
        ImGuiExtd::TextEditor::Coordinates c1(lc.line, lc.column+n);
        editor.SetCursorPosition(c0);
        editor.SetSelection(c0, c1);
    }
    textToBeFound0_ = textToBeFound_;
    foundTextCounter_ = std::min(foundTextCounter_, nFound);
    foundTextCounter0_ = foundTextCounter_;
    return madeReplacements;
}

}