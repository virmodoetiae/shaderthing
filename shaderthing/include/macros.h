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

#pragma once

#define NO_COPY(class)                                                      \
    class(const class&)=delete;                                             \
    class& operator=(const class&)=delete;

#define NO_COPY_MOVE(class)                                                 \
    class(const class&)=delete;                                             \
    class& operator=(const class&)=delete;                                  \
    class(class&&)=delete;                                                  \
    class& operator=(class&&)=delete;

#define DELETE_IF_NOT_NULLPTR(ptr) if (ptr!=nullptr) delete ptr; ptr=nullptr;
#define DELETE_ARRAY_IF_NOT_NULLPTR(ptr) if (ptr!=nullptr) delete[] ptr; ptr=nullptr;

// Use TO_STRING instead of this, this is just a helper
#define _TO_STRING(x) #x

#define TO_STRING(x) _TO_STRING(x)