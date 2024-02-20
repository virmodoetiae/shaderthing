#pragma once

#include <vector>
#include "thirdparty/glm/glm.hpp"

namespace ShaderThing
{

namespace Helpers
{

bool isCtrlKeyPressed(ImGuiKey key);

bool isCtrlShiftKeyPressed(ImGuiKey key);

glm::vec2 normalizedWindowResolution();

std::string fileExtension(const std::string& filepath, bool toLowerCase=true);

// Returns the raw data of file at filepath and the overall data size. It is
// your responsibility to de-allocate the returned data, once used, with 
// delete[]
unsigned char* readFileContents
(
    const std::string& filepath,
    unsigned int& size
);

// Get the format string of a numeric value type such that:
// 1) if value < lowExpThreshold or value > highExpThreshold, the format will
//    consists of expDigits digits in exponential notation;
// 2) otherwise, the format will consist of floatDigits digits in regular
//    floating point notation
template<typename T>
std::string getFormat
(
    T value, 
    float lowExpThreshold=1e-3, 
    float highExpThreshold=1e3,
    unsigned int floatDigits=3,
    unsigned int expDigits=2
);

}

}