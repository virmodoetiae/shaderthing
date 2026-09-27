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
#include "shaderthing/include/filedialog.h"
#include "shaderthing/include/helpers.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/resource.h"
#include "shaderthing/include/uniform.h"

namespace ShaderThing
{

void Resource::renderActionsButtonGui
(
    UPtrVector<Resource>& resources, 
    DeferredActionBuffer& dab
)
{
    if (type_ == Resource::Type::Framebuffer)
    {
        if (ImGui::Button(ICON_FA_COG, ImVec2(-1,0)))
            ImGui::OpenPopup("##framebufferResourceSettings");
    }
    else
    {
        if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1,0)))
            ImGui::OpenPopup("##resourceActions");
    }

    if (ImGui::BeginPopup("##framebufferResourceSettings"))
    {
        renderSettingsGui();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##resourceActions"))
    {
        auto size = ImVec2(12*ImGui::GetFontSize(), 0);
        if (ImGui::Button(ICON_FA_TRASH, size))
        {
            dab.add
            (
                [this, &resources]()
                {
                    auto r = 
                        std::find(resources.begin(), resources.end(), this);
                    resources.erase(r);
                }
            );
        }
        if (ImGui::Button(ICON_FA_COG, size))
        {
            ImGui::OpenPopup("##resourceSettings");
        }
        if (ImGui::BeginPopup("##resourceSettings"))
        {
            renderSettingsGui();
            ImGui::EndPopup();
        }
        renderReplaceButtonGui(resources, dab);
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------//

void Resource::renderSettingsGui()
{
    auto size = ImVec2(12*ImGui::GetFontSize(), 0);
    if (type_ == Resource::Type::AnimatedTexture2D)
    {
        auto animation = 
            (AnimatedTexture2DResource*)this;
        auto nativeAnimation = animation->native();
        unsigned int frameIndex = nativeAnimation->frameIndex();
        unsigned int nFrames = nativeAnimation->nFrames();
        ImGui::Text("Animation frame     ");
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::Button
        (   
            std::to_string(frameIndex+1).c_str(),
            ImVec2(size.x/3.25, 0)
        );
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::Text("/ %d", nFrames);
        ImGui::Text("Animation timing    ");
        ImGui::SameLine();
        if 
        (
            ImGui::Button
            (
                animation->isAnimationBoundToGlobalTime ?
                "Bound to iTime" :
                "Manual control",
                size
            )
        )
        {
            animation->isAnimationBoundToGlobalTime = 
                !animation->isAnimationBoundToGlobalTime;
        }
        if (animation->isAnimationBoundToGlobalTime)
            ImGui::BeginDisabled();
        ImGui::Text("Animation controls  ");
        ImGui::SameLine();
        if 
        (
            ImGui::Button
            (
                ICON_FA_STEP_BACKWARD, 
                ImVec2(size.x/3.25, 0)
            )
        )
            nativeAnimation->previousFrame(); // Step backwards
        ImGui::SameLine();
        if 
        (
            ImGui::Button
            (
                animation->isAnimationPaused ? 
                ICON_FA_PLAY : 
                ICON_FA_PAUSE,
                ImVec2(size.x/3.25, 0)
            )
        )
            animation->isAnimationPaused = 
                !animation->isAnimationPaused;
        ImGui::SameLine();
        if 
        (
            ImGui::Button
            (
                ICON_FA_STEP_FORWARD, 
                ImVec2(-1, 0)
            )
        )
            nativeAnimation->nextFrame();
        if (animation->isAnimationBoundToGlobalTime)
            ImGui::EndDisabled();
        ImGui::Text("Animation FPS       ");
        ImGui::SameLine();
        float fps(nativeAnimation->fps());
        ImGui::PushItemWidth(size.x);
        if 
        (
            ImGui::DragFloat
            (
                "##animationFpsDragFloat",
                &fps,
                0.1f,
                0.1f,
                160.0f,
                "%.1f"
            )
        )
        {
            fps = std::min(std::max(0.1f, fps), 1000.f);
            nativeAnimation->setFps(fps);
        }
        ImGui::Text("Animation duration  ");
        ImGui::SameLine();
        float duration(nativeAnimation->duration());
        ImGui::PushItemWidth(size.x);
        if 
        (
            ImGui::DragFloat
            (
                "##animationDurationDragFloat",
                &duration,
                0.1f,
                nFrames/1000.0f,
                nFrames/.1f,
                "%.3f"
            )
        )
        {
            fps = nFrames/duration;
            nativeAnimation->setFps(fps);
        }
        ImGui::PopItemWidth();
        ImGui::Separator();
    }
    std::string selectedWrapModeX = "";
    std::string selectedWrapModeY = "";
    std::string selectedMagFilterMode = "";
    std::string selectedMinFilterMode = "";
    selectedWrapModeX=vir::TextureBuffer::wrapModeToName.at
    (
        wrapMode(0)
    );
    selectedWrapModeY=vir::TextureBuffer::wrapModeToName.at
    (
        wrapMode(1)
    );
    selectedMagFilterMode = 
        vir::TextureBuffer::filterModeToName.at
        (
            magFilterMode()
        );
    selectedMinFilterMode = 
        vir::TextureBuffer::filterModeToName.at
        (
            minFilterMode()
        );
    if (type_ != Resource::Type::Cubemap)
    {
        ImGui::Text("Texture wrap mode H ");
        ImGui::SameLine();
        ImGui::PushItemWidth(size.x);
        if 
        (
            ImGui::BeginCombo
            (
                "##bufferWrapModeXCombo",
                selectedWrapModeX.c_str()
            )
        )
        {
            for (auto e:vir::TextureBuffer::wrapModeToName)
                if (ImGui::Selectable(e.second.c_str()))
                    setWrapMode(0, e.first);
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::Text("Texture wrap mode V ");
        ImGui::SameLine();
        ImGui::PushItemWidth(size.x);
        if 
        (
            ImGui::BeginCombo
            (
                "##bufferWrapModeYCombo",
                selectedWrapModeY.c_str()
            )
        )
        {
            for (auto e:vir::TextureBuffer::wrapModeToName)
                if (ImGui::Selectable(e.second.c_str()))
                    setWrapMode(1, e.first);
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
    }
    ImGui::Text("Texture mag. filter ");
    ImGui::SameLine();
    ImGui::PushItemWidth(size.x);
    if 
    (
        ImGui::BeginCombo
        (
            "##bufferMagModeCombo",
            selectedMagFilterMode.c_str()
        )
    )
    {
        for (auto e : vir::TextureBuffer::filterModeToName)
        {
            // Mipmap filters are for min only,
            // no effect on mag, hence they 
            // are skipped here
            if
            (
                e.first != 
                vir::TextureBuffer::FilterMode::Nearest &&
                e.first != 
                vir::TextureBuffer::FilterMode::Linear
            )
                continue;
            if (ImGui::Selectable(e.second.c_str()))
                setMagFilterMode(e.first);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::Text("Texture min. filter ");
    ImGui::SameLine();
    ImGui::PushItemWidth(size.x);
    if 
    (
        ImGui::BeginCombo
        (
            "##bufferMinModeCombo",
            selectedMinFilterMode.c_str()
        )
    )
    {
        for (auto e : vir::TextureBuffer::filterModeToName)
        {
            if 
            (
                isInternalFormatUnsigned() &&
                e.first != FilterMode::Linear && 
                e.first != FilterMode::Nearest
            )
                continue;
            if (ImGui::Selectable(e.second.c_str()))
                setMinFilterMode(e.first);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    bool usesMipmap = 
        minFilterMode() != FilterMode::Nearest &&
        minFilterMode() != FilterMode::Linear;
    if (type_ == Resource::Type::Texture2D)
    {
        auto texture = (Texture2DResource*)this;
        if (usesMipmap)
        {
            ImGui::Text("Auto mipmap update  ");
            ImGui::SameLine();
            ImGui::Checkbox
            (
                "##autoMipmapUpdate", 
                &(texture->autoUpdateMipmap)
            );
            if 
            (
                !texture->autoUpdateMipmap && 
                ImGui::Button("Update mipmap", ImVec2(-1, 0))
            )
                texture->updateMipmap();
        }
        /*if 
        (
            texture->rawData() == nullptr ||
            texture->rawDataSize() == 0
        )
        {
            bool disabled = inUseByCubemapOrAnimation.size() > 0;
            if (disabled)
                ImGui::BeginDisabled();
            createOrResizeOrReformatTexture2DGui
            (
                resource,
                false,
                settingsOpened
            );
            if (disabled)
                ImGui::EndDisabled();
        }*/
    }
    else if (type_ == Resource::Type::Texture3D)
    {
        auto texture = (Texture3DResource*)this;
        if (usesMipmap)
        {
            ImGui::Text("Auto mipmap update  ");
            ImGui::SameLine();
            ImGui::Checkbox
            (
                "##autoMipmapUpdate", 
                &(texture->autoUpdateMipmap)
            );
            if 
            (
                !texture->autoUpdateMipmap && 
                ImGui::Button("Update mipmap", ImVec2(-1, 0))
            )
                texture->updateMipmap();
        }
        /*createOrResizeOrReformatTexture3DGui
        (
            resource,
            false,
            settingsOpened
        );*/
    }
    else if 
    (
        type_ == Resource::Type::AnimatedTexture2D &&
        usesMipmap
    )
    {
        ImGui::Text("Auto mipmap update  ");
        ImGui::SameLine();
        ImGui::Checkbox
        (
            "##autoMipmapUpdate", 
            &(((AnimatedTexture2DResource*)this)->autoUpdateMipmap)
        );
        // No manual mipmap update button for animation-type
        // resources because it just does not make much sense (also,
        // the way it's currently implemented in vir, it only 
        // updates the current animation frame)
    }
}

//----------------------------------------------------------------------------//

void LayerResource::renderSettingsGui()
{
    layer_->renderFramebufferSettingsGui();
}

//----------------------------------------------------------------------------//

void Texture2DResource::renderReplaceButtonGui
(
    UPtrVector<Resource>& resources, 
    DeferredActionBuffer& dab
)
{
    auto size = ImVec2(12*ImGui::GetFontSize(), 0);
    if (ImGui::Button("Replace", size))
    {
        Resource::fileDialog.runOpenFileDialog
        (
            "Select an image",
            {
                "Image files (.png,.jpg,.jpeg,.bmp)", 
                "*.png *.jpg *.jpeg *.bmp"
            },
            "."
        );
        dab.add
        (
            [this, &resources]()
            {
                auto filepath = 
                    Resource::fileDialog.selection().front();
                dynamic_cast<Texture2DResource*>(this)->
                    set(filepath);
                std::string name = Helpers::filename(filepath);
                Helpers::enforceUniqueName
                (
                    name, 
                    resources, 
                    (Resource*)this
                );
                setName(name);
            },
            []() -> bool
            {
                return Resource::fileDialog.validSelection();
            }
        );
    }
}

//----------------------------------------------------------------------------//

void AnimatedTexture2DResource::renderReplaceButtonGui
(
    UPtrVector<Resource>& resources, 
    DeferredActionBuffer& dab
)
{
    auto size = ImVec2(12*ImGui::GetFontSize(), 0);
    if (ImGui::Button("Replace", size))
    {
        Resource::fileDialog.runOpenFileDialog
        (
            "Select a GIF",
            {
                "Image files (.gif)", 
                "**.gif"
            },
            "."
        );
        dab.add
        (
            [this, &resources]()
            {
                auto filepath = 
                    Resource::fileDialog.selection().front();
                dynamic_cast<AnimatedTexture2DResource*>(this)->
                    set(filepath);
                std::string name = Helpers::filename(filepath);
                Helpers::enforceUniqueName
                (
                    name, 
                    resources, 
                    (Resource*)this
                );
                setName(name);
            },
            []() -> bool
            {
                return Resource::fileDialog.validSelection();
            }
        );
    }
}

//----------------------------------------------------------------------------//

}