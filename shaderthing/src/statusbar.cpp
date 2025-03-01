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

#include <algorithm>

#include "shaderthing/include/statusbar.h"

#include "vir/include/vtime/vtime.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

namespace ShaderThing
{

std::vector<StatusBar::Message> StatusBar::messageQueue_ = {};
float                           StatusBar::defaultMessageDuration = 3.0f; // s

void StatusBar::renderGui(bool withSeparator)
{
    if (messageQueue_.empty())
        return;
    static char lBuffer[72];

    // The queue is rendered in order, each message is rendered for
    // its specified time duration
    auto& message = messageQueue_[0];

    message.duration -= vir::Time::instance()->outerTimestep();;
    if (message.duration < 0)
    {
        if (!message.isPersistent)
            messageQueue_.erase(messageQueue_.begin());
        else 
        {
            message.duration = StatusBar::defaultMessageDuration;
            if (messageQueue_.size() > 1) // Move to end of queue
                std::rotate
                (
                    messageQueue_.begin(), 
                    messageQueue_.begin() + 1, 
                    messageQueue_.end()
                );
        }
    }

    if (withSeparator)
        ImGui::Separator();
    // TODO
    // That '72' should actually be actually be adjusted based on the actual
    // horziontal available space (measured in numbers of characters), 
    // which can be somehow obtained via ImGui
    snprintf(lBuffer, 72, message.text.c_str());
    auto imGuiCursor = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddText
    (
        ImVec2
        (
            imGuiCursor.x,
            imGuiCursor.y
        ),
        message.textColorABGR,
        lBuffer
    );
}

void StatusBar::queueMessage
(
    const std::string& text,
    bool isPersistent, 
    float durationInSeconds,
    unsigned int textColorABGR
)
{
    // Only queue the message if it does not exist already
    if
    (
        std::find_if
        (
            messageQueue_.begin(),
            messageQueue_.end(),
            [&text](const Message& message)
            {
                return text == message.text;
            }
        ) == messageQueue_.end()
    )
    {
        // Actually, newly queued messages should be displayed first, so calling
        // this "queueMessage(...)" might be a bit misleading. Oh, well
        messageQueue_.insert
        (
            messageQueue_.begin(), 
            Message{text, isPersistent, durationInSeconds, textColorABGR}
        );
        // If the new message is inserted when the currently displayed message's
        // timer is close to running out, the newly inserted message will be
        // rendered, and the the previously displayed message will only be
        // rendered for its remaining duration, afterwards. If said duration is 
        // very short, the previous message will simply flash very briefly. This
        // is somewhat glitchy to see, so simply reset its timer
        if (messageQueue_.size() > 1)
        {
            messageQueue_[1].duration = StatusBar::defaultMessageDuration;
        }
    }
}

void StatusBar::queueMessage
(
    const std::string& text,
    unsigned int textColorABGR
)
{
    queueMessage(text, true, defaultMessageDuration, textColorABGR);
}

void StatusBar::queueTemporaryMessage
(
    const std::string& text,
    float duration,
    unsigned int textColorABGR
)
{
    queueMessage(text, false, duration, textColorABGR);
}

void StatusBar::removeMessageFromQueue
(
    const std::string& text
)
{
    auto it = std::find_if
    (
        messageQueue_.begin(),
        messageQueue_.end(),
        [&text](const Message& message)
        {
            return text == message.text;
        }
    );
    if(it != messageQueue_.end())
        messageQueue_.erase(it);
}

void StatusBar::clearMessageQueue()
{
    messageQueue_.clear();
}

}