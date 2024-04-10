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

#include <algorithm>
#include <cctype>
#include <charconv>
#include <ctime>

#include "shaderthing/include/helpers.h"

#include "vir/include/vir.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"

namespace ShaderThing
{

namespace Helpers
{

bool isCtrlKeyPressed(ImGuiKey key)
{
    return
    (
        ImGui::IsKeyDown(ImGuiKey_LeftCtrl) &&
        ImGui::IsKeyPressed(key, false)
    );
}

bool isCtrlShiftKeyPressed(ImGuiKey key)
{
    return
    (
        ImGui::IsKeyDown(ImGuiKey_LeftCtrl) &&
        ImGui::IsKeyDown(ImGuiKey_LeftShift) &&
        ImGui::IsKeyPressed(key, false)
    );
}

glm::vec2 normalizedWindowResolution()
{
    static const auto* window(vir::Window::instance());
    float windowAspectRatio = window->iconified() ? 1.f : window->aspectRatio();
    return
    {
        windowAspectRatio > 1.f ? 1.f : windowAspectRatio,
        windowAspectRatio < 1.f ? 1.0 : 1.0/windowAspectRatio
    };
}

unsigned int countNewLines(const std::string& text)
{
    unsigned int result(0);
    for (auto c : text)
        result += (unsigned int)(c == '\n');
    return result;
}

std::string fileExtension(const std::string& filepath, bool toLowerCase)
{
    std::string fileExtension = "";
    bool foundDot(false);
    for (int i=(int)filepath.size()-1; i>=0; i--)
    {
        const char& c(filepath[i]);
        if (c == '\\' || c == '/')
            break;
        if (!foundDot)
        {
            foundDot = (c == '.');
            fileExtension = c + fileExtension;
        }
        else break;
    }
    if (toLowerCase)
        std::transform
        (
            fileExtension.begin(), 
            fileExtension.end(), 
            fileExtension.begin(), 
            ::tolower
        );
    return foundDot ? fileExtension : "";
}

std::string filename(const std::string& filepath)
{
    std::string filename = "";
    bool foundSlash(false);
    for (int i=(int)filepath.size()-1; i>=0; i--)
    {
        const char& c(filepath[i]);
        foundSlash = (c == '\\' || c == '/');
        if (!foundSlash)
            filename = c + filename;
        else break;
    }
    return foundSlash ? filename : "";
}

void splitFilepath
(
    const std::string& filepath,
    std::string& filepathNoExtension,
    std::string& fileExtension
)
{
    filepathNoExtension.clear();
    fileExtension.clear();
    bool foundDot(false);
    int i = 0;
    for (i=(int)filepath.size()-1; i>=0; i--)
    {
        const char& c(filepath[i]);
        if (c == '\\' || c == '/')
            break;
        if (!foundDot)
        {
            foundDot = (c == '.');
            fileExtension = c + fileExtension;
        }
        else break;
    }
    if (foundDot)
        filepathNoExtension = filepath.substr(0, i+1);
    else
        filepathNoExtension = filepath;
}

std::string appendToFilename(const std::string& filepath, const std::string& s)
{
    std::string filepathNoExtension;
    std::string fileExtension;
    splitFilepath(filepath, filepathNoExtension, fileExtension);
    return filepathNoExtension+s+fileExtension;
}

std::string randomString(const unsigned int size)
{
    srand((unsigned)time(NULL));
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    auto tmp = std::string();
    tmp.reserve(size);
    for (unsigned int i = 0; i < size; ++i)
        tmp += alphanum[rand() % (sizeof(alphanum) - 1)];
    return tmp;
}

unsigned char* readFileContents
(
    const std::string& filepath,
    unsigned int& size
)
{
    std::ifstream dataStream(filepath, std::ios::binary | std::ios::in);
    dataStream.seekg(0, std::ios::end);
    size = dataStream.tellg();
    dataStream.seekg(0, std::ios::beg);
    unsigned char* data = new unsigned char[size];
    dataStream.read((char*)data, size);
    dataStream.close();
    return data;
}

std::string format(float value, unsigned int precision) 
{
    char buffer[8];
    auto result = std::to_chars
    (
        buffer, 
        buffer + sizeof(buffer), 
        value, 
        std::chars_format::fixed, 
        precision
    );
    if (result.ec == std::errc())
        return std::string(buffer, result.ptr - buffer);
    return "";
}

// This approach is a modification of mine to the one proposed here
// https://github.com/ocornut/imgui/issues/902#issuecomment-1103072284, which
// did not result in pixel-perfect alignment with other lines rendered with the
// traditional ImGui::Text(). This approach fixes this, but it is currently
// limited to one-line-only-text (not meant to be used with multi-line static
// text). I suspect It wouldn't be too difficult to extend, I simply don't
// need it for now
void oneLineColorfulText
(
    const std::string& text, 
    const std::vector<std::pair<char, ImVec4>>& colors
)
{
    auto p = ImGui::GetCursorScreenPos();
    auto im_colors = ImGui::GetStyle().Colors;
    const auto default_color = im_colors[ImGuiCol_Text];
    std::string temp_str;
    struct text_t 
    {
        ImVec4 color;
        std::string text;
    };
    std::vector<text_t> texts;
    bool color_time = false;
    ImVec4 last_color = default_color;
    int nControlChars(0);
    for (const auto& i : text) 
    {
        if (color_time) 
        {
            const auto& f = std::find_if
            (
                colors.begin(), 
                colors.end(), 
                [i](const auto& v) {return v.first == i;}
            );
            if (f != colors.end())
                last_color = f->second;
            else
                temp_str += i;
            color_time = false;
            continue;
        };
        switch (i) 
        {
        case '$':
            ++nControlChars;
            color_time = true;
            if (!temp_str.empty()) 
            {
                texts.push_back({last_color, temp_str});
                temp_str.clear();
            };
            break;
        default:
            temp_str += i;
        };
    };
    if (!temp_str.empty()) 
    {
        texts.push_back({ last_color, temp_str });
        temp_str.clear();
    };
    int nChars = text.size()-2*nControlChars;
    auto dummy = std::string(nChars, ' ');
    ImGui::Text(dummy.c_str());
    float dx = ImGui::CalcTextSize(dummy.c_str()).x/nChars;
    float x = p.x;
    for (int i=0; i<(int)texts.size(); i++)
    {
        auto& texti = texts[i];
        im_colors[ImGuiCol_Text] = texti.color;
        ImGui::RenderText(ImVec2(x, p.y), texti.text.c_str());
        x += texti.text.size()*dx;
    }
    im_colors[ImGuiCol_Text] = default_color;
}

#define RETURN_SCALAR_FORMAT                                            \
    if (value == 0)                                                     \
        return "%.1e";                                                  \
    floatDigits = floatDigits > 9 ? 9 : floatDigits;                    \
    expDigits = expDigits > 9 ? 9 : expDigits;                          \
    std::string format = "%.";                                          \
    if (abs(value) < lowExpThreshold || abs(value) > highExpThreshold)  \
        format += std::to_string(expDigits)+"e";                        \
    else                                                                \
        format += std::to_string(floatDigits)+"f";                      \
    return format;

#define RETURN_VECTOR_FORMAT(nCmpts)                                    \
    if (length(value) == 0)                                             \
        return "%.1e";                                                  \
    floatDigits = floatDigits > 9 ? 9 : floatDigits;                    \
    expDigits = expDigits > 9 ? 9 : expDigits;                          \
    std::string format = "%.";                                          \
    for (int i=0; i<nCmpts; i++)                                        \
    {                                                                   \
        float vi(std::abs(value[i]));                                   \
        if (vi >= lowExpThreshold && vi <= highExpThreshold)            \
            continue;                                                   \
        format += std::to_string(expDigits)+"e";                        \
        return format;                                                  \
    }                                                                   \
    format += std::to_string(floatDigits)+"f";                          \
    return format;

#define TYPED_GET_FORMAT_FUNC(type)                                     \
    template<>                                                          \
    std::string Helpers::getFormat                                      \
    (                                                                   \
        const type& value,                                              \
        float lowExpThreshold,                                          \
        float highExpThreshold,                                         \
        unsigned int floatDigits,                                       \
        unsigned int expDigits                                          \
    )

TYPED_GET_FORMAT_FUNC(unsigned int){return "%.0f";}

TYPED_GET_FORMAT_FUNC(int){return "%.0f";}

TYPED_GET_FORMAT_FUNC(float){RETURN_SCALAR_FORMAT}

TYPED_GET_FORMAT_FUNC(double){RETURN_SCALAR_FORMAT}

TYPED_GET_FORMAT_FUNC(glm::vec2){RETURN_VECTOR_FORMAT(2)}

TYPED_GET_FORMAT_FUNC(glm::vec3){RETURN_VECTOR_FORMAT(3)}

TYPED_GET_FORMAT_FUNC(glm::vec4){RETURN_VECTOR_FORMAT(4)}

}

}