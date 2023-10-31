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

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

#include <iostream>

namespace ShaderThing
{

void FindReplaceTextTool::renderGui()
{
    if (!isGuiOpen_)
    {
        if (isTextInFocus_)
            isTextInFocus_ = false;
        return;
    }
    //ImGui::SetCursorPos(pos);
    //ImGui::BeginChild("##findTool", size, true);
    ImGui::Dummy(ImVec2(0, 0.05f*ImGui::GetFontSize()));
    float x0 = ImGui::GetCursorPosX();
    ImGui::Text("Find text ");
    ImGui::SameLine();
    isArrowClicked_ = false;
    if (ImGui::SmallButton("<"))
    {
        foundTextCounter_ = std::max(foundTextCounter_-1, 0);
        focusOnText_ = true;
        isArrowClicked_ = true;
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
        focusOnText_ = true;
        isArrowClicked_ = true;
    }
    ImGui::SameLine();
    float x1 = ImGui::GetCursorPosX();
    ImGui::PushItemWidth(-1);
    if (focusOnText_)
        ImGui::SetKeyboardFocusHere(0);
    isEnterPressed_ = ImGui::InputText
    (
        "##findText", 
        &textToBeFound_,
        ImGuiInputTextFlags_NoUndoRedo | 
        ImGuiInputTextFlags_EnterReturnsTrue |
        ImGuiInputTextFlags_AllowTabInput
    );
    if (isEnterPressed_)
    {
        foundTextCounter_+=1;
        if (foundTextCounter_ >= nFound > 0 ? nFound-1 : 0)
            foundTextCounter_ = 0;
    }
    isTextInFocus_ = 
        ImGui::IsItemActive() || ImGui::IsItemFocused() || 
        isEnterPressed_ || focusOnText_ || 
        isArrowClicked_;
    
    if (isReplaceModeOn_)
    {
        replaceText_ = ImGui::Button
        (
            "Replace with", 
            ImVec2(x1-x0-0.5*ImGui::GetFontSize(), 0)
        );
        ImGui::SameLine();
        ImGui::SetCursorPosX(x1);
        ImGui::InputText
        (
            "##replaceTextWith", 
            &replaceTextWith_, 
            ImGuiInputTextFlags_NoUndoRedo | 
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_AllowTabInput
        );
    }

    ImGui::PopItemWidth();
    ImGui::Dummy(ImVec2(0, 0.05f*ImGui::GetFontSize()));
    //ImGui::EndChild();
}

//----------------------------------------------------------------------------//

void FindReplaceTextTool::renderGuiMenu()
{
    // These isFindOpen/isFindAndReplaceOpen flags are purely for aesthetic
    // purposes to enable checkmars showing correctly next to the menu items
    bool isFindOpen = isGuiOpen_ && !isReplaceModeOn_;
    bool isFindAndReplaceOpen = isGuiOpen_ && isReplaceModeOn_;
    
    bool find = ImGui::MenuItem("Find text", "Ctrl+F", &isFindOpen);
    bool findAndReplace = ImGui::MenuItem
    (
        "Find & replace text", 
        "Ctrl+H", 
        &isFindAndReplaceOpen
    );
    if (find || findAndReplace)
    {
        if (!isGuiOpen_)
        {
            isGuiOpen_ = true;
            if (findAndReplace)
                isReplaceModeOn_ = true;
            else 
                isReplaceModeOn_ = false;
        }
        else if (isReplaceModeOn_)
        {
            if (findAndReplace)
                isGuiOpen_ = false;
            else 
                isReplaceModeOn_ = false;
        }
        else
        {
            if (find)
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

}