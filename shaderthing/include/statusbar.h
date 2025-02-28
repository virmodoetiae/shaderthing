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

#include <string>
#include <vector>

namespace ShaderThing
{

class StatusBar
{
private :
    struct Message
    {
        std::string  content;
        bool         isPersistent;
        float        duration = 1.f;
        // Two digits per channel (0-256 in hex, i.e., 00-ff), per 4 channels, 
        // namely transparency (alpha), blue, green, red, in that order
        unsigned int textColorABGR = 0xff00ffff; // Yellow
    };
    static std::vector<Message> messageQueue_;
    // Prevent any type of instantiation (the class itself only serves as a 
    // private namespace)
    StatusBar() = delete;
    StatusBar(const StatusBar&) = delete;
    StatusBar(StatusBar&&) = delete;
    StatusBar(int value) = delete;
    StatusBar& operator=(const StatusBar&) = delete;
    StatusBar& operator=(StatusBar&&) = delete;
    // Helpers
    static void queueMessage
    (
        const std::string& content,
        bool isPersistent, 
        float durationInSeconds,
        unsigned int textColorABGR
    );
public:
    static float defaultMessageDuration;
    static void renderGui(bool withSeparator = true);
    static void queueMessage
    (
        const std::string& content, 
        unsigned int textColorABGR = 0xff00ffff
    );
    static void queueTemporaryMessage
    (
        const std::string& content, 
        float durationInSeconds = defaultMessageDuration,
        unsigned int textColorABGR = 0xff00ffff
    );
    static void removeMessageFromQueue(const std::string& content);
    static void clearMessageQueue();
};

}