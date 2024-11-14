#include <algorithm>

#include "shaderthing/include/statusbar.h"

#include "vir/include/vtime/vtime.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

namespace ShaderThing
{

std::string                              StatusBar::message_ = "";
std::vector<StatusBar::TemporaryMessage> StatusBar::temporaryMessageQueue_ = {};
unsigned int                             StatusBar::textColorABGR_ = 0xff00ffff;

void StatusBar::renderGui(unsigned int textColorABGR, bool withSeparator)
{
    static char lBuffer[48];
    const std::string* message = nullptr;
    if (!temporaryMessageQueue_.empty())
    {
        auto& item = temporaryMessageQueue_[0];
        item.duration -= vir::Time::instance()->outerTimestep();
        if (item.duration < 0)
            temporaryMessageQueue_.erase
            (
                temporaryMessageQueue_.begin()
            );
        else
            message = &(item.message);
    }
    else if (!message_.empty())
        message = &message_;
    if (message != nullptr)
    {
        if (withSeparator)
            ImGui::Separator();
        snprintf(lBuffer, 60, message->c_str());
        auto imGuiCursor = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddText
        (
            ImVec2
            (
                imGuiCursor.x,
                imGuiCursor.y
            ),
            textColorABGR,
            lBuffer
        );
    }
}

void StatusBar::renderGui(bool withSeparator)
{
    renderGui(textColorABGR_, withSeparator);
}

void StatusBar::setMessage(const std::string& message)
{
    message_ = message;
}

void StatusBar::clearMessage()
{
    message_.clear();
}

void StatusBar::queueTemporaryMessage
(
    const std::string& message, 
    unsigned int durationInSeconds
)
{
    // Only queue the message if it does not exist already
    if
    (
        std::find_if
        (
            temporaryMessageQueue_.begin(),
            temporaryMessageQueue_.end(),
            [&message](const TemporaryMessage& tmsgi)
            {
                return message == tmsgi.message;
            }
        ) == temporaryMessageQueue_.end()
    )
    {
        temporaryMessageQueue_.emplace_back
        (
            TemporaryMessage{message, float(durationInSeconds)}
        );
    }
}

void StatusBar::clearTemporaryMessageQueue()
{
    temporaryMessageQueue_.clear();
}

void StatusBar::setTextColor(unsigned int textColorABGR)
{
    textColorABGR_ = textColorABGR;
}

}