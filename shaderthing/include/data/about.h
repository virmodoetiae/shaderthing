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

#ifndef ST_ABOUT_H
#define ST_ABOUT_H

#include <string>

namespace ShaderThing
{

class Resource;

class About
{
private:

    bool isGuiOpen_;
    bool isGuiInMenu_;
    Resource* virmodoetiaeImage_;

public:

    About();
    ~About();

    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}
    void renderGui();

    // Accessors
    bool* isGuiOpenPtr(){return &isGuiOpen_;}
    bool isGuiInMenu(){return isGuiInMenu_;}
};

}

#endif