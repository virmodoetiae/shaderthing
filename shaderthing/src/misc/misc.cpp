#include "misc/misc.h"
#include <ctime>

#include "vir.h"
#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"

namespace ShaderThing
{

std::string Misc::randomString(int length)
{
    srand((unsigned)time(NULL));
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    auto tmp = std::string();
    tmp.reserve(length);
    for (int i = 0; i < length; ++i)
        tmp += alphanum[rand() % (sizeof(alphanum) - 1)];
    return tmp;
}

// This approach is a modification of mine to the one proposed here
// https://github.com/ocornut/imgui/issues/902#issuecomment-1103072284, which
// did not result in pixel-perfect alignment with other lines rendered with the
// traditional ImGui::Text(). This approach fixes this, but it is currently
// limited to one-line-only-text (not meant to be used with multi-line static
// text). I suspect It wouldn't be too difficult to implement, I simply don't
// need it, for now
void Misc::OneLineColorfulText
(
    const std::string& text, 
    const std::vector<std::pair<char, ImVec4>>& colors
) 
{
    auto p = ImGui::GetCursorScreenPos();
    const auto first_px = p.x, first_py = p.y;
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
    for (int i=0; i<texts.size(); i++)
    {
        auto& texti = texts[i];
        im_colors[ImGuiCol_Text] = texti.color;
        ImGui::RenderText(ImVec2(x, p.y), texti.text.c_str());
        x += texti.text.size()*dx;
    }
    im_colors[ImGuiCol_Text] = default_color;
};

bool Misc::isCtrlKeyPressed(ImGuiKey key)
{
    return
    (
        ImGui::IsKeyDown(ImGuiKey_LeftCtrl) &&
        ImGui::IsKeyPressed(key, false)
    );
}

bool Misc::isCtrlShiftKeyPressed(ImGuiKey key)
{
    return
    (
        ImGui::IsKeyDown(ImGuiKey_LeftCtrl) &&
        ImGui::IsKeyDown(ImGuiKey_LeftShift) &&
        ImGui::IsKeyPressed(key, false)
    );
}

void Misc::limitWindowResolution(glm::ivec2& resolution)
{
    static auto* window(vir::GlobalPtr<vir::Window>::instance());
    auto monitorScale = window->contentScale();
    glm::ivec2 minResolution = {120*monitorScale.x, 1};
    glm::ivec2 maxResolution = window->primaryMonitorResolution();
    resolution.x = 
        std::max(std::min(resolution.x, maxResolution.x), minResolution.x);
    resolution.y = 
        std::max(std::min(resolution.y, maxResolution.y), minResolution.y);
}

}