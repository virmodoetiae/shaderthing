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
            rendererTargetToName[rendersTo_].c_str()
        )
    )
    {
        for(auto rendererTargetName : rendererTargetNames)
        {
            if (!ImGui::Selectable(rendererTargetName.c_str()))
                continue;
            RendersTo target = nameToRendererTarget[rendererTargetName];
            if (target != rendersTo_)
            {
                rendersTo_ = target;
                if (rendersTo_ != RendersTo::Window)
                    app_.resourceManagerRef().addLayerAsResource(this);
                else
                {
                    app_.resourceManagerRef().removeLayerAsResource(this);
                    auto window = vir::GlobalPtr<vir::Window>::instance();
                    resolutionScale_ = 1.0;
                    targetResolution_ = {window->width(), window->height()};
                }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::Text("Resolution           ");
    ImGui::SameLine();
    ImGui::PushItemWidth(selectableWidth);
    if (rendersTo_ == RendersTo::Window)
        ImGui::BeginDisabled();
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
        }
    }
    ImGui::Text("Show vertex shader   ");
    ImGui::SameLine();
    ImGui::Checkbox("##isVertexEditorVisible", &isVertexEditorVisible_);
}

//----------------------------------------------------------------------------//
// Private functions ---------------------------------------------------------//

void Layer::renderGuiMain()
{
    isGuiRendered_ = true;
    app_.layerManagerRef().setActiveGuiLayer(this);
    float fontSize = ImGui::GetFontSize();
    float time = vir::GlobalPtr<vir::Time>::instance()->outerTime();
    ImVec4 compileButtonColor = 
    {
        .5f*glm::sin(6.283f*(time/3+0.f/3))+.3f,
        .5f*glm::sin(6.283f*(time/3+1.f/3))+.3f,
        .5f*glm::sin(6.283f*(time/3+2.f/3))+.3f,
        1.f
    };
    static ImVec4 ctrlBColor = ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];

