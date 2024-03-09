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

#include "data/about.h"
#include "data/data.h"
#include "resources/resource.h"
#include "thirdparty/stb/stb_image.h"

namespace ShaderThing
{

About::About():
isGuiOpen_(false),
isGuiInMenu_(true),
virmodoetiaeImage_(nullptr)
{
    virmodoetiaeImage_ = new Resource();
    if 
    (
        virmodoetiaeImage_->set
        (
            ImageData::virmodoetiae0Data, 
            ImageData::virmodoetiae0Size
        )
    ) return;
    delete virmodoetiaeImage_;
    virmodoetiaeImage_ = nullptr;
}

About::~About()
{
    if (virmodoetiaeImage_ == nullptr)
        return;
    delete virmodoetiaeImage_;
    virmodoetiaeImage_ = nullptr;
}

}