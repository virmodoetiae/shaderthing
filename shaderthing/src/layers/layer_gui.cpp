/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a, virmodoetiae).
|  |\  \|\__    __\   |  For more information, visit:
|  \ \  \|__|\  \_|   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \|__|\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2023 Stefan Radman
|  Ↄ|C    \|__|\|__|  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "shaderthingapp.h"
#include "layers/layer.h"
#include "layers/layermanager.h"
#include "resources/resource.h"
#include "resources/resourcemanager.h"
#include "tools/findreplacetexttool.h"
#include "misc/misc.h"
#include "data/data.h"

#include "vir/include/vir.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

#include <random>

namespace ShaderThing
{

//----------------------------------------------------------------------------//
// Public functions ----------------------------------------------------------//

void Layer::renderGui()
{
    if (!ImGui::GetIO().WantTextInput && name_ != targetName_)
        toBeRenamed_ = true;
    
    bool tabOpen(true);
    bool* pTabOpen((isGuiRendered_) ? nullptr : &tabOpen);
    bool isGuiRendered0 = isGuiRendered_;
    if (ImGui::BeginTabItem(name_.c_str(), pTabOpen, ImGuiTabItemFlags_NoPushId))
    {
        renderGuiMain();
        ImGui::EndTabItem();
    }
    else
        isGuiRendered_ = false;
    if (isGuiRendered0 != isGuiRendered_ && !isGuiRendered_)
        app_.findReplaceTextToolRef().reset();
    if (!tabOpen)
        isGuiDeletionConfirmationPending_ = true;
    renderGuiConfirmDeletion();
}

void Layer::renderGuiSettings()
{
    float fontSize = ImGui::GetFontSize();
    float selectableWidth(14*fontSize);
    ImGui::Text("Name                 ");
    ImGui::SameLine();
    ImGui::PushItemWidth(selectableWidth);
    ImGui::InputText("##layerName", &targetName_);
    ImGui::PopItemWidth();
    ImGui::Text("Render target        ");
    ImGui::SameLine();
    ImGui::PushItemWidth(selectableWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##rendererTarget", 
            renderTargetToName[rendersTo_].c_str()
        )
    )
    {
        for(auto entry : renderTargetToName)
        {
            if (!ImGui::Selectable(entry.second.c_str()))
                continue;
            RendersTo target = entry.first;
            if (target != rendersTo_)
            {
                rendersTo_ = target;
                if (rendersTo_ != RendersTo::Window)
                    app_.resourceManagerRef().addLayerAsResource(this);
                else
                {
                    app_.resourceManagerRef().removeLayerAsResource(this);
                    auto window = vir::GlobalPtr<vir::Window>::instance();
                    resolutionScale_ = {1.f, 1.f};
                    isAspectRatioBoundToWindow_ = true;
                    targetResolution_ = {window->width(), window->height()};
                }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::Text("Resolution           ");
    ImGui::SameLine();
    
    if (rendersTo_ == RendersTo::Window)
        ImGui::BeginDisabled();
    ImGui::SameLine();
    auto x0 = ImGui::GetCursorPos().x;
    bool isAspectRatioBoundToWindow0(isAspectRatioBoundToWindow_);
    ImGui::Checkbox("##layerAspectRatioLock", &isAspectRatioBoundToWindow_);
    if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
    {
        ImGui::Text(
R"(If checked, the layer aspect ratio is 
locked to that of the main window)"
        );
        ImGui::EndTooltip();
    }
    if 
    (
        isAspectRatioBoundToWindow_ &&
        isAspectRatioBoundToWindow_ != isAspectRatioBoundToWindow0
    )
    {
        auto window = vir::GlobalPtr<vir::Window>::instance();
        resolutionScale_ = {1.f, 1.f};
        isAspectRatioBoundToWindow_ = true;
        targetResolution_ = {window->width(), window->height()};
    }
    ImGui::SameLine();
    auto checkboxSize = ImGui::GetCursorPos().x-x0;
    ImGui::PushItemWidth(selectableWidth-checkboxSize);
    if (ImGui::InputInt2("##resInput", glm::value_ptr(targetResolution_)))
        adjustTargetResolution();
    if (rendersTo_ == RendersTo::Window)
        ImGui::EndDisabled();
    ImGui::PopItemWidth();

    if (rendersTo_ != RendersTo::Window)
    {
        if (ImGui::CollapsingHeader("Framebuffer color attachment settings"))
        {
            ImGui::Text("Internal data format ");
            ImGui::SameLine();
            ImGui::SameLine();
            ImGui::PushItemWidth(selectableWidth);
            if 
            (
                ImGui::BeginCombo
                (
                    "##layerInternalFormatCombo",
                    vir::TextureBuffer::internalFormatToName.at
                    (
                        writeOnlyFramebuffer_->
                            colorBufferInternalFormat()
                    ).c_str()
                )
            )
            {
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
                        rebuildFramebuffers
                        (
                            internalFormat,
                            resolution_
                        );
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            //
            std::string selectedWrapModeX = "";
            std::string selectedWrapModeY = "";
            std::string selectedMagFilterMode = "";
            std::string selectedMinFilterMode = "";
            std::string selectedExportClearPolicy = "";
            if (readOnlyFramebuffer_ != nullptr)
            {
                selectedWrapModeX = vir::TextureBuffer::wrapModeToName.at
                (
                    readOnlyFramebuffer_->colorBufferWrapMode(0)
                );
                selectedWrapModeY = vir::TextureBuffer::wrapModeToName.at
                (
                    readOnlyFramebuffer_->colorBufferWrapMode(1)
                );
                selectedMagFilterMode = 
                    vir::TextureBuffer::filterModeToName.at
                    (
                        readOnlyFramebuffer_->colorBufferMagFilterMode()
                    );
                selectedMinFilterMode = 
                    vir::TextureBuffer::filterModeToName.at
                    (
                        readOnlyFramebuffer_->colorBufferMinFilterMode()
                    );
                selectedExportClearPolicy = 
                    internalFramebufferClearPolicyOnExportToName.at
                    (
                        internalFramebufferClearPolicyOnExport_
                    );
            }
            ImGui::Text("Horizontal wrap mode ");
            ImGui::SameLine();
            ImGui::PushItemWidth(selectableWidth);
            if 
            (
                ImGui::BeginCombo
                (
                    "##layerWrapModeXCombo",
                    selectedWrapModeX.c_str()
                ) && readOnlyFramebuffer_ != nullptr
            )
            {
                for (auto entry : vir::TextureBuffer::wrapModeToName)
                {
                    if (ImGui::Selectable(entry.second.c_str()))
                    {
                        readOnlyFramebuffer_->setColorBufferWrapMode
                        (
                            0, 
                            entry.first
                        );
                        writeOnlyFramebuffer_->setColorBufferWrapMode
                        (
                            0, 
                            entry.first
                        );
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            ImGui::Text("Vertical   wrap mode ");
            ImGui::SameLine();
            ImGui::PushItemWidth(selectableWidth);
            if 
            (
                ImGui::BeginCombo
                (
                    "##layerWrapModeYCombo",
                    selectedWrapModeY.c_str()
                ) && readOnlyFramebuffer_ != nullptr
            )
            {
                for (auto entry : vir::TextureBuffer::wrapModeToName)
                {
                    if (ImGui::Selectable(entry.second.c_str()))
                    {
                        readOnlyFramebuffer_->setColorBufferWrapMode
                        (
                            1, 
                            entry.first
                        );
                        writeOnlyFramebuffer_->setColorBufferWrapMode
                        (
                            1, 
                            entry.first
                        );
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ImGui::Text("Magnification filter ");
            ImGui::SameLine();
            ImGui::PushItemWidth(selectableWidth);
            if 
            (
                ImGui::BeginCombo
                (
                    "##layerMagModeCombo",
                    selectedMagFilterMode.c_str()
                ) && readOnlyFramebuffer_ != nullptr
            )
            {
                for (auto entry : vir::TextureBuffer::filterModeToName)
                {
                    if 
                    (
                        entry.first != vir::TextureBuffer::FilterMode::Nearest&&
                        entry.first != vir::TextureBuffer::FilterMode::Linear
                    )
                        continue;
                    if (ImGui::Selectable(entry.second.c_str()))
                    {
                        readOnlyFramebuffer_->setColorBufferMagFilterMode
                        (
                            entry.first
                        );
                        writeOnlyFramebuffer_->setColorBufferMagFilterMode
                        (
                            entry.first
                        );
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ImGui::Text("Minimization  filter ");
            ImGui::SameLine();
            ImGui::PushItemWidth(selectableWidth);
            if 
            (
                ImGui::BeginCombo
                (
                    "##layerMinModeCombo",
                    selectedMinFilterMode.c_str()
                ) && readOnlyFramebuffer_ != nullptr
            )
            {
                for (auto entry : vir::TextureBuffer::filterModeToName)
                {
                    if 
                    (
                        entry.first != vir::TextureBuffer::FilterMode::Nearest&&
                        entry.first != vir::TextureBuffer::FilterMode::Linear
                    )
                        continue;
                    if (ImGui::Selectable(entry.second.c_str()))
                    {
                        readOnlyFramebuffer_->setColorBufferMinFilterMode
                        (
                            entry.first
                        );
                        writeOnlyFramebuffer_->setColorBufferMinFilterMode
                        (
                            entry.first
                        );
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ImGui::Text("Export clear policy  ");
            ImGui::SameLine();
            ImGui::PushItemWidth(selectableWidth);
            if 
            (
                ImGui::BeginCombo
                (
                    "##layerExportClearPolicy",
                    selectedExportClearPolicy.c_str()
                ) && readOnlyFramebuffer_ != nullptr
            )
            {
                for (auto entry : internalFramebufferClearPolicyOnExportToName)
                {
                    if (ImGui::Selectable(entry.second.c_str()))
                        internalFramebufferClearPolicyOnExport_ = entry.first;
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
        }
    }
}

//----------------------------------------------------------------------------//
// Private functions ---------------------------------------------------------//

void Layer::renderGuiMain()
{
    isGuiRendered_ = true;
    app_.layerManagerRef().setActiveGuiLayer(this);
    float fontSize = ImGui::GetFontSize();

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("##layerTabBar", tab_bar_flags)) //------------------
    {
        static int tabIndex(0);
        static int tabIndex0(0);
        if (ImGui::BeginTabItem("Fragment source"))
        {
            tabIndex = 0;
            static ImVec4 redColor = {1,0,0,1};
            static ImVec4 grayColor = 
                ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
            static ImVec4 defaultColor = 
                ImGui::GetStyle().Colors[ImGuiCol_Text];
            app_.findReplaceTextToolRef().renderGui();
            hasUncompiledChanges_=
                app_.findReplaceTextToolRef().findReplaceTextInEditor
                (
                    fragmentSourceEditor_
                ) || hasUncompiledChanges_;
            if (app_.findReplaceTextToolRef().isGuiOpen()) 
                ImGui::Separator();
            ImGui::Indent();
            if (hasHeaderErrors_)
                ImGui::PushStyleColor(ImGuiCol_Text, redColor);
            if (ImGui::TreeNode("Header"))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, grayColor);
                ImGui::Text(fragmentSourceHeader_.c_str());
                ImGui::PopStyleColor(); 
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                {
                    ImGui::PushTextWrapPos(40.0f*fontSize);
                    if (hasHeaderErrors_)
                    {
                        std::string error = 
"Header has error(s), likely due to invalid uniform declaration(s), correct "
"uniform name(s)";
                        ImGui::PushStyleColor(ImGuiCol_Text, redColor);
                        ImGui::Text(error.c_str());
                        ImGui::PopStyleColor();
                        ImGui::Separator();
                    }
                    ImGui::PushStyleColor(ImGuiCol_Text, defaultColor);
                    ImGui::Text("Header information:");
                    ImGui::Bullet();ImGui::Text(
"the highest possible GLSL version (based on your hardware) is used;");
                    ImGui::Bullet();ImGui::Text(
"the quad coordinate 'qc' varies in the [-.5, .5] range across the (current) "
"shortest side of the window, and from [-x/2, x/2] across its longest side, "
"wherein 'x' is the ratio between the lengths of the longest and the shortest "
"window sides. In practice, any line between e.g., two points described via  "
"this coordinate will maintain its angle if the window aspect ratio is "
"changed by resizing. The origin is at the window center;");
                    ImGui::Bullet();ImGui::Text(
"the texture coordinate 'tc' varies in the [0-1] range across both sides of "
"the window, regardless of the window size. The origin is at the bottom-left "
"corner of the window;");
                    ImGui::Bullet();ImGui::Text(
"uniform declarations are added automatically (on shader compilation) based "
"on the uniforms you specify in this layer's 'Uniforms' tab.");
                    ImGui::PopTextWrapPos();
                    ImGui::PopStyleColor();
                    ImGui::EndTooltip();
                }
                ImGui::TreePop();
                ImGui::Separator();
                ImGui::Dummy(ImVec2(-1, ImGui::GetTextLineHeight()));
            }
            if (hasHeaderErrors_)
                ImGui::PopStyleColor();
            ImGui::Unindent();
            fragmentSourceEditor_.Render("##fragmentEditor");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Common source"))
        {
            tabIndex = 1;
            app_.findReplaceTextToolRef().renderGui();
            hasUncompiledChanges_=
                app_.findReplaceTextToolRef().findReplaceTextInEditor
                (
                    Layer::sharedSourceEditor_
                ) || hasUncompiledChanges_;
            if (app_.findReplaceTextToolRef().isGuiOpen()) 
                ImGui::Separator();
            Layer::sharedSourceEditor_.Render
            (
                "##sharedEditor"
            );
            hasUncompiledChanges_ = 
                hasUncompiledChanges_ || 
                sharedSourceEditor_.IsTextChanged();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Uniforms"))
        {
            tabIndex = 2;
            renderGuiUniforms();
            ImGui::EndTabItem();
        }
        if (tabIndex != tabIndex0)
            app_.findReplaceTextToolRef().clearCachedTextToBeFound();
        tabIndex0 = tabIndex;
        ImGui::EndTabBar();
    }

    app_.findReplaceTextToolRef().update(); // Ctrl+F/Ctrl+H to open 
}

//----------------------------------------------------------------------------//

void Layer::renderGuiConfirmDeletion()
{
    std::string text("Delete "+name_);
    if (isGuiDeletionConfirmationPending_)
        ImGui::OpenPopup(text.c_str());
    static auto flags = ImGuiWindowFlags_AlwaysAutoResize;
    if (ImGui::BeginPopupModal(text.c_str(), nullptr, flags))
    {
        ImGui::Text("Are you sure you want to delete this layer?");
        ImGui::Text("This action cannot be undone");
        if (ImGui::Button("Confirm"))
        {
            toBeDeleted_ = true;
            isGuiDeletionConfirmationPending_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            isGuiDeletionConfirmationPending_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------//

void Layer::renderGuiUniforms()
{
    float fontSize = ImGui::GetFontSize();
    shader_->bind();
    static ImGuiTableFlags tableFlags = 
        ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_SizingFixedFit;
    static int nColumns = 5;
    int deleteRow = -1;
    // Very dirty stuff, totally not proud of this
    bool nextUniformIsSampler2DResolution(false);
    static bool deferredUserAction(false);
    if (ImGui::BeginTable("##uniformTable", nColumns, tableFlags))
    {
        // Declare columns
        ImGui::TableSetupColumn("Actions", 0, 5*fontSize);
        ImGui::TableSetupColumn("Uniform name", 0, 9*fontSize);
        ImGui::TableSetupColumn("Uniform type", 0, 7*fontSize);
        ImGui::TableSetupColumn("Value limits", 0, 7*fontSize);
        ImGui::TableSetupColumn
        (
            "Uniform value", 
            0, 
            ImGui::GetContentRegionAvail().x
        );

        ImGui::TableHeadersRow();
        int nSharedUniforms(app_.sharedUniformsRef().size());
        int defaultUniformsStartRow = nSharedUniforms;
        int nDefaultUniforms(defaultUniforms_.size());
        int uniformsStartRow = nSharedUniforms+nDefaultUniforms;
        int nTotalUniforms(nSharedUniforms+nDefaultUniforms+uniforms_.size());
        
        // Actual rendering ----------------------------------------------------
        for (int row = 0; row < nTotalUniforms+1; row++)
        {
            ImGui::PushID(row);
            ImGui::TableNextRow(0, 1.6*fontSize);
            int col = 0;

            // This is exclusively to render the sampler2D resolution uniforms,
            // which are automatically managed
            if (nextUniformIsSampler2DResolution)
            {
                row--; // Extremely bad practice, I know!
                nextUniformIsSampler2DResolution = false;
                auto uniform = uniforms_[row-nDefaultUniforms-nSharedUniforms];
                // Column 0 ----------------------------------------------------
                ImGui::TableSetColumnIndex(col++);
                // Column 1 ----------------------------------------------------
                ImGui::TableSetColumnIndex(col++);
                std::string name = uniform->name+"Resolution";
                ImGui::Text(name.c_str());
                // Column 2 ----------------------------------------------------
                ImGui::TableSetColumnIndex(col++);
                ImGui::Text
                (
                    vir::Shader::uniformTypeToName
                    [
                        vir::Shader::Variable::Type::Float2
                    ].c_str()
                );
                // Column 3 ----------------------------------------------------
                ImGui::TableSetColumnIndex(col++);
                // Column 4-----------------------------------------------------
                ImGui::TableSetColumnIndex(col++);
                auto resource = uniform->getValuePtr<Resource>();
                if (resource != nullptr)
                {
                    std::string resolution
                    (
                        std::to_string(resource->width())+" x "+
                        std::to_string(resource->height())
                    );
                    ImGui::Text(resolution.c_str());
                }
                ImGui::PopID();
                continue;
            }

            if (row == nTotalUniforms)
            {
                ImGui::TableSetColumnIndex(col++);
                ImGui::PushItemWidth(-1);
                if (ImGui::Button("Add", ImVec2(-1, 0)))
                {
                    auto uniform = new Uniform();
                    uniforms_.emplace_back(uniform);
                    uncompiledUniforms_.emplace_back(uniform);
                }
                ImGui::PopItemWidth();
                ImGui::PopID();
                break;
            }

            std::string tooltipText = "";
            bool isShared(row < defaultUniformsStartRow);
            bool isDefault(!isShared && row < uniformsStartRow);
            bool isCustom(!isShared && !isDefault);
            Uniform* uniform = nullptr;
            if (isCustom)
                uniform = uniforms_[row-uniformsStartRow];
            else if (isDefault)
                uniform = defaultUniforms_[row-defaultUniformsStartRow];
            else 
                uniform = app_.sharedUniformsRef()[row];

            // These are used to check wheter the uniform name or type has
            // changed. If so, it is added to the list of uncompiled uniforms
            // to enable the appearance of the compilation button (managed by
            // the LayerManager)
            auto uniformName0 = uniform->name;
            auto uniformType0 = uniform->type;

            // Column 0 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            if (row >= uniformsStartRow)
            {
                if (ImGui::Button("Delete", ImVec2(-1, 0)))
                    deleteRow = row-uniformsStartRow;
            }
            else if (uniform->specialType == Uniform::SpecialType::Frame)
            {
                //isShared = true;
                if (ImGui::Button(ICON_FA_UNDO, ImVec2(2.2*fontSize, 0)))
                    app_.restartRendering();
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                {
                    static ImVec4 ctrlRColor = 
                        ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
                    ImGui::Text("Restart rendering");
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, ctrlRColor);
                    ImGui::Text("Ctrl+R");
                    ImGui::PopStyleColor();
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                bool& isRenderingPaused(app_.isRenderingPausedRef());
                bool& isTimePaused(app_.isTimePausedRef());
                // Pause/resume rendering, which also affects iTime (but the
                // opposite is not true)
                if 
                (
                    ImGui::Button
                    (
                        isRenderingPaused ? ICON_FA_PLAY : ICON_FA_PAUSE, 
                        ImVec2(-1, 0)
                    )
                )
                {
                    isRenderingPaused = !isRenderingPaused;
                    isTimePaused = isRenderingPaused;
                }
            }
            else if (uniform->specialType == Uniform::SpecialType::Time)
            {
                //isShared = true;
                bool& isTimeLooped(app_.isTimeLoopedRef());
                if 
                (
                    ImGui::Button
                    (
                        isTimeLooped ? ICON_FA_MINUS : ICON_FA_CIRCLE_NOTCH, 
                        ImVec2(2.2*fontSize, 0)
                    )
                )
                    isTimeLooped = !isTimeLooped;
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                {
                    ImGui::Text(isTimeLooped ? "Disable loop" : "Enable loop");
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                std::string text = 
                    (app_.isTimePausedCRef()) ? ICON_FA_PLAY : ICON_FA_PAUSE;
                if (ImGui::Button(text.c_str(), ImVec2(-1, 0)))
                    app_.isTimePausedRef() = !app_.isTimePausedCRef();
            }
#define ENABLE_DISABLE_APP_INPUT_TOOLTIP(eventName)                         \
if(ImGui::IsItemHovered() && ImGui::BeginTooltip())                         \
{                                                                           \
    if (enabled)                                                            \
        ImGui::Text(                                                        \
"Disable "#eventName" for controlling the state of this uniform"            \
        );                                                                  \
    else                                                                    \
        ImGui::Text(                                                        \
"Enable "#eventName" for controlling the state of this uniform"             \
        );                                                                  \
    ImGui::EndTooltip();                                                    \
}
            else if (uniform->specialType==Uniform::SpecialType::CameraPosition)
            {
                isShared = true;
                tooltipText = 
R"(Keyboard-controlled vec3 position in 3-D space. All movement is with respect to 
the current direction of iLook. For an iLook aligned with the z axis in a right
handed reference frame, z is forward, x is left, y is up.  With respect to said
reference frame, pressing/holding the W, A, S, D, space or left-shift keys will
move the position forwards, left, backwards, right, up, down respectively.
Please also note that the up direction (positive y) never changes)";
                if (ImGui::Button("Settings", ImVec2(-1, 0)))
                    ImGui::OpenPopup("##iCameraPositionSettings");
                if (ImGui::BeginPopup("##iCameraPositionSettings"))
                {
                    bool enabled = app_.isCameraPositionInputEnabled();
                    std::string text = enabled ? "Disable" : "Enable";
                    if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
                    {
                        app_.setShaderCameraPositionInputsEnabled(!enabled);
                    }
                    ENABLE_DISABLE_APP_INPUT_TOOLTIP(keyboard inputs)
                    ImGui::Text("Keyboard sensitivity ");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1);
                    ImGui::SliderFloat
                    (
                        "##iCameraPositionSensitivity", 
                        &shaderCamera_.keySensitivityRef(),
                        1e-1,
                        50
                    );
                    ImGui::PopItemWidth();
                    ImGui::EndPopup();
                }
            }
            else if 
            (
                uniform->specialType == Uniform::SpecialType::CameraDirection
            )
            {
                tooltipText = 
R"(Mouse-controlled look direction in 3-D space. Click and drag (on the window)
to modify. Best suited for controlling a camera)";
                isShared = true;
                if (ImGui::Button("Settings", ImVec2(-1, 0)))
                    ImGui::OpenPopup("##iCameraDirectionSettings");
                if (ImGui::BeginPopup("##iCameraDirectionSettings"))
                {
                    bool enabled = app_.isCameraDirectionInputEnabled();
                    std::string text = enabled ? "Disable" : "Enable";
                    if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
                    {
                        app_.setShaderCameraDirectionInputsEnabled(!enabled);
                    }
                    ENABLE_DISABLE_APP_INPUT_TOOLTIP(keyboard inputs)
                    ImGui::Text("Mouse sensitivity ");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1);
                    ImGui::SliderFloat
                    (
                        "##iCameraDirectionSensitivity", 
                        &shaderCamera_.mouseSensitivityRef(),
                        1e-3,
                        1
                    );
                    ImGui::PopItemWidth();
                    ImGui::EndPopup();
                }
            }
            else if (uniform->specialType == Uniform::SpecialType::Mouse)
            {
                isShared = true;
                bool enabled = app_.isMouseInputEnabled();
                std::string text = enabled ? "Disable" : "Enable";
                if (ImGui::Button(text.c_str(), ImVec2(-1, 0)))
                    app_.setMouseInputsEnabled(!enabled);
                ENABLE_DISABLE_APP_INPUT_TOOLTIP(mouse inputs)
            }
            ImGui::PopItemWidth();

            // Column 1 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            if (isShared || isDefault)
            {
                ImGui::Text(uniform->name.c_str());
                if (isShared)
                {
                    if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                    {
                        ImGui::Text
                        (
                            "This uniform is shared across all layers"
                        );
                        if (tooltipText.size() > 0)
                        {
                            ImGui::Separator();
                            ImGui::Text(tooltipText.c_str());
                        }
                        ImGui::EndTooltip();
                    }
                }
            }
            else 
                ImGui::InputText("##name", &uniform->name);
            ImGui::PopItemWidth();

            bool named(uniform->name.size() > 0);
            
            // Column 2 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            if (isShared || isDefault)
                ImGui::Text
                (
                    vir::Shader::uniformTypeToName[uniform->type].c_str()
                );
            else if 
            (
                ImGui::BeginCombo
                (
                    "##ufSelector", 
                    vir::Shader::uniformTypeToName[uniform->type].c_str()
                )
            )
            {
                for(auto uniformTypeName : Layer::supportedUniformTypeNames)
                    if (ImGui::Selectable(uniformTypeName.c_str()))
                    {
                        auto selectedType = 
                            vir::Shader::uniformNameToType[uniformTypeName];
                            if (selectedType != uniform->type)
                            {
                                uniform->resetValue();
                                uniform->type = selectedType;
                                uniform->showLimits = 
                                (
                                    selectedType != 
                                        vir::Shader::Variable::Type::Bool &&
                                    selectedType != 
                                        vir::Shader::Variable::Type::Sampler2D &&
                                    selectedType != 
                                        vir::Shader::Variable::Type::SamplerCube
                                );
                            }
                    }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            // Column 3 --------------------------------------------------------
            glm::vec2& uLimits = uniform->limits;
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            bool limitsChanged(false);
            if (uniform->showLimits)
            {
                std::string format = 
                    (uniform->type == vir::Shader::Variable::Type::Int) ?
                    "%.0f" :
                    "%.1f";
                glm::vec2 uLimits0(uLimits);
                if (uniform->type == vir::Shader::Variable::Type::UInt)
                    uLimits.x = std::max(uLimits.x, 0.0f);
                ImGui::InputFloat2
                (
                    "##f2Input", 
                    glm::value_ptr(uLimits), 
                    format.c_str()
                );
                limitsChanged = (uLimits != uLimits0);
            }
            ImGui::PopItemWidth();

            // Column 4--------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            switch(uniform->type)
            {
                case vir::Shader::Variable::Type::Bool :
                {
                    auto value = uniform->getValue<bool>();
                    if (uniform->specialType==Uniform::SpecialType::UserAction)
                    {
                        ImGui::Text
                        (
                            (userAction0_ || deferredUserAction) ? 
                            "true" : "false"
                        );
                    }
                    else if (ImGui::Checkbox((value) ? "true" : "false", &value))
                    {
                        uniform->setValue(value);
                        if (named)
                        {
                            shader_->setUniformBool(uniform->name, value);
                            app_.userActionRef() = true;
                        }
                    }
                    break;
                }
                case vir::Shader::Variable::Type::UInt :
                {
                    auto value = uniform->getValue<uint32_t>();
                    value = std::max(value, (uint32_t)0);
                    if (!limitsChanged)
                    {
                        uLimits.x = std::min((float)value, uLimits.x);
                        uLimits.y = std::max((float)value, uLimits.y);
                    }
                    bool input(false);
                    if (uniform->specialType == Uniform::SpecialType::Frame)
                        ImGui::Text
                        (
                            std::to_string(std::max(app_.frameRef(),0)).c_str()
                        );
                    else if 
                    (
                        uniform->specialType == Uniform::SpecialType::RenderPass
                    )
                        ImGui::Text
                        (
                            std::to_string
                            (
                                std::max(app_.renderPassRef(), 0)
                            ).c_str()
                        );
                    else
                    {
                        input = ImGui::SliderInt
                        (
                            "##iSlider", 
                            (int*)&value, 
                            (int)(uLimits.x),
                            (int)(uLimits.y)
                        );
                    }
                    if (input || limitsChanged)
                    {
                        value = std::max(value, (uint32_t)0);
                        if (limitsChanged)
                        {
                            value = std::max((float)value, uLimits.x);
                            value = std::min((float)value, uLimits.y);
                        }
                        uniform->setValue(value);
                        if (named)
                        {
                            shader_->setUniformInt(uniform->name, value);
                            app_.userActionRef() = true;
                        }
                    }
                    break;
                }
                case vir::Shader::Variable::Type::Int :
                {
                    auto value = uniform->getValue<int>();
                    if (!limitsChanged)
                    {
                        uLimits.x = std::min((float)value, uLimits.x);
                        uLimits.y = std::max((float)value, uLimits.y);
                    }
                    bool input(false);
                    if 
                    (
                        ImGui::SliderInt
                        (
                            "##iSlider", 
                            &value, 
                            (int)(uLimits.x),
                            (int)(uLimits.y)
                        ) || limitsChanged
                    )
                    {
                        if (limitsChanged)
                        {
                            value = std::max((float)value, uLimits.x);
                            value = std::min((float)value, uLimits.y);
                        }
                        uniform->setValue(value);
                        if (named)
                        {
                            shader_->setUniformInt(uniform->name, value);
                            app_.userActionRef() = true;
                        }
                    }
                    break;
                }
                case vir::Shader::Variable::Type::Int2 :
                {
                    auto value = uniform->getValue<glm::ivec2>();
                    if (!limitsChanged)
                    {
                        uLimits.x = std::min(value.x, (int)uLimits.x);
                        uLimits.x = std::min(value.y, (int)uLimits.x);
                        uLimits.y = std::max(value.x, (int)uLimits.y);
                        uLimits.y = std::max(value.y, (int)uLimits.y);
                    }
                    bool input(false);
                    {
                        ImGui::SmallButton(ICON_FA_MOUSE_POINTER); 
                        ImGui::SameLine();
                        if (ImGui::IsItemActive())
                        {
                            ImGui::GetForegroundDrawList()->AddLine
                            (
                                ImGui::GetIO().MouseClickedPos[0], 
                                ImGui::GetIO().MousePos, 
                                ImGui::GetColorU32(ImGuiCol_Button), 
                                4.0f
                            );
                            ImVec2 delta = ImGui::GetMouseDragDelta(0, 0.0f);
                            delta.y = -delta.y;
                            auto monitor = 
                                vir::GlobalPtr<vir::Window>::instance()->
                                primaryMonitorResolution();
                            int maxRes = std::max(monitor.x, monitor.y);
                            uLimits.x = std::min(uLimits.x, -uLimits.y);
                            uLimits.y = std::max(-uLimits.x, uLimits.y);
                            value.x = 
                                std::max
                                (
                                    std::min
                                    (
                                        (float)(4*delta.x*uLimits.y)/maxRes, 
                                        uLimits.y
                                    ),
                                    uLimits.x
                                );
                            value.y = 
                                std::max
                                (
                                    std::min
                                    (
                                        (float)(4*delta.y*uLimits.y)/maxRes, 
                                        uLimits.y
                                    ),
                                    uLimits.x
                                );
                            input = true;
                        }
                        bool input2 = ImGui::SliderInt2
                        (
                            "##i2Slider", 
                            glm::value_ptr(value), 
                            uLimits.x,
                            uLimits.y
                        );
                        input = input || input2;
                    }
                    if (input || limitsChanged)
                    {
                        if (limitsChanged)
                        {
                            value.x = std::max(value.x, (int)uLimits.x);
                            value.x = std::min(value.x, (int)uLimits.y);
                            value.y = std::max(value.y, (int)uLimits.x);
                            value.y = std::min(value.y, (int)uLimits.y);
                        }
                        uniform->setValue(value);
                        if (named)
                        {
                            shader_->setUniformInt2(uniform->name, value);
                            app_.userActionRef() = true;
                        }
                    }
                    break;
                }
                case vir::Shader::Variable::Type::Int3 :
                {
                    auto value = uniform->getValue<glm::ivec3>();
                    if (!limitsChanged)
                    {
                        uLimits.x = std::min(value.x, (int)uLimits.x);
                        uLimits.x = std::min(value.y, (int)uLimits.x);
                        uLimits.x = std::min(value.z, (int)uLimits.x);
                        uLimits.y = std::max(value.x, (int)uLimits.y);
                        uLimits.y = std::max(value.y, (int)uLimits.y);
                        uLimits.y = std::max(value.z, (int)uLimits.y);
                    }
                    bool keyboardInput(false);
                    static float recordT0(0.0f);
                    if (!(isShared || isDefault))
                    {
                        if 
                        (
                            ImGui::SmallButton
                            (
                                uniform->usesKeyboardInput ? 
                                ICON_FA_SLIDERS_H : 
                                ICON_FA_KEYBOARD
                            )
                        )
                        {
                            uniform->usesKeyboardInput = 
                                !uniform->usesKeyboardInput;
                            uniform->keyCode = -1;
                            value = glm::ivec3(0);
                            if (uniform->usesKeyboardInput)
                            {
                                uniform->showLimits = false;
                                recordT0 = 
                                    vir::GlobalPtr<vir::Time>::instance()->
                                    outerTime();
                            }
                            else
                            {
                                for (int i=0; i<3; i++)
                                    uniform->keyState[i] = nullptr;
                            }
                        }
                        ImGui::SameLine();
                        keyboardInput = uniform->usesKeyboardInput;
                    }
                    if (keyboardInput)
                    {
                        if (uniform->keyCode == -1)
                        {
                            for 
                            (
                                ImGuiKey key = ImGuiKey_NamedKey_BEGIN;
                                key < ImGuiKey_NamedKey_END;
                                key = (ImGuiKey)(key+1)
                            )
                            {
                                if (!ImGui::IsKeyDown(key))
                                    continue;
                                int virKey=vir::inputKeyCodeImGuiToVir(key);
                                if (virKey != VIR_KEY_UNKNOWN)
                                {
                                    uniform->keyCode = virKey;
                                    auto is = 
                                        vir::GlobalPtr<vir::InputState>::
                                        instance();
                                    uniform->keyState[0] = 
                                        &is->keyState(virKey).pressedRef();
                                    uniform->keyState[1] = 
                                        &is->keyState(virKey).heldRef();
                                    uniform->keyState[2] = 
                                        &is->keyState(virKey).toggleRef();
                                    break;
                                }
                            }
                            ImGui::PushStyleColor
                            (
                                ImGuiCol_Text,
                                {
                                    1,
                                    0,
                                    0,
                                    int
                                    (
                                        vir::GlobalPtr<vir::Time>::instance()->
                                        outerTime()-recordT0
                                    )%2==0}
                            );
                            ImGui::Text(ICON_FA_CIRCLE);
                            ImGui::PopStyleColor();
                            ImGui::SameLine();
                            ImGui::Text("Press key to bind...", ICON_FA_CIRCLE);
                        }
                        else
                        {
                            ImGui::Text
                            (
                                "Key: \'%s\', (%d, %d, %d)", 
                                vir::keyCodeToName.at(uniform->keyCode).c_str(),
                                value.x,
                                value.y,
                                value.z
                            );
                        }
                    }
                    if (!keyboardInput)
                    {
                        if 
                        (
                            ImGui::SliderInt3
                            (
                                "##i3Slider", 
                                glm::value_ptr(value), 
                                uLimits.x,
                                uLimits.y
                            )
                        )
                        {
                            if (limitsChanged)
                            {
                                value.x = std::max(value.x, (int)uLimits.x);
                                value.x = std::min(value.x, (int)uLimits.y);
                                value.y = std::max(value.y, (int)uLimits.x);
                                value.y = std::min(value.y, (int)uLimits.y);
                                value.z = std::max(value.z, (int)uLimits.x);
                                value.z = std::min(value.z, (int)uLimits.y);
                            }
                            uniform->setValue(value);
                            if (named)
                            {
                                shader_->setUniformInt3(uniform->name, value);
                                app_.userActionRef() = true;
                            }
                        }
                    }
                    break;
                }
                case vir::Shader::Variable::Type::Int4 :
                {
                    auto value = uniform->getValue<glm::ivec4>();
                    if (!limitsChanged)
                    {
                        uLimits.x = std::min(value.x, (int)uLimits.x);
                        uLimits.x = std::min(value.y, (int)uLimits.x);
                        uLimits.x = std::min(value.z, (int)uLimits.x);
                        uLimits.x = std::min(value.w, (int)uLimits.x);
                        uLimits.y = std::max(value.x, (int)uLimits.y);
                        uLimits.y = std::max(value.y, (int)uLimits.y);
                        uLimits.y = std::max(value.z, (int)uLimits.y);
                        uLimits.y = std::max(value.w, (int)uLimits.y);
                    }
                    bool input(false);
                    if (uniform->specialType == Uniform::SpecialType::Mouse)
                    {
                        std::string data = 
                            std::to_string(app_.mouseRef().x)+", "+
                            std::to_string(app_.mouseRef().y)+", "+
                            std::to_string(app_.mouseRef().z)+", "+
                            std::to_string(app_.mouseRef().w);
                        ImGui::Text(data.c_str());
                        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                        {
                            ImGui::Text(
R"(The first two components (x, y) are the current x, y coordinates (with respect
to the lower-left corner of the ShaderThing window) of the mouse cursor if the
left mouse button is currently being held down. The last two components (z, w)
represent the x, y coordinates of the last left mouse button click, with their 
sign reversed. If the sign of the z component is positive, then the left mouse
is currently being held down)");
                            ImGui::EndTooltip();
                        }
                    }
                    else
                        input = ImGui::SliderInt4
                        (
                            "##i4Slider", 
                            glm::value_ptr(value), 
                            uLimits.x,
                            uLimits.y
                        );
                    if (input)
                    {
                        if (limitsChanged)
                        {
                            value.x = std::max(value.x, (int)uLimits.x);
                            value.x = std::min(value.x, (int)uLimits.y);
                            value.y = std::max(value.y, (int)uLimits.x);
                            value.y = std::min(value.y, (int)uLimits.y);
                            value.z = std::max(value.z, (int)uLimits.x);
                            value.z = std::min(value.z, (int)uLimits.y);
                            value.w = std::max(value.z, (int)uLimits.x);
                            value.w = std::min(value.z, (int)uLimits.y);
                        }
                        uniform->setValue(value);
                        if (named)
                        {
                            shader_->setUniformInt4(uniform->name, value);
                            app_.userActionRef() = true;
                        }
                    }
                    break;
                }
                case vir::Shader::Variable::Type::Float :
                {
                    auto value = uniform->getValue<float>();
                    if (!limitsChanged)
                    {
                        uLimits.x = std::min(value, uLimits.x);
                        uLimits.y = std::max(value, uLimits.y);
                    }
                    bool input(false);
                    if 
                    (
                        uniform->specialType == 
                            Uniform::SpecialType::WindowAspectRatio ||
                        uniform->specialType == 
                            Uniform::SpecialType::LayerAspectRatio
                    )
                        ImGui::Text(std::to_string(value).c_str());
                    else
                        input = ImGui::SliderFloat
                        (
                            "##fSlider", 
                            &value, 
                            uLimits.x,
                            uLimits.y
                        );
                    if (input || limitsChanged)
                    {
                        if (limitsChanged)
                        {
                            value = std::max(value, uLimits.x);
                            value = std::min(value, uLimits.y);
                        }
                        uniform->setValue(value);
                        if (named)
                        {
                            shader_->setUniformFloat(uniform->name, value);
                            app_.userActionRef() = true;
                        }
                    }
                    break;
                }
                case vir::Shader::Variable::Type::Float2 :
                {
                    auto value = uniform->getValue<glm::vec2>();
                    if (!limitsChanged)
                    {
                        uLimits.x = std::min(value.x, uLimits.x);
                        uLimits.x = std::min(value.y, uLimits.x);
                        uLimits.y = std::max(value.x, uLimits.y);
                        uLimits.y = std::max(value.y, uLimits.y);
                    }
                    bool input(false);
                    if 
                    (
                        uniform->specialType == 
                            Uniform::SpecialType::WindowResolution || 
                        uniform->specialType == 
                            Uniform::SpecialType::LayerResolution
                    )
                    {
                        auto fvalue = uniform->getValue<glm::ivec2>();
                        std::string resolution
                        (
                            std::to_string(fvalue.x)+" x "+
                            std::to_string(fvalue.y)
                        );
                        ImGui::Text(resolution.c_str());
                    }
                    else 
                    {
                        ImGui::SmallButton(ICON_FA_MOUSE_POINTER); 
                        ImGui::SameLine();
                        if (ImGui::IsItemActive())
                        {
                            ImGui::GetForegroundDrawList()->AddLine
                            (
                                ImGui::GetIO().MouseClickedPos[0], 
                                ImGui::GetIO().MousePos, 
                                ImGui::GetColorU32(ImGuiCol_Button), 
                                4.0f
                            );
                            ImVec2 delta = ImGui::GetMouseDragDelta(0, 0.0f);
                            delta.y = -delta.y;
                            auto monitor = 
                                vir::GlobalPtr<vir::Window>::instance()->
                                primaryMonitorResolution();
                            int maxRes = std::max(monitor.x, monitor.y);
                            uLimits.x = std::min(uLimits.x, -uLimits.y);
                            uLimits.y = std::max(-uLimits.x, uLimits.y);
                            value.x = 
                                std::max
                                (
                                    std::min
                                    (
                                        (float)(4*delta.x*uLimits.y)/maxRes, 
                                        uLimits.y
                                    ),
                                    uLimits.x
                                );
                            value.y = 
                                std::max
                                (
                                    std::min
                                    (
                                        (float)(4*delta.y*uLimits.y)/maxRes, 
                                        uLimits.y
                                    ),
                                    uLimits.x
                                );
                            input = true;
                        }
                        bool input2 = ImGui::SliderFloat2
                        (
                            "##f2Slider", 
                            glm::value_ptr(value), 
                            uLimits.x,
                            uLimits.y
                        );
                        input = input || input2;
                    }
                    if (input || limitsChanged)
                    {
                        if (limitsChanged)
                        {
                            value.x = std::max(value.x, uLimits.x);
                            value.x = std::min(value.x, uLimits.y);
                            value.y = std::max(value.y, uLimits.x);
                            value.y = std::min(value.y, uLimits.y);
                        }
                        uniform->setValue(value);
                        if (named)
                        {
                            shader_->setUniformFloat2(uniform->name, value);
                            app_.userActionRef() = true;
                        }
                    }
                    break;
                }
                case vir::Shader::Variable::Type::Float3 :
                {
                    auto value = uniform->getValue<glm::vec3>();
                    if (!limitsChanged)
                    {
                        uLimits.x = std::min(value.x, uLimits.x);
                        uLimits.x = std::min(value.y, uLimits.x);
                        uLimits.x = std::min(value.z, uLimits.x);
                        uLimits.y = std::max(value.x, uLimits.y);
                        uLimits.y = std::max(value.y, uLimits.y);
                        uLimits.y = std::max(value.z, uLimits.y);
                    }
                    bool colorPicker(false);
                    if (!isShared)
                    {
                        if 
                        (
                            ImGui::SmallButton
                            (
                                uniform->usesColorPicker ? 
                                ICON_FA_SLIDERS_H : 
                                ICON_FA_PAINT_BRUSH
                            )
                        )
                            uniform->usesColorPicker = !uniform->usesColorPicker;
                        ImGui::SameLine();
                        colorPicker = uniform->usesColorPicker;
                    }
                    if (!colorPicker)
                    {
                        if 
                        (
                            ImGui::SliderFloat3
                            (
                                "##f3Slider", 
                                glm::value_ptr(value), 
                                uLimits.x,
                                uLimits.y
                            ) || limitsChanged
                        )
                        {
                            if (limitsChanged)
                            {
                                value.x = std::max(value.x, uLimits.x);
                                value.x = std::min(value.x, uLimits.y);
                                value.y = std::max(value.y, uLimits.x);
                                value.y = std::min(value.y, uLimits.y);
                                value.z = std::max(value.z, uLimits.x);
                                value.z = std::min(value.z, uLimits.y);
                            }
                            bool isCameraDirection
                            (
                                uniform->specialType == 
                                    Uniform::SpecialType::CameraDirection
                            );
                            if (isCameraDirection)
                                value = glm::normalize(value);
                            uniform->setValue(value);
                            if (named)
                            {
                                shader_->setUniformFloat3(uniform->name, value);
                                app_.userActionRef() = true;
                            }
                            // I am manipulating the reference to Z, but not 
                            // re-setting other axes automatically within the 
                            // camera class. Mouse-based rotations work fine in 
                            // resetting the axes because they already call the 
                            // rotate functions based on the input, but a direct
                            // manipulation of the camera direction via the 
                            // ImGui slider does not cause this to work. Thus, I
                            // am re-calculating the other camera directions as
                            // well
                            if (isCameraDirection)
                                shaderCamera_.setDirection(value);
                        }
                    }
                    else
                    {
                        if 
                        (
                            ImGui::ColorEdit3
                            (
                                "##uniformColorEdit3", 
                                glm::value_ptr(value)
                            )
                        )
                        {
                            uLimits.x = 0.0;
                            uLimits.y = 1.0;
                            uniform->setValue(value);
                            if (named)
                            {
                                shader_->setUniformFloat3(uniform->name, value);
                                app_.userActionRef() = true;
                            }
                        }
                    }
                    break;
                }
                case vir::Shader::Variable::Type::Float4 :
                {
                    auto value = uniform->getValue<glm::vec4>();
                    if (!limitsChanged)
                    {
                        uLimits.x = std::min(value.x, uLimits.x);
                        uLimits.x = std::min(value.y, uLimits.x);
                        uLimits.x = std::min(value.z, uLimits.x);
                        uLimits.x = std::min(value.w, uLimits.x);
                        uLimits.y = std::max(value.x, uLimits.y);
                        uLimits.y = std::max(value.y, uLimits.y);
                        uLimits.y = std::max(value.z, uLimits.y);
                        uLimits.y = std::max(value.w, uLimits.y);
                    }
                    bool colorPicker(false);
                    if (!isShared)
                    {
                        if 
                        (
                            ImGui::SmallButton
                            (
                                uniform->usesColorPicker ? 
                                ICON_FA_SLIDERS_H : 
                                ICON_FA_PAINT_BRUSH
                            )
                        )
                            uniform->usesColorPicker = !uniform->usesColorPicker;
                        ImGui::SameLine();
                        colorPicker = uniform->usesColorPicker;
                    }
                    if (!colorPicker)
                    {
                        if 
                        (
                            ImGui::SliderFloat4
                            (
                                "##f4Slider", 
                                glm::value_ptr(value), 
                                uLimits.x,
                                uLimits.y
                            ) || limitsChanged
                        )
                        {
                            if (limitsChanged)
                            {
                                value.x = std::max(value.x, uLimits.x);
                                value.x = std::min(value.x, uLimits.y);
                                value.y = std::max(value.y, uLimits.x);
                                value.y = std::min(value.y, uLimits.y);
                                value.z = std::max(value.z, uLimits.x);
                                value.z = std::min(value.z, uLimits.y);
                                value.w = std::max(value.w, uLimits.x);
                                value.w = std::min(value.w, uLimits.y);
                            }
                            uniform->setValue(value);
                            if (named)
                                shader_->setUniformFloat4(uniform->name, value);
                        }
                    }
                    else
                    {
                        if 
                        (
                            ImGui::ColorEdit4
                            (
                                "##uniformColorEdit4", 
                                glm::value_ptr(value)
                            )
                        )
                        {
                            uLimits.x = 0.0;
                            uLimits.y = 1.0;
                            uniform->setValue(value);
                            if (named)
                            {
                                shader_->setUniformFloat4(uniform->name, value);
                                app_.userActionRef() = true;
                            }
                        }
                    }
                    break;
                }
                case vir::Shader::Variable::Type::Sampler2D :
                {
                    auto resource = 
                        uniform->getValuePtr<Resource>();
                    std::string name
                    (
                        (resource != nullptr) ? resource->name() : ""
                    );
                    if (ImGui::BeginCombo("##txSelector", name.c_str()))
                    {
                        for(auto r : app_.resourceManagerRef().resourcesRef())
                        {
                            if 
                            (
                                r->type() != Resource::Type::Texture2D &&
                                r->type() != 
                                    Resource::Type::FramebufferColorAttachment
                            )
                                continue;
                            if (ImGui::Selectable(r->name().c_str()))
                            {
                               uniform->setValuePtr<Resource>(r);
                               app_.userActionRef() = true;
                            }
                        }
                        ImGui::EndCombo();
                    }
                    break;
                }
                case vir::Shader::Variable::Type::SamplerCube :
                {
                    auto resource = 
                        uniform->getValuePtr<Resource>();
                    std::string name
                    (
                        (resource != nullptr) ? resource->name() : ""
                    );
                    if (ImGui::BeginCombo("##cmSelector", name.c_str()))
                    {
                        for(auto r : app_.resourceManagerRef().resourcesRef())
                        {
                            if (r->type() != Resource::Type::Cubemap)
                                continue;
                            if (ImGui::Selectable(r->name().c_str()))
                            {
                               uniform->setValuePtr<Resource>(r);
                               app_.userActionRef() = true;
                            }
                        }
                        ImGui::EndCombo();
                    }
                    break;
                }
            }

            if 
            (
                std::find
                (
                    uncompiledUniforms_.begin(), 
                    uncompiledUniforms_.end(), 
                    uniform
                ) == uncompiledUniforms_.end() &&
                (
                    uniform->name != uniformName0 ||
                    uniform->type != uniformType0
                )
            )
                uncompiledUniforms_.emplace_back(uniform);
            
            if 
            (
                uniform->type == vir::Shader::Variable::Type::Sampler2D &&
                uniform->name.size() > 0 &&
                uniform->getValuePtr<Resource>() != nullptr
            )
                nextUniformIsSampler2DResolution = true;

            ImGui::PopItemWidth();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (deleteRow != -1)
    {
        auto uniform = uniforms_[deleteRow];
        if 
        (
            uniform->type == Uniform::Type::Sampler2D ||
            uniform->type == Uniform::Type::SamplerCube
        )
            uniform->getValuePtr<Resource>()->unbind();
        delete uniform;
        uniforms_.erase(uniforms_.begin()+deleteRow);
        uniform = nullptr;
        deleteRow = -1;
        hasUncompiledChanges_ = true;
        app_.userActionRef() = true; // Why not?
    }
    deferredUserAction = app_.userActionRef();
}

}