#define TEXT_EDITOR_CONTROLS(editor, editorFlag)                            \
    bool someControlsOpen(                                                  \
        uncompiledVertexEditorChanges_ ||                                   \
        uncompiledFragmentEditorChanges_ ||                                 \
        app_.findReplaceTextToolRef().isGuiOpen());                         \
    app_.findReplaceTextToolRef().renderGui();                              \
    editorFlag = editorFlag ||                                              \
        app_.findReplaceTextToolRef().findReplaceTextInEditor(editor);      \
    if (uncompiledVertexEditorChanges_ || uncompiledFragmentEditorChanges_){\
        ImGui::PushStyleColor(ImGuiCol_Button, compileButtonColor);         \
        if (ImGui::ArrowButton("##right",ImGuiDir_Right))toBeCompiled_=true;\
        ImGui::PopStyleColor();                                             \
        ImGui::SameLine();                                                  \
        ImGui::Text("Compile shader");                                      \
        ImGui::SameLine();                                                  \
        ImGui::PushStyleColor(ImGuiCol_Text, ctrlBColor);                   \
        ImGui::Text("Ctrl+B");                                              \
        ImGui::PopStyleColor();}                                            \
    if (someControlsOpen) ImGui::Separator();

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("##layerTabBar", tab_bar_flags)) //------------------
    {
        if (isVertexEditorVisible_)
            if (ImGui::BeginTabItem("Vertex shader"))
            {
                TEXT_EDITOR_CONTROLS
                (
                    vertexEditor_, 
                    uncompiledVertexEditorChanges_
                )
                vertexEditor_.Render("##vertexEditor");
                ImGui::EndTabItem();
            }
        if (ImGui::BeginTabItem("Fragment shader"))
        {
            TEXT_EDITOR_CONTROLS
            (
                fragmentEditor_, 
                uncompiledFragmentEditorChanges_
            )
            fragmentEditor_.Render("##fragmentEditor");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Uniforms"))
        {
            renderGuiUniforms();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if 
    (
        Misc::isCtrlKeyPressed(ImGuiKey_B) &&
        ImGui::GetIO().WantCaptureKeyboard
    )
        toBeCompiled_ = true;
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
        int nDefaultUniforms(defaultUniforms_.size());
        int nUniforms(uniforms_.size()+defaultUniforms_.size());
        
        // Actual rendering ----------------------------------------------------
        for (int row = 0; row < nUniforms+1; row++)
        {
            ImGui::PushID(row);
            ImGui::TableNextRow(0, 1.6*fontSize);
            int col = 0;

            if (row == nUniforms)
            {
                ImGui::TableSetColumnIndex(col++);
                ImGui::PushItemWidth(-1);
                if (ImGui::Button("Add", ImVec2(-1, 0)))
                {
                    auto uniform = new vir::Shader::Uniform();
                    uniforms_.emplace_back(uniform);
                    uniformLimits_.insert({uniform, glm::vec2({0.0f, 1.0f})});
                    uniformUsesColorPicker_.insert({uniform, false});
                }
                ImGui::PopItemWidth();
                ImGui::PopID();
                break;
            }

            bool isDefault(row < nDefaultUniforms);
            vir::Shader::Uniform* uniform = nullptr;
            bool* uniformUsesColorPicker = nullptr;
            if (row >= nDefaultUniforms)
            {
                uniform = uniforms_[row-nDefaultUniforms];
                uniformUsesColorPicker = &uniformUsesColorPicker_[uniform];
            }
            else 
                uniform = defaultUniforms_[row];
            // Column 0 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            if (row >= nDefaultUniforms)
            {
                if (ImGui::Button("Delete", ImVec2(-1, 0)))
                    deleteRow = row-nDefaultUniforms;
            }
            else if (uniform->name == "iFrame")
            {
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
                static bool& isRenderingPaused(app_.isRenderingPausedRef());
                static bool& isTimePaused(app_.isTimePausedRef());
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
            else if (uniform->name == "iTime")
            {
                std::string text = (timePaused_) ? ICON_FA_PLAY : ICON_FA_PAUSE;
                if (ImGui::Button(text.c_str(), ImVec2(-1, 0)))
                    timePaused_ = !timePaused_;
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
            else if (uniform->name == "iCameraPosition")
            {
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
            else if (uniform->name == "iCameraDirection")
            {
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
            else if (uniform->name == "iMouse")
            {
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
            if (isDefault)
                ImGui::Text(uniform->name.c_str());
            else 
                ImGui::InputText("##name", &uniform->name);
            ImGui::PopItemWidth();
            
            // Column 2 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            if (isDefault)
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
                            }
                    }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            // Column 3 --------------------------------------------------------
            glm::vec2& uLimits = uniformLimits_[uniform];
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            bool limitsChanged(false);
            if 
            (
                uniform->type != vir::Shader::Variable::Type::Bool &&
                uniform->type != vir::Shader::Variable::Type::Sampler2D &&
                uniform->type != vir::Shader::Variable::Type::SamplerCube &&
                uniform->name != "iResolution" &&
                uniform->name != "iFrame" &&
                uniform->name != "iAspectRatio" &&
                uniform->name != "iMouse"
            )
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
                    if (ImGui::Checkbox((value) ? "true" : "false", &value))
                    {
                        uniform->setValue(value);
                        if (uniform->name != "")
                            shader_->setUniformBool(uniform->name, value);
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
                    if (uniform->name == "iFrame")
                        ImGui::Text(std::to_string(std::max(frame_,0)).c_str());
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
                        if (uniform->name != "")
                            shader_->setUniformInt(uniform->name, value);
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
                        if (uniform->name != "")
                            shader_->setUniformInt(uniform->name, value);
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
                        ImGui::SmallButton("Δ"); ImGui::SameLine();
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
                        std::cout << value.x << " " << value.y << std::endl;
                        if (uniform->name != "")
                            shader_->setUniformInt2(uniform->name, value);
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
                        if (uniform->name != "")
                            shader_->setUniformInt3(uniform->name, value);
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
                    if (uniform->name == "iMouse")
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
                        if (uniform->name != "")
                            shader_->setUniformInt4(uniform->name, value);
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
                    if (uniform->name == "iAspectRatio")
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
                        if (uniform->name != "")
                            shader_->setUniformFloat(uniform->name, value);
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
                    if (uniform->name == "iResolution")
                    {
                        std::string resolution
                        (
                            std::to_string(resolution_.x)+" x "+
                            std::to_string(resolution_.y)
                        );
                        ImGui::Text(resolution.c_str());
                    }
                    else 
                    {
                        ImGui::SmallButton("Δ"); ImGui::SameLine();
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
                        if (uniform->name != "")
                            shader_->setUniformFloat2(uniform->name, value);
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
                    if (uniformUsesColorPicker != nullptr)
                    {
                        if 
                        (
                            ImGui::SmallButton
                            (
                                *uniformUsesColorPicker ? "F" : "C"
                            )
                        )
                            *uniformUsesColorPicker = !*uniformUsesColorPicker;
                        ImGui::SameLine();
                        colorPicker = *uniformUsesColorPicker;
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
                                uniform->name == "iCameraDirection"
                            );
                            if (isCameraDirection)
                                value = glm::normalize(value);
                            uniform->setValue(value);
                            if (uniform->name != "")
                                shader_->setUniformFloat3(uniform->name, value);
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
                            if (uniform->name != "")
                                shader_->setUniformFloat3(uniform->name, value);
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
                    if (uniformUsesColorPicker != nullptr)
                    {
                        if 
                        (
                            ImGui::SmallButton
                            (
                                *uniformUsesColorPicker ? "F" : "C"
                            )
                        )
                            *uniformUsesColorPicker = !*uniformUsesColorPicker;
                        ImGui::SameLine();
                        colorPicker = *uniformUsesColorPicker;
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
                            if (uniform->name != "")
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
                            if (uniform->name != "")
                                shader_->setUniformFloat4(uniform->name, value);
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
                               uniform->setValuePtr<Resource>(r);
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
                               uniform->setValuePtr<Resource>(r);
                        }
                        ImGui::EndCombo();
                    }
                    break;
                }
            }
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
            uniform->type == vir::Shader::Variable::Type::Sampler2D ||
            uniform->type == vir::Shader::Variable::Type::SamplerCube
        )
            uniform->getValuePtr<Resource>()->unbind();
        delete uniform;
        uniformUsesColorPicker_.erase(uniform);
        uniformLimits_.erase(uniform);
        uniforms_.erase(uniforms_.begin()+deleteRow);
        uniform = nullptr;
        deleteRow = -1;
    }
}

}