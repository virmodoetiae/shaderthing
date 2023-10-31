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

#ifndef ST_FIND_REPLACE_TEXT_TOOL_H
#define ST_FIND_REPLACE_TEXT_TOOL_H

#include <vector>
#include <string>

#include "thirdparty/imguitexteditor/imguitexteditor.h"

namespace ShaderThing
{

class FindReplaceTextTool
{
protected:

    struct FirstCharCoordinates
    {
        int line;
        int column;
        int index;
    };

    bool isGuiOpen_;
    bool isReplaceModeOn_;
    bool isTextInFocus_;
    bool isArrowClicked_;
    bool isEnterPressed_;
    bool focusOnText_;
    bool replaceText_;
    int foundTextCounter_;
    int foundTextCounter0_;
    std::string textToBeFound_;
    std::string textToBeFound0_;
    std::string replaceTextWith_;
    std::vector<FirstCharCoordinates> foundTextLineCols_;

public:

    FindReplaceTextTool();
    
    bool isGuiOpen(){return isGuiOpen_;}
    std::string& textToBeFound(){return textToBeFound_;}
    
    bool findReplaceTextInEditor(ImGuiExtd::TextEditor& editor);
    void reset();
    void clearCachedTextToBeFound(){textToBeFound0_="";}
    void renderGui();
    void renderGuiMenu();
    void update();
    
};

}

#endif