#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/corelogic.h"
#include "shaderthing/include/gui.h"
#include "shaderthing/include/helpers.h"
#include "shaderthing/include/oo/statusbar.h"
#include "shaderthing/include/oo/texteditor.h"
#include "shaderthing/include/oo/uniform.h"
#include "shaderthing/include/structs.h"
#include "vir/include/vir.h"
#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_extensions.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/icons/IconsFontAwesome5.h"

namespace ShaderThing
{

typedef Uniform::Type Type;
typedef Uniform::ManagedType ManagedType;

namespace GUI
{

//----------------------------------------------------------------------------//
// General -------------------------------------------------------------------//
//----------------------------------------------------------------------------//

void renderControlPanel(AppData& appData)
{
    // Move to deferred update
    //font_.checkLoadJapaneseAndOrSimplifiedChinese();
    
    vir::ImGuiRenderer::newFrame();
    
    ImGui::SetNextWindowSize(ImVec2(750,750), ImGuiCond_FirstUseEver);
    static ImGuiWindowFlags flags
    (
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse
    );
    ImGui::Begin(appData.controlPanelTitle.c_str(), NULL, flags);

    // Refresh icon if needed
    static bool isIconSet(false);
    static bool isWindowDocked(ImGui::IsWindowDocked());
    if (!isIconSet || isWindowDocked != ImGui::IsWindowDocked())
    {
        isIconSet = vir::ImGuiRenderer::setWindowIcon
        (
            appData.controlPanelTitle.c_str(), 
            ByteData::Icon::sTIconData, 
            ByteData::Icon::sTIconSize,
            false
        );
        isWindowDocked = ImGui::IsWindowDocked();
    }
    
    renderMenuBar(appData);
    renderLayersTabBar(appData);

    ImGui::End();
    
    vir::ImGuiRenderer::render();
}

//----------------------------------------------------------------------------//

void renderMenuBar(AppData& appData)
{
    bool windowIconified = vir::Window::instance()->iconified();
    bool newProjectConfirmation = false;
    bool shadersRequireRecompilation = false;
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N", nullptr, !windowIconified))
                newProjectConfirmation = true;
            if (ImGui::MenuItem("Load", "Ctrl+O", nullptr, !windowIconified))
                {}//setProjectAction(Project::Action::Load, project_, fileDialog_);
            if (ImGui::MenuItem("Save", "Ctrl+S"))
                {}//setProjectAction(Project::Action::Save, project_, fileDialog_);
            if (ImGui::MenuItem("Save as", "Ctrl+Shift+S"))
                {}//setProjectAction(Project::Action::SaveAs,project_,fileDialog_);
            ImGui::Separator();
            if (ImGui::BeginMenu("Export"))
            {
                //exporter_->renderGui(*sharedUniforms_, layers_);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Properties"))
        {
            if (ImGui::BeginMenu("Window", !vir::Window::instance()->iconified()))
            {
                auto& su = *(appData.sharedUniforms);
                ImGui::Text("Resolution         ");
                ImGui::SameLine();
                ImGui::PushItemWidth(8.0*ImGui::GetFontSize());
                glm::ivec2 resolution(su.iResolution);
                if 
                (
                    ImGui::InputInt2
                    (
                        "##windowResolution", 
                        glm::value_ptr(resolution)
                    )
                )
                    setWindowResolution
                    (
                        appData,
                        resolution,
                        false
                    );
                ImGui::PopItemWidth();
                // TODO
                
                auto window = vir::Window::instance();
                ImGui::Text("VSync              ");
                ImGui::SameLine();
                if 
                (
                    ImGui::Checkbox
                    (
                        "##windowVSync", 
                        &appData.rendering.isVSyncEnabled
                    )
                )
                    window->setVSync(appData.rendering.isVSyncEnabled);

                ImGui::Text("GUI fps multiplier ");
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                {
                    ImGui::Text(
R"(Frame rate multiplier of the graphical user interface (GUI). By default, the 
GUI frame rate is tied to the shader rendering frame rate in the main window. 
When rendering computationally intensive shaders, the GUI frame rate is affected
as well, resulting in a worsened user experience. Set this multiplier to values
larger than one to recover the GUI frame rate, at the expense, however, of a
further reduction of the shader rendering frame rate)");
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                ImGui::PushItemWidth(8.f*ImGui::GetFontSize());
                int nRenderingTiles = appData.rendering.nTiles;
                if (appData.rendering.isPaused)
                    ImGui::BeginDisabled();
                if (ImGui::InputInt("##nRenderingTiles", &nRenderingTiles))
                {
                    setRenderingTiles(appData, nRenderingTiles);
                }
                if (appData.rendering.isPaused)
                    ImGui::EndDisabled();
                
                ImGui::Text("Pause render below ");
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                {
                    ImGui::Text(
R"(Frame rate of the graphical user interface (GUI) below which shader rendering
is paused, to prevent e.g., making the app unresponsive should the shader(s) be 
accidentally made too computationally intensive. This feature is disabled during 
project exports)");
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                ImGui::PushItemWidth(5.0*ImGui::GetFontSize());
                if 
                (
                    ImGui::InputFloat
                    (
                        "##maxLowFps", 
                        &appData.rendering.lowerFpsLimit, 
                        0.f, 
                        0.f, 
                        "%.1f"
                    )
                )
                    appData.rendering.lowerFpsLimit = 
                        std::max(appData.rendering.lowerFpsLimit, 0.f);
                ImGui::SameLine();
                ImGui::PopItemWidth();
                ImGui::Text("fps");

                if 
                (
                    ImGui::Button
                    (
                        !appData.rendering.isPaused ? 
                        "Pause rendering" : "Resume rendering", 
                        ImVec2(-1, 0)
                    )
                )
                    toggleRenderingPaused(appData, false);

                if (ImGui::Button("Capture mouse cursor", ImVec2(-1, 0)))
                    setMouseCaptured(appData, true);

                ImGui::EndMenu();
            }

            for (auto& layer : appData.layers)
                renderLayerMenuItem(layer, appData);
            /* TODO
            ImGui::Separator();
            Layer::renderShaderLanguangeExtensionsMenuGui
            (
                layers_, 
                *sharedUniforms_
            );
            */
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Resources"))
        {
            // TODO
            renderResourcesMenuItem(appData);
            shadersRequireRecompilation = 
                appData.sharedStorage->renderMenuItemGui();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Find"))
        {
            TextEditor::renderFindReplaceToolMenuGui();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Preferences"))
        {
            /* TODO
            font_.renderMenuItemGui();
            project_.renderAutoSaveMenuItemGui();
            */
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            /* TODO
            CodeRepository::renderMenuItemGui();
            if (ImGui::BeginMenu("Examples"))
            {
                Examples::renderGui(project_.exampleToBeLoaded);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("About ShaderThing"))
            {
                About::renderGui();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("System info"))
            {
                ImGui::Text("%s", "Graphics card in use:");
                ImGui::SameLine();
                ImGui::Text
                (
                    "%s", 
                    vir::Layer::Rendering::instance()->
                    deviceName().c_str()
                );
                ImGui::Text("%s", "Graphics context:    ");
                ImGui::SameLine();
                ImGui::Text
                (
                    "%s", 
                    vir::Window::instance()->
                    context()->name().c_str()
                );
                ImGui::EndMenu();
            }
            */
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // TODO
    if (Resource::gui.isDetachedFromControlPanel)
        renderResourcesTable(appData);
    if (appData.sharedStorage->isGuiDetachedFromMenu())
        shadersRequireRecompilation = 
            appData.sharedStorage->renderGui();
    /* TODO
    if (CodeRepository::isDetachedFromMenu)
        CodeRepository::renderGui();
    
    if (project_.exampleToBeLoaded != nullptr)
        project_.action = Project::Action::LoadExample;

    if (project_.action == Project::Action::None)
    {
        if (Helpers::isCtrlKeyPressed(ImGuiKey_N) && !windowIconified)
            newProjectConfirmation = true;
        else if (Helpers::isCtrlKeyPressed(ImGuiKey_O) && !windowIconified)
            setProjectAction(Project::Action::Load, project_, fileDialog_);
        else if (Helpers::isCtrlShiftKeyPressed(ImGuiKey_S))
            setProjectAction(Project::Action::SaveAs, project_, fileDialog_);
        else if (Helpers::isCtrlKeyPressed(ImGuiKey_S))
            setProjectAction(Project::Action::Save, project_, fileDialog_);
    }
    */

    if (shadersRequireRecompilation)
    {
        for (auto& layer : appData.layers)
        {
            compileShader(layer, appData);
        }
    }

    if (newProjectConfirmation)
        ImGui::OpenPopup("New project confirmation");
    if 
    (
        ImGui::BeginPopupModal
        (
            "New project confirmation", 
            nullptr, 
            ImGuiWindowFlags_NoResize
        )
    )
    {
        ImGui::Text("Are you sure you want to start a new project?");
        ImGui::Text("Any unsaved edits to the current project will be lost!");
        if (ImGui::Button("Confirm"))
        {
            //setProjectAction(Project::Action::New, project_, fileDialog_);
            appData.deferredActionBuffer.add
            (
                [&appData]()
                {
                    initialize(appData);
                }
            );
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------//

void renderLayerMenuItem(UPtr<Layer>& layer, AppData& appData)
{
    if
    (
        ImGui::BeginMenu
        (
            ("Layer ["+layer->name+"]###"+layer->imGuiMenuId).c_str()
        )
    )
    {
        auto& rendering = layer->rendering;
        const float fontSize(ImGui::GetFontSize());
        const float entryWidth(14*fontSize);
        ImGui::Text("Name                 ");
        ImGui::SameLine();
        static std::unique_ptr<char[]> label(new char[24]);
        std::sprintf(label.get(), "##layer%dInputText", layer->id);
        ImGui::PushItemWidth(entryWidth);
        if (ImGui::InputText(label.get(), &layer->name))
        {
            Helpers::enforceUniqueName
            (
                layer->name,
                appData.layers,
                layer.get()
            );
        }
        ImGui::PopItemWidth();
        
        static std::map<Layer::Rendering::Target, const char*> 
        renderTargetToName
        {
            {
                Layer::Rendering::Target::InternalFramebufferAndWindow, 
                "Framebuffer & window"
            },
            {Layer::Rendering::Target::InternalFramebuffer, "Framebuffer"},
            {Layer::Rendering::Target::Window, "Window"}
        };
        ImGui::Text("Render target        ");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        if 
        (
            ImGui::BeginCombo
            (
                "##renderingTarget", 
                renderTargetToName.at(rendering.target)
            )
        )
        {
            for(auto entry : renderTargetToName)
            {
                if (!ImGui::Selectable(entry.second))
                    continue;
                auto target = entry.first;
                if (target != rendering.target)
                {
                    rendering.target = target;
                    if (rendering.target != Layer::Rendering::Target::Window)
                        addLayerToResources
                        (
                            layer,
                            appData.resources
                        );
                    else
                        removeLayerFromResources
                        (
                            layer,
                            appData.resources
                        );
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        if 
        (
            rendering.target == Layer::Rendering::Target::Window || 
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
                layer->isAspectRatioBoundToWindow ? 
                " " ICON_FA_LOCK " " : 
                " " ICON_FA_LOCK_OPEN " "
            )
        )
        {
            layer->isAspectRatioBoundToWindow = 
                !layer->isAspectRatioBoundToWindow;
            if (layer->isAspectRatioBoundToWindow)
            {
                auto window = vir::Window::instance();
                glm::ivec2 resolution = {window->width(), window->height()};
                setLayerResolution
                (
                    layer, 
                    resolution, 
                    appData.rendering.isTiledRenderingEnabled, 
                    false
                );
            }
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text(
                layer->isAspectRatioBoundToWindow ?
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
        glm::ivec2 resolution = layer->resolution;
        std::sprintf(label.get(), "##layer%dResolution", layer->id);
        if (ImGui::InputInt2(label.get(), glm::value_ptr(resolution)))
            setLayerResolution
            (
                layer, 
                resolution, 
                appData.rendering.isTiledRenderingEnabled, 
                false,
                true
            );
        ImGui::PopItemWidth();
        ImGui::Text("Auto-resize mode     ");
        ImGui::SameLine();
        if
        (
            ImGui::Button
            (
                layer->rescaleWithWindow ?
                "Rescale on window resize" :
                "Do not auto-resize",
                {-1, 0}
            )
        )
            layer->rescaleWithWindow = !layer->rescaleWithWindow;
        if 
        (
            rendering.target == Layer::Rendering::Target::Window || 
            vir::Window::instance()->iconified()
        )
            ImGui::EndDisabled();
    
        if (rendering.target != Layer::Rendering::Target::Window)
        {
            ImGui::SeparatorText("Framebuffer settings");
            renderLayerFramebufferSettings(layer, appData);

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

void renderLayerFramebufferSettings
(
    Layer* layer,
    AppData& appData
)
{
    const float entryWidth(14*ImGui::GetFontSize());
    ImGui::Text("Internal data format ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    Layer::Rendering& rendering = layer->rendering;
    if 
    (
        ImGui::BeginCombo
        (
            "##layerInternalFormatCombo",
            vir::TextureBuffer::internalFormatToName.at
            (
                rendering.backFramebuffer->
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
                rebuildLayerFramebuffers
                (
                    layer,
                    internalFormat,
                    layer->resolution,
                    appData.rendering.isTiledRenderingEnabled
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
    if (rendering.backFramebuffer != nullptr)
    {
        selectedWrapModeX = vir::TextureBuffer::wrapModeToName.at
        (
            rendering.backFramebuffer->colorBufferWrapMode(0)
        );
        selectedWrapModeY = vir::TextureBuffer::wrapModeToName.at
        (
            rendering.backFramebuffer->colorBufferWrapMode(1)
        );
        selectedMagFilterMode = 
            vir::TextureBuffer::filterModeToName.at
            (
                rendering.backFramebuffer->colorBufferMagFilterMode()
            );
        selectedMinFilterMode = 
            vir::TextureBuffer::filterModeToName.at
            (
                rendering.backFramebuffer->colorBufferMinFilterMode()
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
        ) && rendering.backFramebuffer != nullptr
    )
    {
        for (auto entry : vir::TextureBuffer::wrapModeToName)
        {
            if (ImGui::Selectable(entry.second.c_str()))
                setLayerFramebufferWrapMode(layer, 0, entry.first);
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
        ) && rendering.backFramebuffer != nullptr
    )
    {
        for (auto entry : vir::TextureBuffer::wrapModeToName)
        {
            if (ImGui::Selectable(entry.second.c_str()))
                setLayerFramebufferWrapMode(layer, 1, entry.first);
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
        ) && rendering.backFramebuffer != nullptr
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
                setLayerFramebufferMagFilterMode(layer, entry.first);
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
        ) && rendering.backFramebuffer != nullptr
    )
    {
        for (auto entry : vir::TextureBuffer::filterModeToName)
        {
            if (ImGui::Selectable(entry.second.c_str()))
                setLayerFramebufferMinFilterMode(layer, entry.first);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
}

//----------------------------------------------------------------------------//

void renderLayersTabBar(AppData& appData)
{
    auto& layers = appData.layers;
    static bool compilationErrors(false);
    static bool anyUncompiledEdits(false);
    if (appData.rendering.toggles.requestFullRecompilation)
    {
        for (auto& layer : layers)
        {
            layer->hasUncompiledEdits = true;
        }
        appData.rendering.toggles.requestFullRecompilation = false;
    }
    if (anyUncompiledEdits || compilationErrors) // Render compilation button 
    {
        float time = vir::Window::instance()->time()->outerTime();
        ImVec4 compileButtonColor = 
        {
            .5f*glm::sin(6.283f*(time/3+0.f/3))+.3f,
            .5f*glm::sin(6.283f*(time/3+1.f/3))+.3f,
            .5f*glm::sin(6.283f*(time/3+2.f/3))+.3f,
            1.f
        };
        ImGui::PushStyleColor(ImGuiCol_Button, compileButtonColor);
        if 
        (
            ImGui::ArrowButton("##right",ImGuiDir_Right) ||
            Helpers::isCtrlKeyPressed(ImGuiKey_B)
        )
        {
            ImGui::SetTooltip("Compiling project shaders...");
            for (auto& layer : layers)
            {
                compileShader(layer, appData);
            }
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("Compile all shaders");
        ImGui::SameLine();
        static ImVec4 grayColor = 
                ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
        ImGui::PushStyleColor(ImGuiCol_Text, grayColor);
        ImGui::Text("Ctrl+B");
        ImGui::PopStyleColor();
    }

    // Render list of compilation errors with formatting -----------------------
    bool errorColorPushed = false;
    if (compilationErrors)
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, {1,0,0,1});
        errorColorPushed = true;
        ImGui::Text("Compilation errors in:");
    }
    compilationErrors = false;
    anyUncompiledEdits = false;
    const auto& sharedErrors // First render errors in shared source -----------
    (
        appData.sharedSourceEditor.getErrorMarkers()
    );
    if (sharedErrors.size() > 0)
    {
        compilationErrors = true;
        ImGui::Bullet(); ImGui::Text("Shared source");
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            for (auto& error : sharedErrors)
            {
                // First is line no., second is actual error text
                std::string errorText = 
                    "Line "+std::to_string(error.first)+": "+error.second;
                ImGui::Text(errorText.c_str());
            }
            ImGui::EndTooltip();
        }
    }
    for (auto& layer : layers) // Second, render layer-specific errors in either
                               // source header or editable source -------------
    {
        const auto& sourceErrors(layer->sourceEditor.getErrorMarkers());
        if (sourceErrors.size() > 0 || layer->headerErrors.size() > 0)
        {
            compilationErrors = true;
            ImGui::Bullet(); ImGui::Text(layer->name.c_str());
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                if (layer->headerErrors.size() > 0)
                    ImGui::Text(layer->headerErrors.c_str());
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
        if
        (
            layer->sourceEditor.isTextChanged() || 
            appData.sharedSourceEditor.isTextChanged()
        )
            layer->hasUncompiledEdits = true;
        if 
        (
            appData.sharedSourceEditor.isTextChanged() ||
            layer->hasUncompiledEdits
        )
            anyUncompiledEdits = true;
    }
    if (compilationErrors)
        ImGui::Separator();
    if (errorColorPushed)
        ImGui::PopStyleColor();

    // Actual layers tab bar ---------------------------------------------------
    static bool reorderable = true;
    if 
    (
        ImGui::BeginTabBar
        (
            "##layersTabBar", 
            reorderable ? 
            ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable :
            ImGuiTabBarFlags_AutoSelectNewTabs
        )
    )
    {
        reorderable = true;
        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
            createNewLayer(appData);
        auto tabBar = ImGui::GetCurrentTabBar();
        std::pair<unsigned int, unsigned int> swap {0,0};
        for (int i = 0; i < (int)layers.size(); i++)
        {
            bool open = true;
            auto& layer = layers[i];
            std::string tabLabel = layer->name+"###"+layer->imGuiTabId;
            if
            (
                ImGui::BeginTabItem
                (
                    tabLabel.c_str(), 
                    &open, 
                    layers.size() == 1 ? ImGuiTabItemFlags_NoCloseButton : 0
                )
            )
            {
                renderLayerTab(layer, appData);
                ImGui::EndTabItem();
            }
            if (!open) // I.e., if 'x' is pressed to delete the tab
            {
                ImGui::OpenPopup("Layer deletion confirmation");
                layer->isDeletionConfirmationPending = true;
                // For some reason, when the 'x' is pressed, the tab in question
                // is moved 
                reorderable = false;
            }
            if 
            (
                layer->isDeletionConfirmationPending && 
                ImGui::BeginPopupModal
                (
                    "Layer deletion confirmation",
                    0,
                    ImGuiWindowFlags_NoResize
                )
            )
            {
                ImGui::Text
                (
                    "Are you sure you want to delete '%s'?", 
                    layer->name.c_str()
                );
                ImGui::Text("This action cannot be undone!");
                if (ImGui::Button("Delete"))
                {
                    appData.deferredActionBuffer.add
                    (
                        [&layer, &layers]()
                        {
                            layers.erase
                            (
                                std::remove
                                (
                                    layers.begin(), 
                                    layers.end(), 
                                    layer
                                ), 
                                layers.end()
                            );
                        }
                    );
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    layer->isDeletionConfirmationPending = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
                continue;
            }
            auto tab = ImGui::TabBarFindTabByID
            (
                tabBar, 
                ImGui::GetID(tabLabel.c_str())
            );
            int order = std::min
            (
                ImGui::TabBarGetTabOrder(tabBar, tab),
                (int)layers.size()-1
            );
            if (order != i)
                swap = {order, i};
        }
        if (swap.first != swap.second)
        {
            const auto d1 = layers[swap.first]->depth;
            const auto d2 = layers[swap.second]->depth;
            std::swap
            (
                layers[swap.first], 
                layers[swap.second]
            );
            setLayerDepth(layers[swap.first], d1);
            setLayerDepth(layers[swap.second], d2);
            swap = {0, 0};
        }
        ImGui::EndTabBar();
    }

    // Check if layers framebuffers should be cleared as a consequence of
    // a rendering restart. This flag is set in the lambda
    // Uniform::renderUniformsTab::renderSharedUniformsGui eventually called by
    // renderTabBarGui
    if (appData.rendering.toggles.restartRendering)
    {
        for (auto& layer : layers)
        {
            layer->rendering.framebufferA->clearColorBuffer();
            layer->rendering.framebufferB->clearColorBuffer();
        }
        appData.rendering.toggles.restartRendering = false;
    }
}

//----------------------------------------------------------------------------//

void renderLayerTab(UPtr<Layer>& layer,AppData& appData)
{
    static unsigned int gActiveTabId = 0;
    static unsigned int gActiveLayerId = 0;
    bool layerChanged = (gActiveLayerId != layer->id);
    if (layerChanged)
        gActiveLayerId = layer->id;
    if (ImGui::BeginTabBar("##layerTabBar"))
    {
        if (layerChanged && layer->activeGuiTabId != gActiveTabId)
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
            bool headerErrors(layer->headerErrors.size() > 0);
            bool madeReplacements = 
                layer->sourceEditor.renderFindReplaceToolGui();
            layer->hasUncompiledEdits = 
                layer->hasUncompiledEdits || madeReplacements;
            if (ImGui::TreeNode("Header"))
            {
                float indent(layer->sourceEditor.getLineIndexColumnWidth());
                ImGui::Unindent(); // Remove indent from Header TreeNode
                ImGui::Indent(indent);
                ImGui::PushStyleColor
                (
                    ImGuiCol_Text, 
                    ImGui::GetStyle().Colors[ImGuiCol_TextDisabled] // Gray
                );
                ImGui::Text(layer->sourceHeader.c_str());
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
                    ImGui::Text(layer->headerErrors.c_str());
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
            layer->sourceEditor.renderGui("##sourceEditor");
            gActiveTabId = 0;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Shared source"))
        {
            bool madeReplacements = 
                appData.sharedSourceEditor.renderFindReplaceToolGui();
            layer->hasUncompiledEdits = 
                layer->hasUncompiledEdits || madeReplacements;
            appData.sharedSourceEditor.renderGui("##sharedSourceEditor");
            gActiveTabId = 1;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Uniforms"))
        {
            renderUniformsTab
            (
                layer,
                appData
            );
            gActiveTabId = 2;
            ImGui::EndTabItem();
        }
        layer->activeGuiTabId = gActiveTabId;
        ImGui::EndTabBar();
    }
}

#define START_ROW(row, column)                                          \
    ImGui::PushID(row);                                                 \
    ImGui::TableNextRow(0, 1.6*ImGui::GetFontSize());                   \
    column = 0;
#define END_ROW(row)                                                    \
    ImGui::PopID();                                                     \
    ++row;
#define START_COLUMN(column)                                            \
    ImGui::TableSetColumnIndex(column);                                 \
    ImGui::PushItemWidth(-1);
#define END_COLUMN(column)                                              \
    ++column;                                                           \
    ImGui::PopItemWidth();
#define NEXT_COLUMN(column)                                             \
    ImGui::TableSetColumnIndex(column++);

//----------------------------------------------------------------------------//
// Uniforms ------------------------------------------------------------------//
//----------------------------------------------------------------------------//

void renderUniformsTab(UPtr<Layer>& layer, AppData& appData)
{
    //--------------------------------------------------------------------------
    auto& su = *(appData.sharedUniforms);
    float fontSize = ImGui::GetFontSize();
    bool atLeastOneUniformMarkedForDeletion = false;
    bool atLeastOneUniformTypeChanged = false;
    bool atLeastOneSharedUniformStateChanged = false;
    static bool showSharedAndDefaultUniforms = true;
    if 
    (
        ImGui::Button
        (
            showSharedAndDefaultUniforms ? 
            "Hide shared and default uniforms" : 
            "Show shared and uniforms",
            {-1,0}
        )
    )
        showSharedAndDefaultUniforms = !showSharedAndDefaultUniforms;

    std::string uniformFrameName = "##uniformsFrame"+std::to_string(layer->id);
    ImGui::BeginChild(uniformFrameName.c_str(), ImVec2(-1, -1), false);

    int nColumns = 5;
    if 
    (
        ImGui::BeginTable
        (
            "##uniformTable", 
            nColumns, 
            ImGuiTableFlags_BordersV | 
            ImGuiTableFlags_BordersOuterH |
            ImGuiTableFlags_SizingFixedFit
        )
    )
    {
        ImGui::TableSetupColumn("##actions", 0, 4*fontSize);
        ImGui::TableSetupColumn("Name", 0, 10*fontSize);
        ImGui::TableSetupColumn("Type", 0, 7*fontSize);
        ImGui::TableSetupColumn("Bounds", 0, 3.5*fontSize);
        ImGui::TableSetupColumn
        (
            "Value", 
            0, 
            ImGui::GetContentRegionAvail().x
        );
        ImGui::TableHeadersRow();
        
        int row = 0;
        int column = 0;

        // First, render the built-in shared uniforms
        if (showSharedAndDefaultUniforms)
            row = renderBuiltInSharedUniforms(appData);

        // Then, render the user-created shared uniforms
        int nSharedUniforms = 
            su.fragment.uniforms.size()-
            su.userUniformsStartIndex;
        for (int i=0; i < nSharedUniforms; i++)
        {
            auto& uniform = su.fragment.uniforms
            [
                i + su.userUniformsStartIndex
            ];
            atLeastOneSharedUniformStateChanged = 
                atLeastOneSharedUniformStateChanged ||
                renderUniformTableRow
                (
                    uniform,
                    layer,
                    appData,
                    row++,
                    i == nSharedUniforms-1
                );
            if (uniform->isMarkedForDeletion)
                atLeastOneUniformMarkedForDeletion = true;
            if (uniform->hasSharedByUserChanged)
                atLeastOneSharedUniformStateChanged = true;
        }

        // Finally, render the user-created layer-specific uniforms
        for(unsigned int i=0; i<layer->uniforms.size(); i++)
        {
            auto& uniform = layer->uniforms[i];
            atLeastOneUniformTypeChanged = 
                atLeastOneUniformTypeChanged ||
                renderUniformTableRow
                (
                    uniform,
                    layer,
                    appData,
                    row++,
                    false,
                    showSharedAndDefaultUniforms
                );
            if (uniform->isMarkedForDeletion)
                atLeastOneUniformMarkedForDeletion = true;
            if (uniform->hasSharedByUserChanged)
                atLeastOneSharedUniformStateChanged = true;
        }

        // Render the "Create new uniform" button
        START_ROW(row, column)
        START_COLUMN(column)
        if (ImGui::Button(ICON_FA_PLUS, ImVec2(-1, 0)))
        {
            auto& u = Uniform::create(layer);
            layer->cache.uncompiledUniforms.emplace_back(u.getWeak());
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text("Add new uniform");
            ImGui::EndTooltip();
        }
        END_COLUMN(column)
        END_ROW(row)

        ImGui::EndTable();
    }

    ImGui::EndChild();

    ImGui::SetCursorPosY
    (
        ImGui::GetCursorPosY()+
        ImGui::GetContentRegionAvail().y-
        ImGui::GetTextLineHeightWithSpacing()
    );
    StatusBar::renderGui();

    // Remove uniforms marked for deletion
    if (atLeastOneUniformMarkedForDeletion)
    {
        for (unsigned int i=0; i<layer->uniforms.size(); i++)
        {
            UPtr<Uniform>& u = layer->uniforms[i];
            if (!u->isMarkedForDeletion)
                continue;
            u->deleteSelf();
            i--;
        }
        for (unsigned int i=0; i<su.fragment.uniforms.size(); i++)
        {
            UPtr<Uniform>& u = su.fragment.uniforms[i];
            if (!u->isMarkedForDeletion)
                continue;
            u->deleteSelf();
            i--;
            atLeastOneSharedUniformStateChanged = true;
        }
    }

    // Alternative strategy to cope with uniform block alignment changes after
    // uniform type changes or deletions (both of which can alter block layout:
    // compile right away automatically without asking the user)
    if 
    (
        atLeastOneUniformTypeChanged ||
        atLeastOneUniformMarkedForDeletion
    )
        compileShader(layer, appData);

    if (!atLeastOneSharedUniformStateChanged)
        return;

    // Check if the uniform state was changed from non-shared to shared
    for (unsigned int i=0; i<layer->uniforms.size(); i++)
    {
        UPtr<Uniform>& uniform = layer->uniforms[i];
        if (!(uniform->hasSharedByUserChanged && uniform->isSharedByUser))
            continue;
        uniform->hasSharedByUserChanged = false;
        uniform->setOwner(su.fragment);
        i--;
    }

    // Check if the uniform state was changed from shared to non-shared
    for (unsigned int i=0; i<su.fragment.uniforms.size(); i++)
    {
        auto& uniform = su.fragment.uniforms[i];
        if (!(uniform->hasSharedByUserChanged && !uniform->isSharedByUser))
            continue;
        uniform->hasSharedByUserChanged = false;
        uniform->setOwner(layer);
        i--;
    }

    // Alternative strategy to cope with uniform block alignment changes after
    // uniform type changes or deletions (both of which can alter block layout:
    // compile right away automatically without asking the user
    for (auto& l : appData.layers)
        compileShader(l, appData);
}

//----------------------------------------------------------------------------//

// Render the default/built-in shared uniforms as a table and return the row
// count
int renderBuiltInSharedUniforms(AppData& appData)
{
    auto& su = *(appData.sharedUniforms);
    int row = 0;
    int column;
    float fontSize = ImGui::GetFontSize();
    float halfButtonSize(1.7*fontSize);

    // iFrame --------------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if (ImGui::Button(ICON_FA_UNDO, ImVec2(halfButtonSize, 0)))
    {
        appData.rendering.toggles.resetFrameCounter = true;
        appData.rendering.toggles.restartRendering = true;
    }
    if 
    (
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) &&
        ImGui::BeginTooltip()
    )
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
    // Pause/resume rendering, which also affects iTime (but the
    // opposite is not true)
    if 
    (
        ImGui::Button
        (
            appData.rendering.isPaused ? 
            ICON_FA_PLAY : 
            ICON_FA_PAUSE, 
            ImVec2(-1, 0)
        )
    )
    {
        toggleRenderingPaused(appData, false);
        // When stopping rendering while tile rendering is enabled,
        // make sure to render all the tiles to reach the end of the
        // shader frame
        if
        (
            appData.rendering.isPaused && 
            appData.rendering.isTiledRenderingEnabled
        )
            appData.rendering.toggles.stepToNextFrame = true;
    }
    if (appData.rendering.isPaused)
    {
        if (ImGui::Button(ICON_FA_STEP_FORWARD, {-1,0}))
            appData.rendering.toggles.stepToNextFrame = true;
        else
        {
            // If tiled rendering is enabled, stepping by one shader frame
            // means stepping by nTiles app frames (since each app frame 
            // only renders a single shader tile), so the frame step is over
            // only once all tiles have been rendered (i.e., when tileIndex
            // is reset to 0)
            if (appData.rendering.isTiledRenderingEnabled)
            {
                if (appData.rendering.tileIndex == 0)
                    appData.rendering.toggles.stepToNextFrame = false;
            }
            else
                appData.rendering.toggles.stepToNextFrame = false;
        }
    }
    if 
    (
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
        ImGui::BeginTooltip()
    )
    {
        ImGui::Text("Render next frame and increment\niTime by iTimeDelta");
        ImGui::EndTooltip();
    }
    NEXT_COLUMN(column)
    ImGui::Text("iFrame");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::UInt].c_str());
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    ImGui::Text("%d", appData.rendering.frameIndex);
    END_ROW(row)

    // iTime --------------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if 
    (
        ImGui::Button
        (
            su.isTimeLooped ?
            ICON_FA_INFINITY : 
            ICON_FA_CIRCLE_NOTCH,
            ImVec2(halfButtonSize, 0)
        )
    )
        su.isTimeLooped = 
            !su.isTimeLooped;
    if 
    (
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
        ImGui::BeginTooltip()
    )
    {
        ImGui::Text
        (
            su.isTimeLooped ? 
            "Disable loop" : 
            "Enable loop"
        );
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    if 
    (
        ImGui::Button
        (
            su.isTimePaused ? 
            ICON_FA_PLAY : 
            ICON_FA_PAUSE, 
            ImVec2(-1, 0)
        ) && !appData.rendering.isPaused
    )
        su.isTimePaused = 
            !su.isTimePaused;
    if (su.isTimePaused)
    {
        if (ImGui::Button(ICON_FA_STEP_FORWARD, {-1,0}))
            appData.rendering.toggles.stepToNextFrame = true;
        else 
            appData.rendering.toggles.stepToNextFrame = false;
    }
    if 
    (
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
        ImGui::BeginTooltip()
    )
    {
        ImGui::Text("Increment iTime by iTimeDelta");
        ImGui::EndTooltip();
    }
    NEXT_COLUMN(column)
    ImGui::Text("iTime");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
    NEXT_COLUMN(column)
    glm::vec2* bounds = &su.iTimeUniform->gui.bounds;
    bool boundsChanged = renderEditUniformBoundsButton
    (
        su.iTimeUniform
    );
    NEXT_COLUMN(column)
    auto iTimePtr = &su.iTime;
    if (!boundsChanged)
    {
        bounds->x = std::min(*iTimePtr, bounds->x);
        bounds->y = std::max(*iTimePtr, bounds->y);
    }
    ImGui::PushItemWidth(-1);
    if 
    (
        ImGui::SliderFloat
        (
            "##iTimeSlider", 
            iTimePtr, 
            bounds->x,
            bounds->y,
            "%.3f"
        ) || boundsChanged
    )
    {
        if (boundsChanged)
        {
            *iTimePtr = std::max(*iTimePtr, bounds->x);
            *iTimePtr = std::min(*iTimePtr, bounds->y);
        }
        su.iUserAction = true;
        su.toggles.updateDataRangeII = true;
    }
    ImGui::PopItemWidth();
    END_ROW(row)

    START_ROW(row, column)
    NEXT_COLUMN(column)
    if 
    (
        ImGui::Button
        (
            su.isTimeResetOnFrameCounterReset ? 
            ICON_FA_BAN " " ICON_FA_UNDO: 
            ICON_FA_CHECK " " ICON_FA_UNDO, 
            ImVec2(-1, 0)
        )
    )
        su.isTimeResetOnFrameCounterReset =
            !su.isTimeResetOnFrameCounterReset;
    if 
    (
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
        ImGui::BeginTooltip()
    )
    {
        if (su.isTimeResetOnFrameCounterReset)
            ImGui::Text("Disable time reset on rendering restart");
        else
            ImGui::Text("Enable time reset on rendering restart");
        ImGui::EndTooltip();
    }
    END_ROW(row)

    // iTimeDelta --------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if 
    (
        ImGui::Button
        (
            su.isTimeDeltaSmooth ?
            ICON_FA_WAVE_SQUARE : 
            ICON_FA_SIGNATURE, 
            ImVec2(-1, 0)
        )
    )
        su.isTimeDeltaSmooth =
            !su.isTimeDeltaSmooth;
    if 
    (
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
        ImGui::BeginTooltip()
    )
    {
        if (su.isTimeDeltaSmooth)
            ImGui::Text("Disable time step smoothing");
        else
            ImGui::Text("Enable time step smoothing");
        ImGui::EndTooltip();
    }
    // No actions
    NEXT_COLUMN(column)
    ImGui::Text("iTimeDelta");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    if 
    (
        appData.rendering.isPaused || 
        su.isTimePaused
    )
    {
        ImGui::InputFloat
        (
            "##iTimeDeltaSliderFloat", 
            &su.iTimeDelta,
            0,
            0,
            "%.6f"
        );
        su.iTimeDelta = 
            std::max(su.iTimeDelta, 0.f);
        ImGui::SameLine();
        ImGui::Text("s");
    }
    else
        ImGui::Text("%.6f s", su.iTimeDelta);
    END_ROW(row)
    ImGui::Dummy({0, 0.1f*fontSize});

    // iRandom -------------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if 
    (
        ImGui::Button
        (
            su.isRandomNumberGeneratorPaused ? 
            ICON_FA_PLAY : 
            ICON_FA_PAUSE, 
            ImVec2(-1, 0)
        )
    )
        su.isRandomNumberGeneratorPaused = 
            !su.isRandomNumberGeneratorPaused;
    NEXT_COLUMN(column)
    ImGui::Text("iRandom");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    ImGui::Text("%.6f", su.iRandom);
    END_ROW(row)

    // iWindowAspectRatio --------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    // No actions
    NEXT_COLUMN(column)
    ImGui::Text("iWindowAspectRatio");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float].c_str());
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    ImGui::Text
    (
        "%.3f", su.iAspectRatio
    );
    END_ROW(row)
    ImGui::Dummy({0, 0.1f*fontSize});

    // iWindowResolution ---------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    // No actions
    NEXT_COLUMN(column)
    ImGui::Text("iWindowResolution");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Int2].c_str());
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    ImGui::Text
    (
        "%d x %d", 
        (int)su.iResolution.x, 
        (int)su.iResolution.y
    );
    END_ROW(row)
    
    // iKeyboard -----------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if 
    (
        ImGui::Button
        (
            su.isKeyboardInputEnabled ? 
            ICON_FA_PAUSE : 
            ICON_FA_PLAY, 
            ImVec2(-1, 0)
        )
    )
        toggleKeyboardInputs(appData);
    NEXT_COLUMN(column)
    ImGui::Text("iKeyboard");
    NEXT_COLUMN(column)
    ImGui::Text("vec3[]");
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    std::string pressed = "Pressed:";
    std::string held    = "Held:   ";
    std::string toggled = "Toggled:";
    for (int key=0; key<255; key++)
    {
        auto& keyData(su.iKeyboard[key]);
        if (keyData.x > 0)
            pressed += " "+vir::keyCodeToName[key];
        else if (keyData.y > 0)
            held += " "+vir::keyCodeToName[key];
        if (keyData.z > 0)
            toggled += " "+vir::keyCodeToName[key];
    }
    ImGui::Text(pressed.c_str());
    ImGui::Dummy({0, 0.1f*fontSize});
    ImGui::Text(held.c_str());
    ImGui::Dummy({0, 0.1f*fontSize});
    ImGui::Text(toggled.c_str());
    ImGui::Dummy({0, 0.1f*fontSize});
    END_ROW(row)
    
    // iMouse --------------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if 
    (
        ImGui::Button
        (
            su.isMouseInputEnabled ? 
            ICON_FA_PAUSE : 
            ICON_FA_PLAY, 
            ImVec2(-1, 0)
        )
    )
        toggleMouseInputs(appData);
    if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1, 0)))
        ImGui::OpenPopup("##iMouseSettings");
    if (ImGui::BeginPopup("##iMouseSettings"))
    {
        bool enabled = su.isMouseInputEnabled;
        std::string text = enabled ? "Disable inputs" : "Enable inputs";
        if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
            toggleMouseInputs(appData);
        ImGui::Text("Clamp value to window resolution ");
        ImGui::SameLine();
        bool status = su.isMouseInputClampedToWindow;
        ImGui::Checkbox("##iMouseSettings_ClampValue", &status);
        if (status != su.isMouseInputClampedToWindow)
            setMouseInputsClamped(appData, status);
        ImGui::Text("Input requires holding LMB       ");
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            ImGui::Text(
R"(If true, the iMouse uniform value will change on mouse 
motion only if the left mouse button (LMB) is held)");
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        ImGui::Checkbox
        (
            "##iMouseSettings_LMBHold", 
            &(su.mouseInputRequiresLMBHold)
        );
        ImGui::EndPopup();
    }
    NEXT_COLUMN(column)
    ImGui::Text("iMouse");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float4].c_str());
    NEXT_COLUMN(column)
    // No bounds
    NEXT_COLUMN(column)
    ImGui::Text
    (
        "%d, %d, %d, %d", 
        (int)su.iMouse.x, 
        (int)su.iMouse.y, 
        (int)su.iMouse.z, 
        (int)su.iMouse.w
    );
    if 
    (
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
        ImGui::BeginTooltip()
    )
    {
        ImGui::Text(
R"(The first two components (x, y) are the current x, y coordinates (with respect
to the lower-left corner of the ShaderThingOld window) of the mouse cursor if the
left mouse button is currently being held down. The last two components (z, w)
represent the x, y coordinates of the last left mouse button click, with their 
sign reversed. If the sign of the z component is positive, then the left mouse
is currently being held down)");
        ImGui::EndTooltip();
    }
    END_ROW(row)

    // iLook ---------------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1, 0)))
        ImGui::OpenPopup("##iLookSettings");
    if (ImGui::BeginPopup("##iLookSettings"))
    {
        bool enabled = su.isCameraMouseInputEnabled;
        std::string text = enabled ? "Disable inputs" : "Enable inputs";
        if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
            toggleCameraMouseInputs(appData);
        ImGui::Text("Input requires holding LMB       ");
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            ImGui::Text(
R"(If true, the iLook uniform value will change on mouse 
motion only if the left mouse button (LMB) is held)");
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        ImGui::Checkbox
        (
            "##iLookSettings_LMBHold", 
            &(su.cameraMouseInputRequiresLMBHold)
        );
        ImGui::Text("Mouse sensitivity ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::SliderFloat
        (
            "##iLookSensitivity", 
            &su.shaderCamera->mouseSensitivityRef(),
            1e-3,
            1
        );
        ImGui::PopItemWidth();
        ImGui::EndPopup();
    }
    NEXT_COLUMN(column)
    ImGui::Text("iLook");
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float3].c_str());
    NEXT_COLUMN(column)
    // All cmpts always bounds in [-1, 1]
    NEXT_COLUMN(column)
    {
        glm::vec3 value = su.iLook;
        std::string format = Helpers::getFormat(value);
        ImGui::PushItemWidth(-1);
        if 
        (
            ImGui::SliderFloat3
            (
                "##iLookSlider", 
                glm::value_ptr(value), 
                -1,
                1,
                format.c_str()
            )
        )
        {
            value = glm::normalize(value);
            su.iLook = value;
            su.shaderCamera->setDirection(value);
            su.iUserAction = true;
            su.toggles.updateDataRangeII = true;
        }
        ImGui::PopItemWidth();
    }
    END_ROW(row)

    // iWASD ---------------------------------------------------------------
    float posY = 0;
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1, 0)))
        ImGui::OpenPopup("##iWASDSettings");
    if (ImGui::BeginPopup("##iWASDSettings"))
    {
        bool enabled = su.isCameraKeyboardInputEnabled;
        std::string text = enabled ? "Disable inputs" : "Enable inputs";
        if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
            toggleCameraKeyboardInputs(appData);
        ImGui::Text("Keyboard sensitivity ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::SliderFloat
        (
            "##iWASDSensitivity", 
            &su.shaderCamera->keySensitivityRef(),
            1e-1,
            50
        );
        ImGui::PopItemWidth();
        ImGui::EndPopup();
    }

    bool showSeparator
    (
        su.fragment.uniforms.size() == 
        su.userUniformsStartIndex
    );
    if (showSeparator)
    {
        posY = ImGui::GetCursorPosY();
        ImGui::Separator();
    }
    NEXT_COLUMN(column)
    ImGui::Text("iWASD");
    if (showSeparator)
    {
        ImGui::SetCursorPosY(posY);
        ImGui::Separator();
    }
    NEXT_COLUMN(column)
    ImGui::Text(vir::Shader::uniformTypeToName[Type::Float3].c_str());
    if (showSeparator)
    {
        ImGui::SetCursorPosY(posY);
        ImGui::Separator();
    }
    NEXT_COLUMN(column)
    bounds = &su.iWASDUniform->gui.bounds;
    boundsChanged = renderEditUniformBoundsButton
    (
        su.iWASDUniform
    );
    if (showSeparator)
        ImGui::Separator();
    NEXT_COLUMN(column)
    {
        glm::vec3 value = su.iWASD;
        std::string format = Helpers::getFormat(value);
        if (!boundsChanged)
        {
            bounds->x = std::min(value.x, bounds->x);
            bounds->x = std::min(value.y, bounds->x);
            bounds->x = std::min(value.z, bounds->x);
            bounds->y = std::max(value.x, bounds->y);
            bounds->y = std::max(value.y, bounds->y);
            bounds->y = std::max(value.z, bounds->y);
        }
        ImGui::PushItemWidth(-1);
        if 
        (
            ImGui::SliderFloat3
            (
                "##iWASDSlider", 
                glm::value_ptr(value), 
                bounds->x,
                bounds->y,
                format.c_str()
            ) || boundsChanged
        )
        {
            if (boundsChanged)
            {
                value.x = std::max(value.x, bounds->x);
                value.x = std::min(value.x, bounds->y);
                value.y = std::max(value.y, bounds->x);
                value.y = std::min(value.y, bounds->y);
                value.z = std::max(value.z, bounds->x);
                value.z = std::min(value.z, bounds->y);
            }
            su.iWASD = value;
            su.shaderCamera->setPosition(value);
            su.iUserAction = true;
            su.toggles.updateDataRangeII = true;
        }
        ImGui::PopItemWidth();
    }
    if (showSeparator)
        ImGui::Separator();
    END_ROW(row)
    return row;
}

//----------------------------------------------------------------------------//

bool renderEditUniformBoundsButton
(
    const vir::Ptr<Uniform>& uniform,
    bool renderDragStepSlider
)
{
    glm::vec2& bounds = uniform->gui.bounds;
    float* dragStep = &uniform->gui.dragStep;
    float* logarithmicZero = &uniform->gui.logarithmicZero;
    auto type = uniform->type();
    if (ImGui::Button(ICON_FA_RULER_COMBINED, ImVec2(-1, 0)))
        ImGui::OpenPopup("##uniformBounds");
    if (ImGui::BeginPopup("##uniformBounds"))
    {
        
        glm::vec2 bounds0(bounds);
        if (type == vir::Uniform::Type::UInt)
            bounds.x = std::max(bounds.x, 0.0f);
        ImGui::Text("Minimum value    ");
        ImGui::SameLine();
        float inputWidth = 6*ImGui::GetFontSize();
        auto minf = Helpers::getFormat(bounds0.x);
        auto maxf = Helpers::getFormat(bounds0.y);
        ImGui::PushItemWidth(inputWidth);
        ImGui::InputFloat
        (
            "##minValueInput", 
            &(bounds.x), 0.f, 0.f,
            minf.c_str()
        );
        ImGui::PopItemWidth();
        ImGui::Text("Maximum value    ");
        ImGui::SameLine();
        ImGui::PushItemWidth(inputWidth);
        ImGui::InputFloat
        (
            "##maxValueInput", 
            &(bounds.y), 0.f, 0.f,
            maxf.c_str()
        );
        ImGui::PopItemWidth();
        if 
        (
            (type == Type::Int2 || type == Type::Float2) && 
            renderDragStepSlider
        )
        {
            auto format = Helpers::getFormat(*dragStep);
            ImGui::Text("Mouse drag step  ");
            ImGui::SameLine();
            ImGui::PushItemWidth(inputWidth);
            if (type == Type::Int2)
            {
                int iDragStep = (int)(*dragStep);
                ImGui::InputInt
                (
                    "##dragStepSize", 
                    &iDragStep, 0.f, 0.f
                );
                *dragStep = (float)iDragStep;
            }
            else
                ImGui::InputFloat
                (
                    "##dragStepSize", 
                    dragStep, 0.f, 0.f,
                    format.c_str()
                );
            ImGui::PopItemWidth();
        }
        else if 
        (
            uniform->isLogarithmic &&
            bounds.x * bounds.y <= 0
        )
        {
            auto format = Helpers::getFormat(*logarithmicZero);
            ImGui::Text("Logarithmic zero ");
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                ImGui::Text(
R"(When a log-scale slider is used and the uniform bounds contain or cross 0, 
this value determines the closest value to 0 (that differs from 0) that can be
set by adjusting the slider)");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            ImGui::PushItemWidth(inputWidth);
            float logarithmicZero0(*logarithmicZero);
            if 
            (
                ImGui::InputFloat
                (
                    "##logarithmicZero", 
                    logarithmicZero, 0.f, 0.f,
                    format.c_str()
                )
            )
            {
                if (*logarithmicZero <= 0)
                    *logarithmicZero = logarithmicZero0;
            }
            ImGui::PopItemWidth();
        }
        ImGui::EndPopup();
        return (bounds != bounds0); 
    }
    return false;
}

//----------------------------------------------------------------------------//

bool renderUniformTableRow
(
    const UPtr<Uniform>& uniform,
    const UPtr<Layer>& layer,
    AppData& appData,
    int row,
    const bool showSeparator,
    const bool showSharedAndDefaultUniforms
)
{
    float fontSize = ImGui::GetFontSize();
    int column;
    bool managed
    (
        uniform->managedType != ManagedType::None
    );
    if 
    (
        (managed && !showSharedAndDefaultUniforms) ||
        uniform->managedType == ManagedType::ResourceResolution
    )
        return false;
    bool isSharedByUser0 = uniform->isSharedByUser;
    bool nameChanged = false;
    bool typeChanged = false;
    auto& su = *(appData.sharedUniforms);
    auto& resources = appData.resources;
    
    START_ROW(row, column)

    START_COLUMN(column) // Action column --------------------------------------
    float y0 = 0;
    if (!managed)
    {
        float halfButtonSize(1.7*fontSize);
        if (ImGui::Button(ICON_FA_TRASH, ImVec2(halfButtonSize, 0)))
        {
            uniform->isMarkedForDeletion = true;
            // layer->uniformBuffer_->removeUniform(uniform.get()); // TODO - CHECK
            // The uniform is gonna get deleted, so the layer(s) using it
            // will have to be recompiled
            if (uniform->isSharedByUser)
            {
                for (auto& l : appData.layers)
                    l->hasUncompiledEdits = true;
            }
            else
                layer->hasUncompiledEdits = true;
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text("Delete this uniform");
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        if 
        (
            ImGui::Button
            (
                !uniform->isSharedByUser ?
                ICON_FA_ARROW_UP :
                ICON_FA_ARROW_DOWN,
                {-1, 0}
            )
        )
        {
            uniform->hasSharedByUserChanged = true;
            uniform->isSharedByUser = 
                !uniform->isSharedByUser;
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            if (!uniform->isSharedByUser)
                ImGui::Text("Share this uniform across all layers");
            else
                ImGui::Text(
"Remove this uniform from shared\nuniforms across all layers"\
                );
            ImGui::EndTooltip();
        }
        y0 = ImGui::GetCursorPosY();
    }
    if (showSeparator)
        ImGui::Separator();
    END_COLUMN(column)
    
    START_COLUMN(column) // Name column ----------------------------------------
    if (managed)
        ImGui::Text(uniform->name.c_str());
    else
    {
        if (ImGui::InputText("##uniformName", &uniform->name))
        {
            uniform->markForSubmissionToAllClientBuffers();
            nameChanged = true;
            Helpers::enforceUniqueName
            (
                uniform->name,
                layer->uniforms,
                uniform.get(),
                true
            );
        }
    }
    bool named(uniform->name.size() > 0);
    if (showSeparator)
        ImGui::Separator();
    END_COLUMN(column)

    START_COLUMN(column) // Type column ----------------------------------------
    if (managed)
        ImGui::Text(vir::Shader::uniformTypeToName[uniform->type()].c_str());
    else if 
    (
        ImGui::BeginCombo
        (
            "##uniformTypeSelector", 
            vir::Shader::uniformTypeToName[uniform->type()].c_str()
        )
    )
    {
        for(auto uniformTypeName : Uniform::supportedTypeNames)
        {
            if (!ImGui::Selectable(uniformTypeName.c_str()))
                continue;
            auto selectedType = 
                vir::Shader::uniformNameToType[uniformTypeName];
            if (selectedType == uniform->type())
                continue;
            typeChanged = true;
            uniform->setType(selectedType);
        }
        ImGui::EndCombo();
    }
    if (showSeparator)
        ImGui::Separator();
    END_COLUMN(column)

    START_COLUMN(column) // Bounds column ------------------------------------------
    bool boundsChanged(false);
    glm::vec2& bounds = uniform->gui.bounds;
    if (uniform->gui.showBounds)
        boundsChanged = renderEditUniformBoundsButton(uniform, true);
    if (showSeparator)
    {
        if (y0 > 0)
            ImGui::SetCursorPosY(y0);
        ImGui::Separator();
    }
    END_COLUMN(column)

    START_COLUMN(column) // Value column ---------------------------------------
    switch(uniform->type())
    {
        case vir::Uniform::Type::Bool :
        {
            auto value = uniform->getValue<bool>();
            if (ImGui::Checkbox((value) ? "true" : "false", &value))
            {
                uniform->setValue(value, Type::Bool);
                if (named)
                {
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::UInt : //-------------------------
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
                uniform->setValue(value, Type::UInt);
                if (named)
                {
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Int : //--------------------------
        {
            auto value = uniform->getValue<int>();
            if (!boundsChanged)
            {
                bounds.x = std::min((float)value, bounds.x);
                bounds.y = std::max((float)value, bounds.y);
            }
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
                uniform->setValue(value, Type::Int);
                if (named)
                {
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Int2 : //-------------------------
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
                        vir::Window::instance()->
                        primaryMonitorResolution();
                    int maxRes = std::max(monitor.x, monitor.y);
                    bounds.x = std::min(bounds.x, -bounds.y);
                    bounds.y = std::max(-bounds.x, bounds.y);
                    auto valuei0 = uniform->getCache<glm::ivec2>();
                    value.x = valuei0.x + 
                        (float)(10.f*delta.x*uniform->gui.dragStep)/maxRes;
                    value.y = valuei0.y + 
                        (float)(10.f*delta.y*uniform->gui.dragStep)/maxRes;
                    input = true;
                }
                else
                    uniform->setCache<glm::ivec2>(value);
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
                uniform->setValue(value, Type::Int2);
                if (named)
                {
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Int3 : //-------------------------
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
                uniform->setValue(value, Type::Int3);
                if (named)
                {
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Int4 : //-------------------------
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
                uniform->setValue(value, Type::Int4);
                if (named)
                {
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Float : //------------------------
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
                uniform->managedType == 
                    Uniform::ManagedType::LayerAspectRatio
            )
                ImGui::Text("%.3f", value);
            else
            {
                if (ImGui::SmallButton(uniform->isLogarithmic?"log":"lin"))
                    uniform->isLogarithmic = !uniform->isLogarithmic;
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                {
                    ImGui::Text
                    ( 
                        uniform->isLogarithmic ?
                        "Switch slider to linear scale" : 
                        "Switch slider to logarithmic scale"
                    );
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                ImGuiSliderFlags flags = 0;
                std::string format;
                if (uniform->isLogarithmic)
                {
                    ImGui::PushDragSliderLogZeroForScientificNotation
                    (
                        uniform->gui.logarithmicZero
                    );
                    format = "%.3e";
                    flags = ImGuiSliderFlags_Logarithmic;
                } 
                else
                    format = Helpers::getFormat(value);
                input = ImGui::SliderFloat
                (
                    "##fSlider", 
                    &value, 
                    bounds.x,
                    bounds.y,
                    format.c_str(),
                    flags
                );
                if (uniform->isLogarithmic)
                    ImGui::PopDragSliderLogZeroForScientificNotation();
            }
            if (input || boundsChanged)
            {
                if (boundsChanged)
                {
                    value = std::max(value, bounds.x);
                    value = std::min(value, bounds.y);
                }
                uniform->setValue(value, Type::Float);
                if (named)
                {                                                              
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Float2 : //-----------------------
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
                uniform->managedType == 
                    Uniform::ManagedType::LayerResolution || 
                uniform->managedType == 
                    Uniform::ManagedType::ResourceResolution 
            )
            {
                auto value = uniform->getValue<glm::vec2>();
                ImGui::Text
                (
                    "%d x %d",
                    (int)value.x, (int)value.y
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
                        vir::Window::instance()->
                        primaryMonitorResolution();
                    int maxRes = std::max(monitor.x, monitor.y);
                    bounds.x = std::min(bounds.x, -bounds.y);
                    bounds.y = std::max(-bounds.x, bounds.y);
                    auto valuef0 = uniform->getCache<glm::vec2>();
                    value.x = valuef0.x + 
                        (float)(10.f*delta.x*uniform->gui.dragStep)/maxRes;
                    value.y = valuef0.y + 
                        (float)(10.f*delta.y*uniform->gui.dragStep)/maxRes;
                    input = true;
                }
                else
                    uniform->setCache<glm::vec2>(value);
                std::string format = Helpers::getFormat(value);
                bool input2 = ImGui::SliderFloat2
                (
                    "##f2Slider", 
                    glm::value_ptr(value), 
                    bounds.x,
                    bounds.y,
                    format.c_str()
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
                uniform->setValue(value, Type::Float2);
                if (named)
                {
                    uniform->markForSubmissionToAllClientBuffers();
                    su.iUserAction = true;
                    su.toggles.updateDataRangeII = true;
                }
            }
            break;
        }
        case vir::Uniform::Type::Float3 : //-----------------------
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
                std::string format = Helpers::getFormat(value);
                if 
                (
                    ImGui::SliderFloat3
                    (
                        "##f3Slider", 
                        glm::value_ptr(value), 
                        bounds.x,
                        bounds.y,
                        format.c_str()
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
                    uniform->setValue(value, Type::Float3);
                    if (named)
                    {
                        uniform->markForSubmissionToAllClientBuffers();
                        su.iUserAction = true;
                        su.toggles.updateDataRangeII = true;
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
                    uniform->setValue(value, Type::Float3);
                    if (named)
                    {
                        uniform->markForSubmissionToAllClientBuffers();
                        su.iUserAction = true;
                        su.toggles.updateDataRangeII = true;
                    }
                }
            }
            break;
        }
        case vir::Uniform::Type::Float4 : //-----------------------
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
                std::string format = Helpers::getFormat(value);
                if 
                (
                    ImGui::SliderFloat4
                    (
                        "##f4Slider", 
                        glm::value_ptr(value), 
                        bounds.x,
                        bounds.y,
                        format.c_str()
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
                    uniform->setValue(value, Type::Float4);
                    if (named)
                    {
                        uniform->markForSubmissionToAllClientBuffers();
                        su.iUserAction = true;
                        su.toggles.updateDataRangeII = true;
                    }
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
                    uniform->setValue(value, Type::Float4);
                    if (named)
                    {
                        uniform->markForSubmissionToAllClientBuffers();
                        su.iUserAction = true;
                        su.toggles.updateDataRangeII = true;
                    }
                }
            }
            break;
        }

        #define CHECK_RESOURCE_SELECTED                                        \
        if (ImGui::Selectable(r->name().c_str()))                              \
        {                                                                      \
            if (resource != nullptr)                                           \
            {                                                                  \
                appData.rendering.toggles.requestFullRecompilation =           \
                    appData.rendering.toggles.requestFullRecompilation ||      \
                    (resource->isInternalFormatUnsigned() !=                   \
                    r->isInternalFormatUnsigned() && named);                   \
                if (resource->isUsedByUniform(uniform.get()))                  \
                    resource->removeClientUniform(uniform.get());              \
            }                                                                  \
            else if (named)                                                    \
                appData.rendering.toggles.requestFullRecompilation = true;     \
            if (!r->isUsedByUniform(uniform.get()))                            \
                r->addClientUniform(uniform.get());                            \
            appData.deferredActionBuffer.add                                   \
            (                                                                  \
                [&uniform, &r]()                                               \
                {uniform->setResourcePtr(r);}                                  \
            );                                                                 \
            su.iUserAction = true;                                 \
            su.toggles.updateDataRangeII = true;                   \
        }

        case vir::Uniform::Type::Sampler2D :
        case vir::Uniform::Type::Image2D :
        {
            auto resource = 
                uniform->getValuePtr<Resource>();
            std::string name
            (
                (resource != nullptr) ? resource->name() : ""
            );
            if (ImGui::BeginCombo("##tx2DSelector", name.c_str()))
            {
                for (UPtr<Resource>& r : resources)
                {
                    if 
                    (
                        r->type() != Resource::Type::Texture2D &&
                        r->type() != Resource::Type::AnimatedTexture2D &&
                        r->type() != Resource::Type::Framebuffer
                    )
                        continue;
                    CHECK_RESOURCE_SELECTED
                }
                ImGui::EndCombo();
            }
            break;
        }
        case vir::Uniform::Type::Sampler3D :
        case vir::Uniform::Type::Image3D :
        {
            auto resource = 
                uniform->getValuePtr<Resource>();
            std::string name
            (
                (resource != nullptr) ? resource->name() : ""
            );
            if (ImGui::BeginCombo("##tx3DSelector", name.c_str()))
            {
                for (auto& r : resources)
                {
                    if (r->type() != Resource::Type::Texture3D)
                        continue;
                    CHECK_RESOURCE_SELECTED
                }
                ImGui::EndCombo();
            }
            break;
        }
        case vir::Uniform::Type::SamplerCube :
        case vir::Uniform::Type::ImageCube :
        {
            auto resource = 
                uniform->getValuePtr<Resource>();
            std::string name
            (
                (resource != nullptr) ? resource->name() : ""
            );
            if (ImGui::BeginCombo("##cmSelector", name.c_str()))
            {
                for (auto& r : resources)
                {
                    if (r->type() != Resource::Type::Cubemap)
                        continue;
                    CHECK_RESOURCE_SELECTED
                }
                ImGui::EndCombo();
            }
            break;
        }
        default:
            break;
    }
    if (showSeparator)
        ImGui::Separator();
    END_COLUMN(column)
    
    END_ROW(row)

    if (!nameChanged && !typeChanged)
        return false;

    // If the uniform name or type have changed, it should be added to the
    // list of uncompiled uniforms of the layer using this uniform (if this
    // uniform is not SharedByUser) or of all the layers (if this uniform
    // is SharedByUser). Also, if at least one uniform has been named,
    // said layer should be recompiled (all other layer uniforms which are
    // still unnamed are simply ignored)
    if (isSharedByUser0)
    {
        for (auto& l : appData.layers)
        {
            if 
            (
                std::find // I.e., if not already in uncompiledUniforms
                (
                    l->cache.uncompiledUniforms.begin(), 
                    l->cache.uncompiledUniforms.end(), 
                    uniform.get()
                ) == l->cache.uncompiledUniforms.end()
            )
                l->cache.uncompiledUniforms.emplace_back(uniform.getWeak());
            bool atLeastOneUniformNamed = false;
            for (auto& u : l->cache.uncompiledUniforms)
            {
                if (u->name.size() == 0)
                    continue;
                atLeastOneUniformNamed = true;
                break;
            }
            if (atLeastOneUniformNamed)
                l->hasUncompiledEdits = true;
        }
    }
    else
    {
        if 
        (
            std::find
            (
                layer->cache.uncompiledUniforms.begin(), 
                layer->cache.uncompiledUniforms.end(), 
                uniform.get()
            ) == layer->cache.uncompiledUniforms.end()
        )
            layer->cache.uncompiledUniforms.emplace_back(uniform.getWeak());
        bool atLeastOneUniformNamed = false;
        for (auto& u : layer->cache.uncompiledUniforms)
        {
            if (u->name.size() == 0)
                continue;
            atLeastOneUniformNamed = true;
            break;
        }
        if (atLeastOneUniformNamed)
            layer->hasUncompiledEdits = true;
    }

    return typeChanged;
    
}; // End of renderUniform

//----------------------------------------------------------------------------//
// Resources -----------------------------------------------------------------//
//----------------------------------------------------------------------------//

void renderResourcesMenuItem(AppData& appData)
{
    if 
    (
        ImGui::SmallButton
        (
            Resource::gui.isDetachedFromControlPanel ? 
            ICON_FA_WINDOW_MAXIMIZE : 
            ICON_FA_ARROW_RIGHT
        )
    )
        Resource::gui.isDetachedFromControlPanel = 
            !Resource::gui.isDetachedFromControlPanel;
    ImGui::SameLine();
    if (!Resource::gui.isDetachedFromControlPanel)
    {
        if (ImGui::BeginMenu("Resource manager"))
        {
            Resource::gui.isOpen = true;
            renderResourcesTable(appData);
            ImGui::EndMenu();
        }
        else
            Resource::gui.isOpen = false;
        return;
    }
    ImGui::MenuItem("Resource manager", NULL, &Resource::gui.isOpen);
}

//----------------------------------------------------------------------------//

void renderResourcesTable(AppData& appData)
{
    if (!Resource::gui.isOpen)
        return;
    if (Resource::gui.isDetachedFromControlPanel)
    {
        ImGui::SetNextWindowSize(ImVec2(900,350), ImGuiCond_FirstUseEver);
        static ImGuiWindowFlags windowFlags(ImGuiWindowFlags_NoCollapse);
        ImGui::Begin("Resource manager", &Resource::gui.isOpen, windowFlags);

        // Refresh icon if needed
        static bool isIconSet(false);
        static bool isWindowDocked(ImGui::IsWindowDocked());
        if (!isIconSet || isWindowDocked != ImGui::IsWindowDocked())
        {
            isIconSet = vir::ImGuiRenderer::setWindowIcon
            (
                "Resource manager", 
                ByteData::Icon::sTIconData, 
                ByteData::Icon::sTIconSize,
                false
            );
            isWindowDocked = ImGui::IsWindowDocked();
        }
    }

    float cursorPosY0 = ImGui::GetCursorPosY();
    float fontSize = ImGui::GetFontSize();
    static float tableHeight = 0;
    ImGuiTableFlags flags = 
        ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("##resourceTable", 6, flags, ImVec2(0., tableHeight)))
    {
        // Declare columns
        static ImGuiTableColumnFlags flags = 0;
        ImGui::TableSetupColumn("##controls", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Type", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Preview", flags, 4.0*fontSize);
        ImGui::TableSetupColumn("Name", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Resolution", flags, 10.0*fontSize);
        ImGui::TableSetupColumn
        (
            "Aspect ratio", 
            flags, 
            Resource::gui.isDetachedFromControlPanel ? 
            ImGui::GetContentRegionAvail().x : 8.0*fontSize
        );
        ImGui::TableHeadersRow();

        const int nRows = appData.resources.size();
        for (int row=0; row<nRows; row++)
        {
            renderResourcesTableRow(appData, row);
        }
        renderAddResourceButton(appData, nRows);
        tableHeight = (ImGui::GetCursorPosY()-cursorPosY0);
        ImGui::EndTable();
    }

    if (Resource::gui.isDetachedFromControlPanel)
        ImGui::End();
}

//----------------------------------------------------------------------------//

void renderResourcesTableRow(AppData& appData, int row)
{
    UPtr<Resource>& resource = appData.resources[row];
    float fontSize = ImGui::GetFontSize();
    int column = 0;
    START_ROW(row, column)
    START_COLUMN(column) // Actions column -------------------------------------
    renderResourceActionsButton(appData, row);
    END_COLUMN(column)
    START_COLUMN(column) // Type column ----------------------------------------
    std::string typeName = Resource::typeToName.at(resource->type());
    if (auto texture = dynamic_cast<Texture2DResource*>(resource.get()))
    {
        // Small fix to distinguish storage textures from "regular" image-
        // loaded textures. I will need to revist the whole storage-texture
        // thing in the future, or add the same functionalities (namely
        // resizing/reformatting) to "regular" image-loaded textures to 
        // remove such somewhat artificial distinctions altogether
        if (!texture->hasRawData())
            typeName += " (S)";
    }
    ImGui::Text(typeName.c_str());
    END_COLUMN(column)
    START_COLUMN(column) // Preview column -------------------------------------
    float x = resource->width();
    float y = resource->height();
    float aspectRatio = x/y;
    auto previewTexture2D = 
    [&aspectRatio]
    (
        const Resource* resource, 
        float sideSize, 
        float offset
    )->void
    {
        ImVec2 previewSize;
        ImVec2 hoverSize{256,256};
        if (aspectRatio > 1.0)
        {
            previewSize = ImVec2(sideSize, sideSize/aspectRatio);
            hoverSize.y /= aspectRatio;
        }
        else
        { 
            previewSize = ImVec2(sideSize*aspectRatio, sideSize);
            hoverSize.x *= aspectRatio;
        }
        float startx = ImGui::GetCursorPosX();
        ImGui::SetCursorPosX(startx + offset);
#define SHOW_IMAGE(size)                                                \
ImGui::Image                                                            \
(                                                                       \
    (ImTextureID)                                                       \
    (                                                                   \
        resource->type() != Resource::Type::AnimatedTexture2D ?         \
        resource->id() :                                                \
        ((AnimatedTexture2DResource*)(resource))->frameId()             \
    ),                                                                  \
    size,                                                               \
    {0,1},                                                              \
    {1,0}                                                               \
);
        SHOW_IMAGE(previewSize)
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            SHOW_IMAGE(hoverSize)
            ImGui::EndTooltip();
        }
    };
    if (resource->isInternalFormatUnsigned())
        ImGui::Text("N/A");
    else if 
    (
        resource->type() == Resource::Type::Texture2D ||
        resource->type() == Resource::Type::AnimatedTexture2D ||
        resource->type() == Resource::Type::Framebuffer
    )
        previewTexture2D(resource.get(), 1.4*fontSize, 1.3*fontSize);
    else if (resource->type() == Resource::Type::Cubemap)
    {
        auto cubemap = (CubemapResource*)resource.get();
        for (int i=0; i<6; i++)
        {
            float offset = (i == 0 || i == 3) ? 0.75*fontSize : 0.0;
            previewTexture2D
            (
                cubemap->faces()[i].get(), 
                0.5*fontSize, 
                offset
            );
            if ((i+1)%3 != 0)
                ImGui::SameLine();
        }
    }
    END_COLUMN(column)
    START_COLUMN(column) // Name column ----------------------------------------
    if (resource->type() != Resource::Type::Framebuffer)
    {
        if (ImGui::InputText("##resourceName", resource->namePtr()))
        {
            Helpers::enforceUniqueName
            (
                *(resource->namePtr()),
                appData.resources,
                resource.get()
            );
        }
    }
    else
        ImGui::Text(resource->name().c_str());
    END_COLUMN(column)
    START_COLUMN(column) // Resolution column ----------------------------------
    if (resource->type() == Resource::Type::Texture3D)
        ImGui::Text("%d x %d x %d", (int)x, (int)y, (int)resource->depth());
    else
        ImGui::Text("%d x %d", (int)x, (int)y);
    END_COLUMN(column)
    START_COLUMN(column) // Aspect ratio column --------------------------------
    ImGui::Text("%.3f", aspectRatio);
    END_COLUMN(column)
    END_ROW(row)
}

//----------------------------------------------------------------------------//

void renderAddResourceButton(AppData& appData, int row)
{
    int column = 0;
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if (ImGui::Button(ICON_FA_PLUS, ImVec2(-1,0)))
        ImGui::OpenPopup("##addResourcePopup");
    if (ImGui::BeginPopup("##addResourcePopup"))
    {
        float buttonWidth = 12*ImGui::GetFontSize();
        if 
        (
            ImGui::Button
            (
                "Load texture-2D",
                ImVec2(buttonWidth, 0)
            )
        )
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
            appData.deferredActionBuffer.add
            (
                [&appData]()
                {
                    auto filepath = Resource::fileDialog.selection().front();
                    auto& r = appData.resources.emplace_back
                    (
                        Texture2DResource::create(filepath)
                    );
                    std::string name = Helpers::filename(filepath);
                    Helpers::enforceUniqueName
                    (
                        name, 
                        appData.resources, 
                        r.get()
                    );
                    r->setName(name);
                },
                []() -> bool
                {
                    return Resource::fileDialog.validSelection();
                }
            );
        }
        ImGui::EndPopup();
    }
    END_ROW(row)
}

//----------------------------------------------------------------------------//

void renderResourceActionsButton(AppData& appData, int row)
{
    if (row >= appData.resources.size())
        return;
    UPtr<Resource>& resource = appData.resources[row];
    if (!resource.valid())
        return;
    if (resource->type() == Resource::Type::Framebuffer)
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
        auto& resource = 
            appData.resources[row].dynamicDowncastTo<LayerResource>();
        renderLayerFramebufferSettings(resource->layer(), appData);
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##resourceActions"))
    {
        auto size = ImVec2(12*ImGui::GetFontSize(), 0);
        if (ImGui::Button(ICON_FA_TRASH, size))
        {
            appData.deferredActionBuffer.add
            (
                [&appData, row]()
                {
                    // UPtr<Resource>& resource = appData.resources[row];
                    // Used to call legacy 'removeResourceFromUniforms' on all
                    // layers, tentatively removed
                    appData.resources.erase(appData.resources.begin()+row);
                }
            );
        }
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------//

/*

void renderResourceMemoryEstimateWarning
(
    uint64_t textureSize,
    InternalFormat internalFormat,
    bool is2D
)
{
    double requiredMemory = textureSize;
    double memoryPerPixel = 
        vir::TextureBuffer::internalFormatToBytes.at(internalFormat);
    requiredMemory *= memoryPerPixel;
    // Show a warning for good measure when creating beefier textures,
    // threshold arbitrarily set at 64 MiB of VRAM
    if (requiredMemory >= 67108864)
    {
        // Mip maps occupy a theoretical maximum of an additional 
        // 1/8 + 1/64 + 1/512 + 1/4096 + 1/... = 1/7 of the memory
        // occupied by the base level of a 3D texture, while for
        // 2D textures this is 1/4 + 1/16 + 1/32 + ... = 1/3 of the
        // memory occupied by the base level
        double mipmapsMemory = 
            std::floor(requiredMemory/memoryPerPixel/(is2D?3:7))*memoryPerPixel;
        auto uom1 = Helpers::autoRescaleMemoryValue(requiredMemory);
        auto uom2 = Helpers::autoRescaleMemoryValue(mipmapsMemory);
        ImGui::PushStyleColor(ImGuiCol_Text, {1.f,1.f,0.f,1.f});
        ImGui::Text(
R"(This texture will occupy at least 
%.1f %s of free VRAM, and up to 
an additional %.1f %s for mipmaps. 
Please, make sure your system has 
at least the reported amount of 
free VRAM to avoid program and/or 
system crashes)",
            requiredMemory, 
            uom1,
            mipmapsMemory, 
            uom2
        );
        ImGui::PopStyleColor();
    }
}

//----------------------------------------------------------------------------//

bool createOrResizeOrReformatTexture2DGui
(
    UPtr<Resource>& resource,
    const bool enablePopup,
    const bool resetValues,
    const ImVec2 buttonSize
)
{
    bool valid = false;
    static glm::ivec2 resolution(1, 1);
    static InternalFormat internalFormat = InternalFormat::RGBA_SF_32;
    if (enablePopup && ImGui::Button("Create texture-2D", buttonSize))
    {
        ImGui::OpenPopup("##createTexturePopup");
        resolution = {1,1};
        internalFormat = InternalFormat::RGBA_SF_32;
    }
    if (resetValues && resource != nullptr)
    {
        resolution.x = resource->width();
        resolution.y = resource->height();
        internalFormat = resource->internalFormat();
    }
    if (ImGui::BeginPopup("##createTexturePopup") || !enablePopup)
    {
        if (resource == nullptr)
        {
            ImGui::Text(
R"(Create a blank texture, useful for e.g., 
shader data storage via imageLoad and
imageStore operations. Data written to 
these textures will not be saved within 
the project)");
            ImGui::Separator();
        }
        if (enablePopup)
            ImGui::Text("Resolution ");
        else
            ImGui::Text("Resolution          ");
        ImGui::SameLine();
        float itemWidth = -1; //resource == nullptr ? -1 : 12*ImGui::GetFontSize();
        ImGui::PushItemWidth(itemWidth);
        if 
        (
            ImGui::InputInt2
            (
                "##windowResolution", 
                glm::value_ptr(resolution)
            )
        )
        {
            resolution.x = std::min(std::max(resolution.x, 1), 4096);
            resolution.y = std::min(std::max(resolution.y, 1), 4096);
        }
        ImGui::PopItemWidth();
        if (enablePopup)
            ImGui::Text("Format     ");
        else
            ImGui::Text("Format              ");
        ImGui::SameLine();
        ImGui::PushItemWidth(itemWidth);
        // RGB formats are not easy to work with due to memory-alignment 
        // limitations so they are omitted
        static std::vector<InternalFormat> supportedFormats = 
        {
            InternalFormat::R_UI_32,
            InternalFormat::R_SF_32,
            InternalFormat::RG_UI_32,
            InternalFormat::RG_SF_32,
            InternalFormat::RGBA_UI_32,
            InternalFormat::RGBA_SF_32
        };
        if
        (
            ImGui::BeginCombo
            (
                "##textureFormatCombo",
                vir::TextureBuffer2D::internalFormatToName.at
                (
                    internalFormat
                ).c_str()
            )
        )
        {
            for (auto format : supportedFormats)
            {
                if 
                (
                    ImGui::Selectable
                    (
                        vir::TextureBuffer2D::internalFormatToName.at
                        (
                            format
                        ).c_str()
                    )
                )
                {
                    internalFormat = format;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        bool disabled = false;
        if (resource != nullptr)
        {
            ImGui::Text("VRAM footprint      ");
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                ImGui::Text("VRAM memory occupied by this texture");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            double maxMemoryFootprint = resource->maxMemoryFootprint();
            auto uom = Helpers::autoRescaleMemoryValue(maxMemoryFootprint);
            ImGui::Text("%.1f %s", maxMemoryFootprint, uom);
            
            if 
            (
                (int)resource->width() == resolution.x && 
                (int)resource->height() == resolution.y && 
                resource->internalFormat() == internalFormat
            )
            {
                ImGui::BeginDisabled();
                disabled = true;
            }
        }
        if 
        (
            ImGui::Button
            (
                resource == nullptr ? "Create texture-2D" : "Resize or reformat", 
                ImVec2(-1, 0)
            )
        )
        {
            if (resource != nullptr)
            {
                auto wrapMode0 = resource->wrapMode(0);
                auto wrapMode1 = resource->wrapMode(1);
                auto minFilterMode = resource->minFilterMode();
                auto magFilterMode = resource->magFilterMode();
                auto format0 = resource->internalFormat();
                if 
                (
                    ((Texture2DResource*)resource.get())->set
                    (
                        resolution.x, 
                        resolution.y, 
                        internalFormat
                    )
                )
                {
                    valid = true;
                    auto format = resource->internalFormat();
                    resource->setWrapMode(0, wrapMode0);
                    resource->setWrapMode(1, wrapMode1);
                    resource->setMinFilterMode(minFilterMode);
                    resource->setMagFilterMode(magFilterMode);
                    if (format != format0)
                    {
                        if (resource->clientUniforms_.size() > 0)
                            Layer::Flags::requestRecompilation = true;
                    }
                }
            }
            else
            {
                auto newResource = 
                    Resource::create
                    (
                        resolution.x,
                        resolution.y,
                        internalFormat
                    );
                if (newResource != nullptr)
                {
                    resource = newResource;
                    valid = true;
                }
            }
            resolution.x = resource->width();
            resolution.y = resource->height();
            internalFormat = ((Texture2DResource*)resource)->internalFormat();
        }
        if (!disabled || resource == nullptr)
        {
            createOrResizeOrReformatTextureMemoryEstimateGui
            (
                uint64_t(resolution.x)*uint64_t(resolution.y),
                internalFormat,
                true
            );
        }
        if (disabled)
            ImGui::EndDisabled();
        if (enablePopup)
            ImGui::EndPopup();
    }
    return valid;
}

*/

} // End of GUI namespace

} // End of ShaderThing namespace