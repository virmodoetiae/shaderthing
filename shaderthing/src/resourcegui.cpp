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
        bool disableDelete = type_ == Type::Texture2D && 
            ((Texture2DResource*)this)->clientResources().size() > 0;
        if (disableDelete)
            ImGui::BeginDisabled();
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
        if (disableDelete)
        {
            ImGui::EndDisabled();
            ((Texture2DResource*)this)->renderClientResourceLock();
        }
        if (ImGui::Button(ICON_FA_COG, size))
        {
            ImGui::OpenPopup("##resourceSettings");
        }
        if 
        (
            type_ == Type::Texture2D &&
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) &&
            ((Texture2DResource*)this)->clientResources().size() > 0
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
    //else
    //    size.x = -1;
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
        auto clientResources = texture->clientResources();
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
    const UPtrVector<Resource>& resources, 
    DeferredActionBuffer& dab
)
{
    auto size = ImVec2(12*ImGui::GetFontSize(), 0);
    if (clientResources_.size() > 0)
        ImGui::BeginDisabled();
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
    if (clientResources_.size() > 0)
    {
        ImGui::EndDisabled();
        renderClientResourceLock();
    }
}

//----------------------------------------------------------------------------//

void Texture2DResource::renderClientResourceLock()
{
    if 
    (
        ImGui::IsItemHovered
        (
            ImGuiHoveredFlags_AllowWhenDisabled
        ) && ImGui::BeginTooltip()
    )
    {
        std::string hoverText = 
"This texture is in use by the following resources:\n";
        for (int i=0; i<(int)clientResources_.size(); i++)
            hoverText += 
                "  "+std::to_string(i+1)+") "+
                clientResources_[i]->name()+"\n";
        hoverText += 
"To delete or replace this texture, first delete \n"
"or update the resources which use it";
        ImGui::Text(hoverText.c_str());
        ImGui::EndTooltip();
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

bool AnimatedTexture2DResource::renderEditorButtonGui
(
    EditorGuiData& egd, 
    const UPtrVector<Resource>& resources,
    const std::string& label
)
{
    bool buttonPressed = false;
    ImGui::SeparatorText("Animation frames");
    int nFrames = egd.unmanagedFrames.size();
    if (egd.frameResolution == glm::uvec2{0u, 0u} && nFrames == 1)
        egd.frameResolution = 
        {
            egd.unmanagedFrames[0]->width(),
            egd.unmanagedFrames[0]->height()
        };
    static bool reordered(false);
    int iFrameToBeDeleted = -1;
    auto renumber = [&egd]()
    {
        for (size_t k = 0; k < egd.unmanagedFrames.size(); k++)
            egd.orderedUnmanagedFrameNames[k] =
                std::to_string(k+1) + " - " + egd.unmanagedFrames[k]->name();
    };
    ImGui::BeginChild
    (
        "##framesChild", 
        ImVec2
        (
            ImGui::GetContentRegionAvail().x, 
            std::min
            (
                (float)std::max(nFrames, 1), 
                15.f
            )*
            ImGui::GetTextLineHeightWithSpacing()
        ), 
        false
    );
    for (int i=0; i<nFrames; i++)
    {
        ImGui::PushID(i);
        if (ImGui::SmallButton(ICON_FA_TRASH))
            iFrameToBeDeleted = i;
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::Selectable
        (
            egd.orderedUnmanagedFrameNames[i].c_str(),
            false,
            ImGuiSelectableFlags_DontClosePopups
        );
        if (ImGui::IsItemActive())
        {
            float mouseY = ImGui::GetMousePos().y;
            int dir =
                mouseY < ImGui::GetItemRectMin().y ? -1 :
                mouseY > ImGui::GetItemRectMax().y ? +1 : 0;
            int j = i + dir;
            if (dir != 0 && j >= 0 && j < nFrames)
            {
                std::swap(egd.unmanagedFrames[i], egd.unmanagedFrames[j]);
                std::swap
                (
                    egd.orderedUnmanagedFrameNames[i],
                    egd.orderedUnmanagedFrameNames[j]
                );
                reordered = true;
            }
        }
    }
    if (reordered && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        renumber();
        reordered = false;
    }
    ImGui::EndChild();
    if (iFrameToBeDeleted != -1)
    {
        egd.unmanagedFrames.erase
        (
            egd.unmanagedFrames.begin() + iFrameToBeDeleted
        );
        egd.orderedUnmanagedFrameNames.erase
        (
            egd.orderedUnmanagedFrameNames.begin() + iFrameToBeDeleted
        );
        renumber();
    }
    if
    (
        ImGui::BeginCombo
        (
            "##addAnimationFrameCombo",
            "Select frame to add"
        )
    )
    {
        for (auto& r : resources)
        {
            if 
            (
                r->type() != Resource::Type::Texture2D ||
                (
                    egd.frameResolution.x * egd.frameResolution.y > 0 && 
                    (
                        r->width() != egd.frameResolution.x ||
                        r->height() != egd.frameResolution.y
                    )
                )
            )
                continue;
            if (ImGui::Selectable(r->name().c_str()))
            {
                egd.unmanagedFrames.emplace_back
                (
                    r.dynamicDowncastTo<Texture2DResource>().getWeak())
                ;
                egd.orderedUnmanagedFrameNames.emplace_back
                (
                    std::to_string(egd.unmanagedFrames.size()) +
                    " - " + r->name()
                );
            }
        }
        ImGui::EndCombo();
    }
    if (nFrames == 0)
        ImGui::BeginDisabled();
    if (ImGui::Button(label.c_str(), ImVec2(-1,0)))
        buttonPressed = true;
    if (nFrames == 0)
        ImGui::EndDisabled();

    return buttonPressed;
}

//----------------------------------------------------------------------------//

void AnimatedTexture2DResource::renderReplaceButtonGui
(
    const UPtrVector<Resource>& resources, 
    DeferredActionBuffer& dab
)
{
    bool fromFile = unmanagedFrames_.size() == 0;
    if (ImGui::Button(fromFile ? "Replace" : "Edit", ImVec2(-1, 0)))
    {
        // If this animation was loaded from a file, render the file selector
        if (fromFile)
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
        else // if this animation is created by hand, render the editor
        {
            ImGui::OpenPopup("##editAnimationPopup");
            editorGuiData_.frameResolution = {0u, 0u};
            editorGuiData_.unmanagedFrames = unmanagedFrames_;
            int nFrames = unmanagedFrames_.size();
            editorGuiData_.orderedUnmanagedFrameNames.resize(nFrames);
            for (int i=0; i<nFrames; i++)
                editorGuiData_.orderedUnmanagedFrameNames[i] = 
                    std::to_string(i+1) + " - " +
                    unmanagedFrames_[i]->name();
        }
    }
    if (ImGui::BeginPopup("##editAnimationPopup"))
    {
        if (renderEditorButtonGui(editorGuiData_, resources, "Edit"))
        {
            dab.add
            (
                [this, &resources]()
                {
                    set(editorGuiData_.unmanagedFrames);
                    editorGuiData_.unmanagedFrames.clear();
                    editorGuiData_.orderedUnmanagedFrameNames.clear();
                }
            );
        }
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------//

bool CubemapResource::renderEditorButtonGui
(
    EditorGuiData& egd, 
    const UPtrVector<Resource>& resources,
    const std::string& label
)
{
    static std::string labels[6] = 
    {
        "X+  ", "X-  ", "Y+  ", "Y-  ", "Z+  ", "Z-  "
    };
    int textureResourcei(0);
    int nSelectedTextureResources(0);
    for (int i=0; i<6; i++)
    {
        if (egd.selectedTextureResources[i] != nullptr)
        {
            nSelectedTextureResources++;
            textureResourcei = i;
        }
    }
    bool buttonPressed = false;
    bool validFaces = true;
    float buttonSize(ImGui::GetFontSize()*15.0);
    for (int i=0; i<6; i++)
    {
        ImGui::Text(labels[i].c_str());
        ImGui::SameLine();
        std::string selectedTextureResourceName = 
            egd.selectedTextureResources[i] != nullptr ?
            egd.selectedTextureResources[i]->name() : "";
        ImGui::PushItemWidth(-1);
        std::string comboi = 
            "##cubeMapFaceResourceSelector"+std::to_string(i);
        if 
        (
            ImGui::BeginCombo
            (
                comboi.c_str(), 
                selectedTextureResourceName.c_str()
            )
        )
        {
            for(int j=0; j<(int)resources.size()+1 ;j++)
            {
                if (j==0)
                {
                    if (ImGui::Selectable("-"))
                    {
                        egd.selectedTextureResources[i] = 
                            WPtr<Texture2DResource>();
                        if (nSelectedTextureResources == 1)
                        {
                            egd.faceResolution = {0, 0};
                        }
                    }
                    continue;
                }
                else if (resources[j-1]->type() != Resource::Type::Texture2D)
                    continue;
                auto r = 
                    resources[j-1].dynamicDowncastTo<Texture2DResource>()
                    .getWeak();
                if 
                (
                    !vir::CubeMapBuffer::validFace(r->native())
                )
                    continue;
                if 
                (
                    egd.faceResolution.x != 0 && 
                    egd.faceResolution.y != 0 && 
                    (
                        r->width() != egd.faceResolution.x || 
                        r->height() != egd.faceResolution.y
                    ) &&
                    !(
                        nSelectedTextureResources == 1 && 
                        i == textureResourcei
                    )
                )
                    continue;
                if (ImGui::Selectable(r->name().c_str()))
                {
                    egd.selectedTextureResources[i] = r;
                    egd.faceResolution = {r->width(), r->height()};
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        if (egd.selectedTextureResources[i] == nullptr)
            validFaces = false;
    }
    if (!validFaces)
    {
        
        ImGui::BeginDisabled();
        ImGui::Button(label.c_str(), ImVec2(buttonSize, 0));
        ImGui::EndDisabled();
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text(
R"(To generate a cube map, select a texture from the 
loaded Texture2D resources for each of the 6 faces 
of the cubemap. Said resources need to: 
    1) have a square aspect ratio; 
    2) have the same resolution; 
    3) have a resoluton which is a power of 2. 
The available textures are automatically filtered 
among those loaded in the resource manager.)");
            ImGui::EndTooltip();
        }
    }
    else if (ImGui::Button(label.c_str(), ImVec2(buttonSize, 0)))
    {
        buttonPressed = true;
    }
    return buttonPressed;
}

//----------------------------------------------------------------------------//

void CubemapResource::renderReplaceButtonGui
(
    const UPtrVector<Resource>& resources, 
    DeferredActionBuffer& dab
)
{
    if (ImGui::Button("Edit", ImVec2(-1, 0)))
    {
        ImGui::OpenPopup("##editCubemapPopup");
        editorGuiData_.faceResolution = {0, 0};
        for (int i=0;i<6;i++)
            editorGuiData_.selectedTextureResources[i] = 
                unmanagedFaces_[i].getWeak();
    }   
    if (ImGui::BeginPopup("##editCubemapPopup"))
    {
        if (renderEditorButtonGui(editorGuiData_, resources, "Edit cubemap"))
        {
            dab.add
            (
                [this]()
                {
                    set(editorGuiData_.selectedTextureResources);
                }
            );
        }
        ImGui::EndPopup();
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