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

#define RETURN_SCALAR_FORMAT                                            \
    if (value == 0)                                                     \
        return "%.1e";                                                  \
    floatDigits = floatDigits > 9 ? 9 : floatDigits;                    \
    expDigits = expDigits > 9 ? 9 : expDigits;                          \
    static char format[4] = {'%', '.', char(0), char(0)};               \
    if (abs(value) < lowExpThreshold || abs(value) > highExpThreshold)  \
        sprintf(&(format[2]), "%de", expDigits);                        \
    else                                                                \
        sprintf(&(format[2]), "%df", floatDigits);                      \
    return format;

#define RETURN_VECTOR_FORMAT(nCmpts)                                    \
    value = glm::abs(value);                                            \
    if (length(value) == 0)                                             \
        return "%.1e";                                                  \
    floatDigits = floatDigits > 9 ? 9 : floatDigits;                    \
    expDigits = expDigits > 9 ? 9 : expDigits;                          \
    static char format[4] = {'%', '.', char(0), char(0)};               \
    for (int i=0; i<nCmpts; i++)                                        \
    {                                                                   \
        const float& vi(value[i]);                                      \
        if (vi >= lowExpThreshold && vi <= highExpThreshold)            \
            continue;                                                   \
        sprintf(&(format[2]), "%de", expDigits);                        \
        return format;                                                  \
    }                                                                   \
    sprintf(&(format[2]), "%df", floatDigits);                          \
    return format;

#define TYPED_GET_FORMAT_FUNC(type)                                     \
    template<>                                                          \
    std::string Helpers::getFormat                                      \
    (                                                                   \
        type value,                                                     \
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