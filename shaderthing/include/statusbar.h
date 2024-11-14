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

namespace ShaderThing
{

class StatusBar
{
private :
    struct TemporaryMessage
    {
        std::string message;
        float duration;
    };
    static std::string                   message_;
    static std::vector<TemporaryMessage> temporaryMessageQueue_;
    static unsigned int                  textColorABGR_;
    // Prevent any type of instantiation (the class itself only serves as a 
    // private namespace)
    StatusBar() = delete;
    StatusBar(const StatusBar&) = delete;
    StatusBar(StatusBar&&) = delete;
    StatusBar(int value) = delete;
    StatusBar& operator=(const StatusBar&) = delete;
    StatusBar& operator=(StatusBar&&) = delete;
public:
    static void renderGui(bool withSeparator = true);
    static void renderGui
    (
        unsigned int oneTimeTextColorABGR, 
        bool withSeparator = true
    );
    static void setMessage(const std::string& message);
    static void clearMessage();
    static void queueTemporaryMessage
    (
        const std::string& message, 
        unsigned int durationInSeconds
    );
    static void clearTemporaryMessageQueue();
    static void setTextColor(unsigned int textColorABGR);
};

}