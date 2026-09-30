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

bool Resource::renderActionsButtonGui
(
    UPtrVector<Resource>& resources, 
    DeferredActionBuffer& dab
)
{
    bool requestRecompilation = false;
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
        renderSettingsGui(resources);
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
        if 
        (
            type_ == Type::Texture2D &&
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) &&
            ((Texture2DResource*)this)->isUsedByOtherResources(resources)
        )
        {
            ImGui::BeginTooltip();
            ImGui::Text(
R"(These settings only affect this texture and do not 
affect any cubemaps or animations using this texture)");
            ImGui::EndTooltip();
        }
        if (ImGui::BeginPopup("##resourceSettings"))
        {
            requestRecompilation = renderSettingsGui(resources);
            ImGui::EndPopup();
        }
        renderReplaceButtonGui(resources, dab);
        ImGui::EndPopup();
    }
    return requestRecompilation;
}

//----------------------------------------------------------------------------//

bool Resource::renderSettingsGui(const UPtrVector<Resource>& resources)
{
    bool requestRecompilation = false;
    std::vector<const std::string*> clientResources;
    auto size = ImVec2(15*ImGui::GetFontSize(), 0);
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
    else
        size.x = -1;
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
        clientResources = texture->clientResourceNames(resources);
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
        if 
        (
            texture->rawData() == nullptr ||
            texture->rawDataSize() == 0
        )
        {
            bool disabled = clientResources.size() > 0;
            if (disabled)
                ImGui::BeginDisabled();
            requestRecompilation = texture->renderResizeOrReformatGui();
            if (disabled)
                ImGui::EndDisabled();
        }
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
        requestRecompilation = texture->renderResizeOrReformatGui();
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
    return requestRecompilation;
}

//----------------------------------------------------------------------------//

bool LayerResource::renderSettingsGui(const UPtrVector<Resource>& resources)
{
    layer_->renderFramebufferSettingsGui();
    return false;
}

//----------------------------------------------------------------------------//

template<typename T>
bool TextureNDResource<T>::EditorGuiData::renderInternalFormatSelectorGui()
{
    bool edited = false;
    // RGB formats are not easy to work with due to memory-alignment 
    // limitations so they are omitted
    static constexpr InternalFormat supportedFormats[] = 
    {
        InternalFormat::R_UI_32,    InternalFormat::R_SF_32,
        InternalFormat::RG_UI_32,   InternalFormat::RG_SF_32,
        InternalFormat::RGBA_UI_32, InternalFormat::RGBA_SF_32
    };
    const auto& names = vir::TextureBuffer2D::internalFormatToName;
    if 
    (
        ImGui::BeginCombo
        (
            "##tndrFormat", 
            names.at(internalFormat).c_str()
        )
    )
    {
        for (auto format : supportedFormats)
            if 
            (
                ImGui::Selectable
                (
                    names.at(format).c_str(), 
                    format == internalFormat
                )
            )
            {
                internalFormat = format;
                edited = true;
            }
        ImGui::EndCombo();
    }
    return edited;
}

//----------------------------------------------------------------------------//

template<typename T>
bool TextureNDResource<T>::renderEditorGui(EditorGuiData& egd, int labelWidth)
{
    bool edited = false;
    ImGui::Text("%-*s", labelWidth, "Resolution");
    ImGui::SameLine();
    edited = egd.renderResolutionEditorGui();
    ImGui::Text("%-*s", labelWidth, "Format");
    ImGui::SameLine();
    edited = egd.renderInternalFormatSelectorGui();
    return edited;
}

//----------------------------------------------------------------------------//

