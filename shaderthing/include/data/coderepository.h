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

#ifndef ST_CODE_REPOSITORY_H
#define ST_CODE_REPOSITORY_H

#include <string>

namespace ShaderThing
{

class CodeRepository
{
private:

    bool isGuiOpen_;
    bool isGuiInMenu_;

public:

    CodeRepository();

    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}
    void renderGui();

    // Accessors
    bool* isGuiOpenPtr(){return &isGuiOpen_;}
    bool isGuiInMenu(){return isGuiInMenu_;}
};

}

#endif