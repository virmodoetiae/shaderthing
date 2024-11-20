/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2024 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include <filesystem>
#if (defined(WIN32) || defined(_WIN32)) && NDEBUG
#include <windows.h>
#endif

#include "shaderthing/include/app.h"

int main()
{
    // Hide console if running on Windows
    #if (defined(WIN32) || defined(_WIN32)) && NDEBUG
        FreeConsole();
    #endif
    ShaderThing::App();
    return 0;
}