template<typename T>
bool TextureNDResource<T>::renderResizeOrReformatGui()
{
    if (ImGui::IsWindowAppearing())
        edg_.reset(*this);
    renderEditorGui(edg_, 20);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("VRAM footprint      ");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("VRAM memory occupied by this texture");
    ImGui::SameLine();
    double footprint = this->maxMemoryFootprint();
    auto uom = Helpers::autoRescaleMemoryValue(footprint);
    ImGui::Text("%.1f %s", footprint, uom);
    bool unedited = edg_.isUnedited(*this);
    bool requestRecompilation = false;
    ImGui::BeginDisabled(unedited);
    if (ImGui::Button("Resize or reformat", ImVec2(-1, 0)))
    {
        auto wrapMode0 = this->wrapMode(0);
        auto wrapMode1 = this->wrapMode(1);
        auto wrapMode2 = this->wrapMode(2);
        auto minFilterMode = this->minFilterMode();
        auto magFilterMode = this->magFilterMode();
        auto format0 = this->internalFormat();
        this->set(edg_);
        this->setWrapMode(0, wrapMode0);
        this->setWrapMode(1, wrapMode1);
        this->setWrapMode(2, wrapMode2);
        this->setMinFilterMode(minFilterMode);
        this->setMagFilterMode(magFilterMode);
        requestRecompilation = 
            this->internalFormat() != format0; //&& clientUniforms_.size() > 0;
        edg_.reset(*this);
    }
    if (!unedited)
        Helpers::renderTextureMemoryEstimateGui
        (
            edg_.nPixels(), 
            edg_.internalFormat, 
            true
        );
    ImGui::EndDisabled();
    return requestRecompilation;
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

void Texture2DResource::EditorGuiData::reset
(
    TextureNDResource<vir::TextureBuffer2D>& ref
)
{
    resolution = {ref.width(), ref.height()};
    internalFormat = ref.internalFormat();
}

//----------------------------------------------------------------------------//

bool Texture2DResource::EditorGuiData::isUnedited
(
    TextureNDResource<vir::TextureBuffer2D>& ref
)
{
    return resolution == glm::ivec2{ref.width(), ref.height()} && 
        internalFormat == ref.internalFormat();
}

//----------------------------------------------------------------------------//

bool Texture2DResource::EditorGuiData::renderResolutionEditorGui(int maxSize)
{
    bool modified = false;
    if (ImGui::InputInt2("##t2drResolution", glm::value_ptr(resolution)))
    {
        resolution = glm::clamp(resolution, 1, maxSize);
        modified = true;
    }
    return modified;
}

//----------------------------------------------------------------------------//

bool Texture2DResource::set
(
    TextureNDResource<vir::TextureBuffer2D>::EditorGuiData& edg
)
{
    auto cedg = (Texture2DResource::EditorGuiData*)&edg;
    return this->set
    (
        cedg->resolution.x, cedg->resolution.y, edg.internalFormat
    );
}

//----------------------------------------------------------------------------//

void AnimatedTexture2DResource::renderReplaceButtonGui
(
    UPtrVector<Resource>& resources, 
    DeferredActionBuffer& dab
)
{
    if (ImGui::Button("Replace", ImVec2(-1, 0)))
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

void Texture3DResource::EditorGuiData::reset
(
    TextureNDResource<vir::TextureBuffer3D>& ref
)
{
    resolution = {ref.width(), ref.height(), ref.depth()};
    internalFormat = ref.internalFormat();
}

//----------------------------------------------------------------------------//

bool Texture3DResource::EditorGuiData::isUnedited
(
    TextureNDResource<vir::TextureBuffer3D>& ref
)
{
    return resolution == glm::ivec3{ref.width(), ref.height(), ref.depth()} && 
        internalFormat == ref.internalFormat();
}

//----------------------------------------------------------------------------//

bool Texture3DResource::EditorGuiData::renderResolutionEditorGui(int maxSize)
{
    if (ImGui::InputInt3("##t3drResolution", glm::value_ptr(resolution)))
    {
        resolution = glm::clamp(resolution, 1, maxSize);
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------//

bool Texture3DResource::set
(
    TextureNDResource<vir::TextureBuffer3D>::EditorGuiData& edg
)
{
    auto cedg = (Texture3DResource::EditorGuiData*)&edg;
    return this->set
    (
        cedg->resolution.x, cedg->resolution.y, cedg->resolution.z, 
        edg.internalFormat
    );
}

}

template class ShaderThing::TextureNDResource<vir::TextureBuffer2D>;
template class ShaderThing::TextureNDResource<vir::TextureBuffer3D>;