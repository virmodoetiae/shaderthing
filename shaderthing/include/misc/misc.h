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

#ifndef ST_MISC_H
#define ST_MISC_H

#include <string>
#include <ctime>
#include <vector>

#include "thirdparty/imgui/imgui.h"

namespace ShaderThing
{

namespace Misc
{

std::string randomString(int length);

template<typename T>
void enforceUniqueItemName(std::string& name, T* item, std::vector<T*>& items)
{
    if (name == "")
        name = randomString(6);
    // Replace all spaces with underscores. This is necessary because of
    // the current sscanf-based loading system for save files, and no
    // program names are currently allowed to have spaced. If I just
    // migrated my save file management to something like a e.g., JSON-
    // like structure, I would not have these issues...
    std::replace(name.begin(), name.end(), ' ', '_');
    std::string name0(name);
    int index(2);
    bool run(true);
    while(run)
    {
        run = false;
        for (T* itemi : items)
        {
            if (*item == *itemi) // Eeeh, maybe comparing ptrs is good too
                continue;
            if (itemi->name() == name)
            {
                if (!run) run = true;
                name = name0 + "(" + std::to_string(index)+")";
                ++index;
            }
        }
    }
}

template<typename T>
void enforceUniqueName(std::string& name, std::vector<T*>& items)
{
    if (name == "")
        name = randomString(6);
    // Replace all spaces with underscores. This is necessary because of
    // the current sscanf-based loading system for save files, and no
    // program names are currently allowed to have spaced. If I just
    // migrated my save file management to something like a e.g., JSON-
    // like structure, I would not have these issues...
    std::replace(name.begin(), name.end(), ' ', '_');
    std::string name0(name);
    int index(2);
    bool run(true);
    while(run)
    {
        run = false;
        for (T* itemi : items)
        {
            if (itemi->name() == name)
            {
                if (!run) run = true;
                name = name0 + "(" + std::to_string(index)+")";
                ++index;
            }
        }
    }
}

void OneLineColorfulText
(
    const std::string& text, 
    const std::vector<std::pair<char, ImVec4>>& colors = {}
);

bool isCtrlKeyPressed(ImGuiKey key);

bool isCtrlShiftKeyPressed(ImGuiKey key);

void limitWindowResolution(glm::ivec2& resolution);

}

}

#endif