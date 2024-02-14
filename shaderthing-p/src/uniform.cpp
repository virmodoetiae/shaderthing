#include "shaderthing-p/include/uniform.h"
#include "shaderthing-p/include/shareduniforms.h"
#include "shaderthing-p/include/helpers.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/icons/IconsFontAwesome5.h"

namespace ShaderThing
{

bool Uniform::renderUniformsGUI
(
    SharedUniforms& sharedUniforms,
    std::vector<Uniform*>& uniforms,
    std::vector<Uniform*>& uncompiledUniforms,
    vir::Shader& shader
)
{
    const float fontSize = ImGui::GetFontSize();
    int row = 0; 
    #define START_ROW                                                       \
        ImGui::PushID(row);                                                 \
        ImGui::TableNextRow(0, 1.6*fontSize);               
    #define END_ROW                                                         \
        ImGui::PopID();                                                     \
        ++row;
    #define START_COLUMN                                                    \
        ImGui::TableSetColumnIndex(column);                                 \
        ImGui::PushItemWidth(-1);
    #define END_COLUMN                                                      \
        ++column;                                                           \
        ImGui::PopItemWidth();


    // -------------------------------------------------------------------------
    auto renderSharedUniformsGUI = 
    [&fontSize](SharedUniforms& sharedUniforms, int& row)
    {
        int column = 0;
    }; // End of renderSharedUniformsGUI lambda

    // -------------------------------------------------------------------------
    static std::string supportedUniformTypeNames[11]
    ({
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Bool],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Int],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Int2],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Int3],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Int4],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Float],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Float2],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Float3],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Float4],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Sampler2D],
        vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::SamplerCube]
    });
    auto renderUniformGUI = 
    [&fontSize]
    (
        SharedUniforms& sharedUniforms,
        Uniform* uniform,
        std::vector<Uniform*>& uncompiledUniforms,
        vir::Shader& shader,
        int& row
    )
    {
        int column = 0;
        bool managed
        (
            uniform->specialType == SpecialType::LayerAspectRatio ||
            uniform->specialType == SpecialType::LayerResolution
        );
        auto name0 = uniform->name;
        auto type0 = uniform->type;
        
        START_ROW

        START_COLUMN // Action column ------------------------------------------
        if (!managed)
        {
            if (ImGui::Button(ICON_FA_TRASH, ImVec2(-1, 0)))
                uniform->gui.markedForDeletion = true;
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                ImGui::Text("Delete this uniform");
                ImGui::EndTooltip();
            }
        }
        END_COLUMN
        
        START_COLUMN // Name column --------------------------------------------
        if (managed)
            ImGui::Text(uniform->name.c_str());
        else
            ImGui::InputText("##uniformName", &uniform->name);
        bool named(uniform->name.size() > 0);
        END_COLUMN

        START_COLUMN // Type column --------------------------------------------
        if (managed)
            ImGui::Text(vir::Shader::uniformTypeToName[uniform->type].c_str());
        else if 
        (
            ImGui::BeginCombo
            (
                "##ufSelector", 
                vir::Shader::uniformTypeToName[uniform->type].c_str()
            )
        )
        {
            for(auto uniformTypeName : supportedUniformTypeNames)
                if (ImGui::Selectable(uniformTypeName.c_str()))
                {
                    auto selectedType = 
                        vir::Shader::uniformNameToType[uniformTypeName];
                    if (selectedType != uniform->type)
                    {
                        uniform->resetValue();
                        uniform->type = selectedType;
                        uniform->gui.showBounds = 
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
        END_COLUMN

        START_COLUMN // Bounds column ------------------------------------------
        glm::vec2& bounds = uniform->gui.bounds;
        bool boundsChanged(false);
        if (uniform->gui.showBounds)
        {
            if (ImGui::Button(ICON_FA_RULER_COMBINED, ImVec2(-1, 0)))
                ImGui::OpenPopup("##uniformBounds");
            if (ImGui::BeginPopup("##uniformBounds"))
            {
                glm::vec2 bounds0(bounds);
                if (uniform->type == vir::Shader::Variable::Type::UInt)
                    bounds.x = std::max(bounds.x, 0.0f);
                float* minValue = &(glm::value_ptr(bounds)[0]);
                float* maxValue = &(glm::value_ptr(bounds)[1]);
                ImGui::Text("Minimum value ");
                ImGui::SameLine();
                float inputWidth = 6*fontSize;
                ImGui::PushItemWidth(inputWidth);
                ImGui::InputFloat
                (
                    "##minValueInput", 
                    minValue, 0.f, 0.f,
                    Helpers::getFormat(bounds0.x).c_str()
                );
                ImGui::PopItemWidth();
                ImGui::Text("Maximum value ");
                ImGui::SameLine();
                ImGui::PushItemWidth(inputWidth);
                ImGui::InputFloat
                (
                    "##maxValueInput", 
                    maxValue, 0.f, 0.f,
                    Helpers::getFormat(bounds0.y).c_str()
                );
                ImGui::PopItemWidth();
                boundsChanged = (bounds != bounds0);
                ImGui::EndPopup();
            }
        }
        END_COLUMN

        START_COLUMN // Value column -------------------------------------------
        switch(uniform->type)
        {
            case vir::Shader::Variable::Type::Bool :
            {
                auto value = uniform->getValue<bool>();
                if (ImGui::Checkbox((value) ? "true" : "false", &value))
                {
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformBool(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::UInt : //-------------------------
            {
                auto value = uniform->getValue<uint32_t>();
                value = std::max(value, (uint32_t)0);
                if (!boundsChanged)
                {
                    bounds.x = std::min((float)value, bounds.x);
                    bounds.y = std::max((float)value, bounds.y);
                }
                bool input = ImGui::SliderInt
                (
                    "##uniformSliderInt", 
                    (int*)&value, 
                    (int)(bounds.x),
                    (int)(bounds.y)
                );
                if (input || boundsChanged)
                {
                    value = std::max(value, (uint32_t)0);
                    if (boundsChanged)
                    {
                        value = std::max((float)value, bounds.x);
                        value = std::min((float)value, bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformInt(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Int : //--------------------------
            {
                auto value = uniform->getValue<int>();
                if (!boundsChanged)
                {
                    bounds.x = std::min((float)value, bounds.x);
                    bounds.y = std::max((float)value, bounds.y);
                }
                bool input(false);
                if 
                (
                    ImGui::SliderInt
                    (
                        "##iSlider", 
                        &value, 
                        (int)(bounds.x),
                        (int)(bounds.y)
                    ) || boundsChanged
                )
                {
                    if (boundsChanged)
                    {
                        value = std::max((float)value, bounds.x);
                        value = std::min((float)value, bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformInt(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Int2 : //-------------------------
            {
                auto value = uniform->getValue<glm::ivec2>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value.x, (int)bounds.x);
                    bounds.x = std::min(value.y, (int)bounds.x);
                    bounds.y = std::max(value.x, (int)bounds.y);
                    bounds.y = std::max(value.y, (int)bounds.y);
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
                        bounds.x = std::min(bounds.x, -bounds.y);
                        bounds.y = std::max(-bounds.x, bounds.y);
                        value.x = 
                            std::max
                            (
                                std::min
                                (
                                    (float)(4*delta.x*bounds.y)/maxRes, 
                                    bounds.y
                                ),
                                bounds.x
                            );
                        value.y = 
                            std::max
                            (
                                std::min
                                (
                                    (float)(4*delta.y*bounds.y)/maxRes, 
                                    bounds.y
                                ),
                                bounds.x
                            );
                        input = true;
                    }
                    bool input2 = ImGui::SliderInt2
                    (
                        "##i2Slider", 
                        glm::value_ptr(value), 
                        bounds.x,
                        bounds.y
                    );
                    input = input || input2;
                }
                if (input || boundsChanged)
                {
                    if (boundsChanged)
                    {
                        value.x = std::max(value.x, (int)bounds.x);
                        value.x = std::min(value.x, (int)bounds.y);
                        value.y = std::max(value.y, (int)bounds.x);
                        value.y = std::min(value.y, (int)bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformInt2(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Int3 : //-------------------------
            {
                auto value = uniform->getValue<glm::ivec3>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value.x, (int)bounds.x);
                    bounds.x = std::min(value.y, (int)bounds.x);
                    bounds.x = std::min(value.z, (int)bounds.x);
                    bounds.y = std::max(value.x, (int)bounds.y);
                    bounds.y = std::max(value.y, (int)bounds.y);
                    bounds.y = std::max(value.z, (int)bounds.y);
                }
                if 
                (
                    ImGui::SliderInt3
                    (
                        "##i3Slider", 
                        glm::value_ptr(value), 
                        bounds.x,
                        bounds.y
                    )
                )
                {
                    if (boundsChanged)
                    {
                        value.x = std::max(value.x, (int)bounds.x);
                        value.x = std::min(value.x, (int)bounds.y);
                        value.y = std::max(value.y, (int)bounds.x);
                        value.y = std::min(value.y, (int)bounds.y);
                        value.z = std::max(value.z, (int)bounds.x);
                        value.z = std::min(value.z, (int)bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformInt3
                        (
                            uniform->name, 
                            value
                        );
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Int4 : //-------------------------
            {
                auto value = uniform->getValue<glm::ivec4>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value.x, (int)bounds.x);
                    bounds.x = std::min(value.y, (int)bounds.x);
                    bounds.x = std::min(value.z, (int)bounds.x);
                    bounds.x = std::min(value.w, (int)bounds.x);
                    bounds.y = std::max(value.x, (int)bounds.y);
                    bounds.y = std::max(value.y, (int)bounds.y);
                    bounds.y = std::max(value.z, (int)bounds.y);
                    bounds.y = std::max(value.w, (int)bounds.y);
                }
                if 
                (
                    ImGui::SliderInt4
                    (
                        "##i4Slider", 
                        glm::value_ptr(value), 
                        bounds.x,
                        bounds.y
                    )
                )
                {
                    if (boundsChanged)
                    {
                        value.x = std::max(value.x, (int)bounds.x);
                        value.x = std::min(value.x, (int)bounds.y);
                        value.y = std::max(value.y, (int)bounds.x);
                        value.y = std::min(value.y, (int)bounds.y);
                        value.z = std::max(value.z, (int)bounds.x);
                        value.z = std::min(value.z, (int)bounds.y);
                        value.w = std::max(value.z, (int)bounds.x);
                        value.w = std::min(value.z, (int)bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformInt4(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Float : //------------------------
            {
                auto value = uniform->getValue<float>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value, bounds.x);
                    bounds.y = std::max(value, bounds.y);
                }
                bool input(false);
                if 
                (
                    uniform->specialType == 
                        Uniform::SpecialType::WindowAspectRatio ||
                    uniform->specialType == 
                        Uniform::SpecialType::LayerAspectRatio
                )
                    ImGui::Text("%.3f", value);
                else
                    input = ImGui::SliderFloat
                    (
                        "##fSlider", 
                        &value, 
                        bounds.x,
                        bounds.y,
                        uniform->specialType == Uniform::SpecialType::Time ?
                        "%.3f" : Helpers::getFormat(value).c_str()
                    );
                if (input || boundsChanged)
                {
                    if (boundsChanged)
                    {
                        value = std::max(value, bounds.x);
                        value = std::min(value, bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformFloat(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Float2 : //-----------------------
            {
                auto value = uniform->getValue<glm::vec2>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value.x, bounds.x);
                    bounds.x = std::min(value.y, bounds.x);
                    bounds.y = std::max(value.x, bounds.y);
                    bounds.y = std::max(value.y, bounds.y);
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
                    auto ivalue = uniform->getValue<glm::ivec2>();
                    ImGui::Text
                    (
                        "%d x %d",
                        ivalue.x, ivalue.y
                    );
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
                        bounds.x = std::min(bounds.x, -bounds.y);
                        bounds.y = std::max(-bounds.x, bounds.y);
                        value.x = 
                            std::max
                            (
                                std::min
                                (
                                    (float)(4*delta.x*bounds.y)/maxRes, 
                                    bounds.y
                                ),
                                bounds.x
                            );
                        value.y = 
                            std::max
                            (
                                std::min
                                (
                                    (float)(4*delta.y*bounds.y)/maxRes, 
                                    bounds.y
                                ),
                                bounds.x
                            );
                        input = true;
                    }
                    bool input2 = ImGui::SliderFloat2
                    (
                        "##f2Slider", 
                        glm::value_ptr(value), 
                        bounds.x,
                        bounds.y,
                        Helpers::getFormat(value).c_str()
                    );
                    input = input || input2;
                }
                if (input || boundsChanged)
                {
                    if (boundsChanged)
                    {
                        value.x = std::max(value.x, bounds.x);
                        value.x = std::min(value.x, bounds.y);
                        value.y = std::max(value.y, bounds.x);
                        value.y = std::min(value.y, bounds.y);
                    }
                    uniform->setValue(value);
                    if (named)
                    {
                        shader.setUniformFloat2(uniform->name, value);
                        sharedUniforms.setUserAction(true);
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Float3 : //-----------------------
            {
                auto value = uniform->getValue<glm::vec3>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value.x, bounds.x);
                    bounds.x = std::min(value.y, bounds.x);
                    bounds.x = std::min(value.z, bounds.x);
                    bounds.y = std::max(value.x, bounds.y);
                    bounds.y = std::max(value.y, bounds.y);
                    bounds.y = std::max(value.z, bounds.y);
                }
                bool colorPicker(false);
                if 
                (
                    ImGui::SmallButton
                    (
                        uniform->gui.usesColorPicker ? 
                        ICON_FA_SLIDERS_H : 
                        ICON_FA_PAINT_BRUSH
                    )
                )
                    uniform->gui.usesColorPicker = 
                        !uniform->gui.usesColorPicker;
                ImGui::SameLine();
                colorPicker = uniform->gui.usesColorPicker;
                if (!colorPicker)
                {
                    uniform->gui.showBounds = true;
                    if 
                    (
                        ImGui::SliderFloat3
                        (
                            "##f3Slider", 
                            glm::value_ptr(value), 
                            bounds.x,
                            bounds.y,
                            Helpers::getFormat(value).c_str()
                        ) || boundsChanged
                    )
                    {
                        if (boundsChanged)
                        {
                            value.x = std::max(value.x, bounds.x);
                            value.x = std::min(value.x, bounds.y);
                            value.y = std::max(value.y, bounds.x);
                            value.y = std::min(value.y, bounds.y);
                            value.z = std::max(value.z, bounds.x);
                            value.z = std::min(value.z, bounds.y);
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
                            shader.setUniformFloat3(uniform->name, value);
                            sharedUniforms.setUserAction(true);
                        }
                    }
                }
                else
                {
                    uniform->gui.showBounds = false;
                    if 
                    (
                        ImGui::ColorEdit3
                        (
                            "##uniformColorEdit3", 
                            glm::value_ptr(value)
                        )
                    )
                    {
                        bounds.x = 0.0;
                        bounds.y = 1.0;
                        uniform->setValue(value);
                        if (named)
                        {
                            shader.setUniformFloat3(uniform->name, value);
                            sharedUniforms.setUserAction(true);
                        }
                    }
                }
                break;
            }
            case vir::Shader::Variable::Type::Float4 : //-----------------------
            {
                auto value = uniform->getValue<glm::vec4>();
                if (!boundsChanged)
                {
                    bounds.x = std::min(value.x, bounds.x);
                    bounds.x = std::min(value.y, bounds.x);
                    bounds.x = std::min(value.z, bounds.x);
                    bounds.x = std::min(value.w, bounds.x);
                    bounds.y = std::max(value.x, bounds.y);
                    bounds.y = std::max(value.y, bounds.y);
                    bounds.y = std::max(value.z, bounds.y);
                    bounds.y = std::max(value.w, bounds.y);
                }
                bool colorPicker(false);
                if 
                (
                    ImGui::SmallButton
                    (
                        uniform->gui.usesColorPicker ? 
                        ICON_FA_SLIDERS_H : 
                        ICON_FA_PAINT_BRUSH
                    )
                )
                    uniform->gui.usesColorPicker = 
                        !uniform->gui.usesColorPicker;
                ImGui::SameLine();
                colorPicker = uniform->gui.usesColorPicker;
                if (!colorPicker)
                {
                    uniform->gui.showBounds = true;
                    if 
                    (
                        ImGui::SliderFloat4
                        (
                            "##f4Slider", 
                            glm::value_ptr(value), 
                            bounds.x,
                            bounds.y,
                            Helpers::getFormat(value).c_str()
                        ) || boundsChanged
                    )
                    {
                        if (boundsChanged)
                        {
                            value.x = std::max(value.x, bounds.x);
                            value.x = std::min(value.x, bounds.y);
                            value.y = std::max(value.y, bounds.x);
                            value.y = std::min(value.y, bounds.y);
                            value.z = std::max(value.z, bounds.x);
                            value.z = std::min(value.z, bounds.y);
                            value.w = std::max(value.w, bounds.x);
                            value.w = std::min(value.w, bounds.y);
                        }
                        uniform->setValue(value);
                        if (named)
                            shader.setUniformFloat4(uniform->name, value);
                    }
                }
                else
                {
                    if (uniform->gui.showBounds)
                        uniform->gui.showBounds = false;
                    if 
                    (
                        ImGui::ColorEdit4
                        (
                            "##uniformColorEdit4", 
                            glm::value_ptr(value)
                        )
                    )
                    {
                        bounds.x = 0.0;
                        bounds.y = 1.0;
                        uniform->setValue(value);
                        if (named)
                        {
                            shader.setUniformFloat4(uniform->name, value);
                            sharedUniforms.setUserAction(true);
                        }
                    }
                }
                break;
                // Missing Sampler2D/Cubemap, will be added once I implement 
                // the Resource class
            }
        }
        END_COLUMN
        
        END_ROW

        if 
        (
            std::find
            (
                uncompiledUniforms.begin(), 
                uncompiledUniforms.end(), 
                uniform
            ) == uncompiledUniforms.end() &&
            (
                uniform->name != name0 ||
                uniform->type != type0
            )
        )
            uncompiledUniforms.emplace_back(uniform);
    }; // End of renderUniformGUI lambda

    //--------------------------------------------------------------------------
    auto renderAddUniformButton = 
    [&fontSize]
    (
        std::vector<Uniform*>& uniforms, 
        std::vector<Uniform*>& uncompiledUniforms, 
        int& row
    )
    {
        int column = 0;
        START_ROW
        START_COLUMN
        if (ImGui::Button(ICON_FA_PLUS, ImVec2(-1, 0)))
        {
            auto uniform = new Uniform();
            uniforms.emplace_back(uniform);
            uncompiledUniforms.emplace_back(uniform);
        }
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            ImGui::Text("Add new uniform");
            ImGui::EndTooltip();
        }
        END_COLUMN
        END_ROW
    }; // End of addNewUniform lambda

    //--------------------------------------------------------------------------
    bool shaderRequiresRecompilation(false);
    if 
    (
        ImGui::BeginTable
        (
            "##uniformTable", 
            5, 
            ImGuiTableFlags_BordersV | 
            ImGuiTableFlags_BordersOuterH |
            ImGuiTableFlags_SizingFixedFit
        )
    )
    {
        ImGui::TableSetupColumn("##actions", 0, 4*fontSize);
        ImGui::TableSetupColumn("Name", 0, 10*fontSize);
        ImGui::TableSetupColumn("Type", 0, 5*fontSize);
        ImGui::TableSetupColumn("Bounds", 0, 4*fontSize);
        ImGui::TableSetupColumn
        (
            "Value", 
            0, 
            ImGui::GetContentRegionAvail().x
        );
        ImGui::TableHeadersRow();

        renderSharedUniformsGUI(sharedUniforms, row);

        shader.bind(); // Must be bound, else uniforms in renderUniformGUI will
                       // not be set
        
        for(auto uniform : uniforms)
        {
            renderUniformGUI
            (
                sharedUniforms, // Required to set iUserAction if user changes 
                                // anything
                uniform,
                uncompiledUniforms,
                shader,
                row
            );
            if (uniform->gui.markedForDeletion)
                shaderRequiresRecompilation = true;
        }
        renderAddUniformButton(uniforms, uncompiledUniforms, row);

        // Remove uniforms marked for deletion
        if (shaderRequiresRecompilation)
        {
            uniforms.erase
            (
                std::remove_if
                (
                    uniforms.begin(),
                    uniforms.end(),
                    [](const Uniform* uniform)
                    {
                        return uniform->gui.markedForDeletion;
                    }
                )
            );
        }

        // Check if all uncompiled uniforms have been named (and thus now
        // eligible for compilation)
        if (uncompiledUniforms.size() > 0)
        {
            bool allUniformsAreNamed = true;
            for (auto* uniform : uncompiledUniforms)
            {
                if (uniform->name.size()>0)
                    continue;
                allUniformsAreNamed = false;
                break;
            }
            if (allUniformsAreNamed)
                shaderRequiresRecompilation = true;
        }

        ImGui::EndTable();
    }
    return shaderRequiresRecompilation;
}

}