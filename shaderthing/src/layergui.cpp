/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2026 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "thirdparty/icons/IconsFontAwesome5.h"
#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_extensions.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

#include "vir/include/vir.h"

#include "shaderthing/include/app.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/sharedstorage.h"
#include "shaderthing/include/shareduniforms.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//

void Layer::renderMenuItemGui()
{
    if 
    (
        ImGui::BeginMenu
        (
            ("Layer ["+name_+"]###"+imGuiMenuId).c_str()
        )
    )
    {
        const float fontSize(ImGui::GetFontSize());
        const float entryWidth(14*fontSize);
        ImGui::Text("Name                 ");
        ImGui::SameLine();
        static std::unique_ptr<char[]> label(new char[24]);
        std::sprintf(label.get(), "##layer%dInputText", id);
        ImGui::PushItemWidth(entryWidth);
        if (ImGui::InputText(label.get(), &name_))
        {
            Helpers::enforceUniqueName
            (
                name_,
                app_.layers,
                this
            );
        }
        ImGui::PopItemWidth();
        
        static std::map<Layer::RenderState::Target, const char*> 
        renderTargetToName
        {
            {
                Layer::RenderState::Target::InternalFramebufferAndWindow, 
                "Framebuffer & window"
            },
            {Layer::RenderState::Target::InternalFramebuffer, "Framebuffer"},
            {Layer::RenderState::Target::Window, "Window"}
        };
        ImGui::Text("Render target        ");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        if 
        (
            ImGui::BeginCombo
            (
                "##renderingTarget", 
                renderTargetToName.at(renderState_.target)
            )
        )
        {
            for(auto entry : renderTargetToName)
            {
                if (!ImGui::Selectable(entry.second))
                    continue;
                auto target = entry.first;
                if (target != renderState_.target)
                {
                    renderState_.target = target;
                    auto thisWPtr = this->weakFromThis();
                    if 
                    (
                        renderState_.target != 
                        Layer::RenderState::Target::Window
                    )
                        app_.addLayerToResources(thisWPtr);
                    else
                        app_.removeLayerFromResources(thisWPtr);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        if 
        (
            renderState_.target == Layer::RenderState::Target::Window || 
            vir::Window::instance()->iconified()
        )
            ImGui::BeginDisabled();
        ImGui::Text("Resolution           ");
        ImGui::SameLine();
        auto x0 = ImGui::GetCursorPos().x;
        if 
        (
            ImGui::Button
            (
                isAspectRatioBoundToWindow_ ? 
                " " ICON_FA_LOCK " " : 
                " " ICON_FA_LOCK_OPEN " "
            )
        )
        {
            isAspectRatioBoundToWindow_ = isAspectRatioBoundToWindow_;
            if (isAspectRatioBoundToWindow_)
            {
                auto window = vir::Window::instance();
                setResolution({window->width(), window->height()}, false);
            }
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text(
                isAspectRatioBoundToWindow_ ?
ICON_FA_LOCK " - The aspect ratio is locked\n"
"to that of the main window" :
ICON_FA_LOCK_OPEN " - The aspect ratio is not locked\n"
"to that of the main window"
            );
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        auto aspectRatioLockSize = ImGui::GetCursorPos().x-x0;
        ImGui::PushItemWidth(entryWidth-aspectRatioLockSize);
        glm::ivec2 intResolution = resolution_;
        std::sprintf(label.get(), "##layer%dResolution", id);
        if (ImGui::InputInt2(label.get(), glm::value_ptr(intResolution)))
            setResolution(intResolution, false,true);
        ImGui::PopItemWidth();
        ImGui::Text("Auto-resize mode     ");
        ImGui::SameLine();
        if
        (
            ImGui::Button
            (
                rescaleWithWindow_ ?
                "Rescale on window resize" :
                "Do not auto-resize",
                {-1, 0}
            )
        )
            rescaleWithWindow_ = !rescaleWithWindow_;
        if 
        (
            renderState_.target == Layer::RenderState::Target::Window || 
            vir::Window::instance()->iconified()
        )
            ImGui::EndDisabled();
    
        if (renderState_.target != Layer::RenderState::Target::Window)
        {
            ImGui::SeparatorText("Framebuffer settings");
            renderFramebufferSettingsGui();

            // TODO
            /*
            ImGui::SeparatorText("Post-processing effects");
            int iDelete = -1;
            int iSrc = -1; 
            int iTrg = -1;
            int nPostProcesses = rendering.postProcesses.size();
            for (int i = 0; i < nPostProcesses; i++)
            {
                auto& postProcess = rendering.postProcesses[i];
                ImGui::PushID(i);
                if (ImGui::SmallButton(ICON_FA_TRASH))
                    iDelete = i;
                ImGui::SameLine();
                if (i == 0)
                    ImGui::BeginDisabled();
                if (ImGui::SmallButton(ICON_FA_ARROW_UP))
                {
                    iSrc = i;
                    iTrg = std::max(i-1, 0);
                }
                if (i == 0)
                    ImGui::EndDisabled();
                ImGui::SameLine();
                if (i == nPostProcesses-1)
                    ImGui::BeginDisabled();
                if (ImGui::SmallButton(ICON_FA_ARROW_DOWN))
                {
                    iSrc = i;
                    iTrg = std::min(i+1, nPostProcesses-1);
                }
                if (i == nPostProcesses-1)
                    ImGui::EndDisabled();
                ImGui::SameLine();
                if 
                (
                    ImGui::BeginMenu
                    (
                        std::string
                        (
                            std::to_string(i+1)+" - "+postProcess->name()
                        ).c_str()
                    )
                )
                {
                    // Render post-processing effect GUI
                    if (postProcess->canRunOnDeviceInUse())
                        postProcess->renderGui();
                    else
                    {
                        ImGui::PushTextWrapPos(40.0f*ImGui::GetFontSize());
                        ImGui::Text(postProcess->errorMessage().c_str());
                        ImGui::PopTextWrapPos();
                    }
                    ImGui::EndMenu();
                }
                ImGui::PopID();
            }
            if (iDelete != -1)
                rendering.postProcesses.erase
                (
                    rendering.postProcesses.begin() + iDelete
                );
            else if (iSrc != iTrg)
                std::swap
                (
                    rendering.postProcesses[iSrc], 
                    rendering.postProcesses[iTrg]
                );
            
            // Selector for adding a new post-processing effect with the 
            // constraint that each layer may have at most one 
            // tpost-processing effect of each ype
            ImGui::PushItemWidth(-1);
            if 
            (
                ImGui::BeginCombo
                (
                    "##postProcessingCombo", 
                    "Add a post-processing effect"
                )
            )
            {
                static std::vector<vir::PostProcess::Type> allAvailableTypes(0);
                if (allAvailableTypes.size() == 0)
                {
                    allAvailableTypes.reserve
                    (
                        vir::PostProcess::typeToName.size()
                    );
                    for (auto kv : vir::PostProcess::typeToName)
                    {
                        if (kv.first != vir::PostProcess::Type::Undefined)
                            allAvailableTypes.push_back(kv.first);
                    }
                }
                std::vector<vir::PostProcess::Type> 
                    availableTypes(allAvailableTypes);
                for (auto& postProcess : rendering.postProcesses)
                {
                    auto it = std::find
                    (
                        availableTypes.begin(), 
                        availableTypes.end(), 
                        postProcess->type()
                    );
                    if (it != availableTypes.end())
                        availableTypes.erase(it);
                }
                for (auto type : availableTypes)
                {
                    if 
                    (
                        ImGui::Selectable
                        (
                            vir::PostProcess::typeToName.at(type).c_str()
                        )
                    )
                    {
                        auto postProcess = 
                            PostProcess::create(this, type);
                        if (postProcess != nullptr)
                            rendering.postProcesses.emplace_back
                            (
                                std::move(postProcess)
                            );
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            */
        }
        ImGui::EndMenu();
    }
    //ImGui::PopID();
}

//----------------------------------------------------------------------------//

void Layer::renderFramebufferSettingsGui()
{
    const float entryWidth(14*ImGui::GetFontSize());
    ImGui::Text("Internal data format ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##layerInternalFormatCombo",
            vir::TextureBuffer::internalFormatToName.at
            (
                renderState_.backFramebuffer->
                    colorBufferInternalFormat()
            ).c_str()
        )
    )
    {
        static vir::TextureBuffer::InternalFormat 
        supportedInternalFormats[2]
        {
            vir::TextureBuffer::InternalFormat::RGBA_UNI_8, 
            vir::TextureBuffer::InternalFormat::RGBA_SF_32
        };
        for (auto internalFormat : supportedInternalFormats)
        {
            if 
            (
                ImGui::Selectable
                (
                    vir::TextureBuffer::internalFormatToName.at
                    (
                        internalFormat
                    ).c_str()
                )
            )
                rebuildFramebuffers(internalFormat, resolution_);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    //
    std::string selectedWrapModeX = "";
    std::string selectedWrapModeY = "";
    std::string selectedMagFilterMode = "";
    std::string selectedMinFilterMode = "";
    if (renderState_.backFramebuffer != nullptr)
    {
        selectedWrapModeX = vir::TextureBuffer::wrapModeToName.at
        (
            renderState_.backFramebuffer->colorBufferWrapMode(0)
        );
        selectedWrapModeY = vir::TextureBuffer::wrapModeToName.at
        (
            renderState_.backFramebuffer->colorBufferWrapMode(1)
        );
        selectedMagFilterMode = 
            vir::TextureBuffer::filterModeToName.at
            (
                renderState_.backFramebuffer->colorBufferMagFilterMode()
            );
        selectedMinFilterMode = 
            vir::TextureBuffer::filterModeToName.at
            (
                renderState_.backFramebuffer->colorBufferMinFilterMode()
            );
    }
    ImGui::Text("Horizontal wrap mode ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##layerWrapModeXCombo",
            selectedWrapModeX.c_str()
        ) && renderState_.backFramebuffer != nullptr
    )
    {
        for (auto entry : vir::TextureBuffer::wrapModeToName)
        {
            if (ImGui::Selectable(entry.second.c_str()))
                setFramebufferWrapMode(0, entry.first);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::Text("Vertical   wrap mode ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##layerWrapModeYCombo",
            selectedWrapModeY.c_str()
        ) && renderState_.backFramebuffer != nullptr
    )
    {
        for (auto entry : vir::TextureBuffer::wrapModeToName)
        {
            if (ImGui::Selectable(entry.second.c_str()))
                setFramebufferWrapMode(1, entry.first);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::Text("Magnification filter ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##layerMagModeCombo",
            selectedMagFilterMode.c_str()
        ) && renderState_.backFramebuffer != nullptr
    )
    {
        for (auto entry : vir::TextureBuffer::filterModeToName)
        {
            if 
            (
                entry.first != FilterMode::Nearest&&
                entry.first != FilterMode::Linear
            )
                continue;
            if (ImGui::Selectable(entry.second.c_str()))
                setFramebufferMagFilterMode(entry.first);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::Text("Minimization  filter ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##layerMinModeCombo",
            selectedMinFilterMode.c_str()
        ) && renderState_.backFramebuffer != nullptr
    )
    {
        for (auto entry : vir::TextureBuffer::filterModeToName)
        {
            if (ImGui::Selectable(entry.second.c_str()))
                setFramebufferMinFilterMode(entry.first);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
}

//----------------------------------------------------------------------------//

void Layer::renderTabGui()
{
    static unsigned int gActiveTabId = 0;
    static unsigned int gActiveLayerId = 0;
    bool layerChanged = (gActiveLayerId != id);
    if (layerChanged)
        gActiveLayerId = id;
    if (ImGui::BeginTabBar("##layerTabBar"))
    {
        if (layerChanged && activeGuiTabId_ != gActiveTabId)
        {
            switch (gActiveTabId)
            {
                case 0 :
                    ImGui::SetTabItemClosed("Shared source");
                    ImGui::SetTabItemClosed("Uniforms");
                    break;
                case 1 :
                    ImGui::SetTabItemClosed("Fragment source");
                    ImGui::SetTabItemClosed("Uniforms");
                    break;
                case 2 :
                    ImGui::SetTabItemClosed("Fragment source");
                    ImGui::SetTabItemClosed("Shared source");
                    break;
            }
        }
        if (ImGui::BeginTabItem("Fragment source"))
        {
            bool headerErrors(headerErrors_.size() > 0);
            bool madeReplacements = sourceEditor_.renderFindReplaceToolGui();
            if (ImGui::TreeNode("Header"))
            {
                float indent(sourceEditor_.getLineIndexColumnWidth());
                ImGui::Unindent(); // Remove indent from Header TreeNode
                ImGui::Indent(indent);
                ImGui::PushStyleColor
                (
                    ImGuiCol_Text, 
                    ImGui::GetStyle().Colors[ImGuiCol_TextDisabled] // Gray
                );
                ImGui::Text(sourceHeader_.c_str());
                ImGui::PopStyleColor(); 
                if 
                (
                    headerErrors && 
                    ImGui::IsItemHovered() && 
                    ImGui::BeginTooltip()
                )
                {
                    ImGui::PushTextWrapPos(40.0f*ImGui::GetFontSize());
                    ImGui::PushStyleColor(ImGuiCol_Text, {1,0,0,1}); // Red
                    ImGui::Text(headerErrors_.c_str());
                    ImGui::PopStyleColor();
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                ImGui::TreePop();
                ImGui::Separator();
                ImGui::Dummy(ImVec2(-1, ImGui::GetTextLineHeight()));
                ImGui::Unindent(indent);
                ImGui::Indent(); // Re-add indent from Header TreeNode
            }
            sourceEditor_.renderGui("##sourceEditor");
            bool madeEdits = sourceEditor_.isTextChanged();
            gActiveTabId = 0;
            ImGui::EndTabItem();
            hasUncompiledEdits = 
                hasUncompiledEdits || madeEdits || madeReplacements;
        }
        if (ImGui::BeginTabItem("Shared source"))
        {
            
            bool madeReplacements = 
                sharedSourceEditor_.renderFindReplaceToolGui();
            bool madeEdits = sharedSourceEditor_.isTextChanged();
            gActiveTabId = 1;
            ImGui::EndTabItem();
            hasUncompiledEdits = 
                hasUncompiledEdits || madeEdits || madeReplacements;
            sharedSourceEditor_.renderGui("##sharedSourceEditor");
        }
        if (ImGui::BeginTabItem("Uniforms"))
        {
            app_.renderUniformsTab(this);
            gActiveTabId = 2;
            ImGui::EndTabItem();
        }
        activeGuiTabId_ = gActiveTabId;
        ImGui::EndTabBar();
    }
}

//----------------------------------------------------------------------------//

void Layer::renderSharedCompilationErrorsGui()
{
    const auto& sharedSourceErrors(sharedSourceEditor_.getErrorMarkers());
    if (sharedSourceErrors.size() > 0)
    {
        ImGui::Bullet(); ImGui::Text("Shared source");
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            for (auto& error : sharedSourceErrors)
            {
                // First is line no., second is actual error text
                std::string errorText = 
                    "Line "+std::to_string(error.first)+": "+error.second;
                ImGui::Text(errorText.c_str());
            }
            ImGui::EndTooltip();
        }
    }
}

//----------------------------------------------------------------------------//

void Layer::renderCompilationErrorsGui()
{
    const auto& sourceErrors(sourceEditor_.getErrorMarkers());
    if (sourceErrors.size() > 0 || headerErrors_.size() > 0)
    {
        ImGui::Bullet(); ImGui::Text(name_.c_str());
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            if (headerErrors_.size() > 0)
                ImGui::Text(headerErrors_.c_str());
            for (auto& error : sourceErrors)
            {
                // First is line no., second is actual error text
                std::string errorText = 
                    "Line "+std::to_string(error.first)+": "+error.second;
                ImGui::Text(errorText.c_str());
            }
            ImGui::EndTooltip();
        }
    }
}

}