#include <algorithm>
#include "shaderthing-p/include/helpers.h"
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

}

}