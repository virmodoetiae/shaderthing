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

#pragma once

#include <string>
#include <vector>
#include "thirdparty/glm/glm.hpp"

struct ImVec4;

namespace ShaderThing
{

namespace Helpers
{

bool isCtrlKeyPressed(ImGuiKey key);

bool isCtrlShiftKeyPressed(ImGuiKey key);

glm::vec2 normalizedWindowResolution();

unsigned int countNewLines(const std::string& text);

std::string fileExtension(const std::string& filepath, bool toLowerCase=true);

std::string filename(const std::string& filepath);

void splitFilepath
(
    const std::string& filepath, 
    std::string& filepathNoExtension, 
    std::string& fileExtension
);

std::string appendToFilename(const std::string& filepath, const std::string& s);

std::string randomString(const unsigned int size);

// Returns the raw data of file at filepath and the overall data size. It is
// your responsibility to de-allocate the returned data, once used, with 
// delete[]
unsigned char* readFileContents
(
    const std::string& filepath,
    unsigned int& size
);

std::string format(float value, unsigned int precision);

void oneLineColorfulText
(
    const std::string& text, 
    const std::vector<std::pair<char, ImVec4>>& colors = {}
);

//float parseComputeArrayExpression(const std::string& expression, float* array);

// Get the format string of a numeric value type such that:
// 1) if value < lowExpThreshold or value > highExpThreshold, the format will
//    consists of expDigits digits in exponential notation;
// 2) otherwise, the format will consist of floatDigits digits in regular
//    floating point notation
template<typename T>
std::string getFormat
(
    const T& value, 
    float lowExpThreshold=1e-3, 
    float highExpThreshold=1e3,
    unsigned int floatDigits=3,
    unsigned int expDigits=2
);

template<typename T>
void enforceUniqueName
(
    std::string& name, 
    const std::vector<T*>& items, 
    const T* skipItem=nullptr
)
{
    if (name == "")
        name = Helpers::randomString(6);
    std::replace(name.begin(), name.end(), ' ', '_');
    std::string name0(name);
    int index(2);
    bool run(true);
    while(run)
    {
        run = false;
        for (T* itemi : items)
        {
            if 
            (
                itemi->name() != name || 
                (skipItem != nullptr && skipItem == itemi)
            )
                continue;
            if (!run) 
                run = true;
            name = name0 + "(" + std::to_string(index)+")";
            ++index;
        }
    }
}

}

}