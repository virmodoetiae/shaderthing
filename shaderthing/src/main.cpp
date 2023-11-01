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

#if defined(__WIN32__)||defined(WIN32)||defined(_WIN32)||defined(__WIN32)
#include <windows.h>
#endif

#include "shaderthingapp.h"
#include "vir/include/vir.h"

int main()
{

// Hide console if running on Windows
#if (defined(__WIN32__)||defined(WIN32)||defined(_WIN32)||defined(__WIN32)) \
    && NDEBUG
    FreeConsole();
#endif
    ShaderThing::ShaderThingApp();
    return 0;
    
}
