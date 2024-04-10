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

#include <conio.h>
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
    try
    {
        ShaderThing::App();
        return 0;
    }
    // Very miserable attempt at getting some info on crash, as running in
    // release without:
    // - any stack tracing library (e.g., boost::stacktrace)
    // - any form of a logging system in shaderthing and vir
    // leaves much to be desired
    catch (const std::exception& e)
    {
        // Show console again to print exception (if on Windows)
        #if (defined(WIN32) || defined(_WIN32)) && NDEBUG
            AllocConsole();
        #endif
        std::cout << "ShaderThing has crashed! :c" << std::endl;
        std::cout << "\n" << e.what() << std::endl;
        std::filesystem::path path = 
            std::filesystem::absolute("shaderthing_crash_log.txt");
        std::ofstream file(path);
        if (file.is_open()) 
        {
            file << e.what() << std::endl;
            file.close();
            std::cout
                << "\nException logged to " << path << " as well"
                << std::endl;
        }
        std::cout << "\nPress any key to continue..." << std::endl;
        getch();
        return 1;
    }
}