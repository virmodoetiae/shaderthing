#include <algorithm>
#include "shaderthing-p/include/helpers.h"
#include "vir/include/vir.h"
#include "thirdparty/imgui/imgui.h"

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
    static const auto* window(vir::GlobalPtr<vir::Window>::instance());
    const float& windowAspectRatio(window->aspectRatio());
    return
    {
        windowAspectRatio > 1.f ? 1.f : windowAspectRatio,
        windowAspectRatio < 1.f ? 1.0 : 1.0/windowAspectRatio
    };
}

}

}