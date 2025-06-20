#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/corelogic.h"
#include "shaderthing/include/gui.h"
#include "shaderthing/include/helpers.h"
#include "shaderthing/include/oo/texteditor.h"
#include "shaderthing/include/structs.h"
#include "vir/include/vir.h"
#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/icons/IconsFontAwesome5.h"

namespace ShaderThing
{

namespace GUI
{

void renderControlPanel
(
    AppData& appData
)
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

void renderLayerMenu
(
    Layer& layer, 
    AppData& appData
)
{
    if (ImGui::BeginMenu(("Layer ["+layer.name+"]###"+layer.imGuiMenuId).c_str()))
    {
        auto& renderer = layer.renderer;
        auto& flags = layer.flags;
        const float fontSize(ImGui::GetFontSize());
        const float entryWidth(14*fontSize);
        ImGui::Text("Name                 ");
        ImGui::SameLine();
        static std::unique_ptr<char[]> label(new char[24]);
        std::sprintf(label.get(), "##layer%dInputText", layer.id);
        ImGui::PushItemWidth(entryWidth);
        ImGui::InputText(label.get(), &layer.name);
        ImGui::PopItemWidth();
        
        static std::map<Layer::Renderer::Target, const char*> 
        renderTargetToName
        {
            {
                Layer::Renderer::Target::InternalFramebufferAndWindow, 
                "Framebuffer & window"
            },
            {Layer::Renderer::Target::InternalFramebuffer, "Framebuffer"},
            {Layer::Renderer::Target::Window, "Window"}
        };
        ImGui::Text("Render target        ");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        if 
        (
            ImGui::BeginCombo
            (
                "##rendererTarget", 
                renderTargetToName.at(renderer.target)
            )
        )
        {
            for(auto entry : renderTargetToName)
            {
                if (!ImGui::Selectable(entry.second))
                    continue;
                auto target = entry.first;
                if (target != renderer.target)
                {
                    renderer.target = target;
                    /*if (renderer.target == Layer::Renderer::Target::Window)
                        LayerResource::removeFromResources
                        (
                            this,
                            resources
                        );
                    else
                        LayerResource::insertInResources
                        (
                            this,
                            resources
                        );*/
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        if 
        (
            renderer.target == Layer::Renderer::Target::Window || 
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
                flags.isAspectRatioBoundToWindow ? 
                " " ICON_FA_LOCK " " : 
                " " ICON_FA_LOCK_OPEN " "
            )
        )
        {
            flags.isAspectRatioBoundToWindow = 
                !flags.isAspectRatioBoundToWindow;
            if (flags.isAspectRatioBoundToWindow)
            {
                auto window = vir::Window::instance();
                glm::ivec2 resolution = {window->width(), window->height()};
                //setResolution(resolution, false);
            }
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text(
                flags.isAspectRatioBoundToWindow ?
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
        glm::ivec2 resolution = layer.resolution;
        std::sprintf(label.get(), "##layer%dResolution", layer.id);
        if (ImGui::InputInt2(label.get(), glm::value_ptr(resolution)))
            {}//setResolution(resolution, false, true);
        ImGui::PopItemWidth();
        ImGui::Text("Auto-resize mode     ");
        ImGui::SameLine();
        if
        (
            ImGui::Button
            (
                flags.rescaleWithWindow ?
                "Rescale on window resize" :
                "Do not auto-resize",
                {-1, 0}
            )
        )
            flags.rescaleWithWindow = !flags.rescaleWithWindow;
        if 
        (
            renderer.target == Layer::Renderer::Target::Window || 
            vir::Window::instance()->iconified()
        )
            ImGui::EndDisabled();
    
        if (renderer.target != Layer::Renderer::Target::Window)
        {
            ImGui::SeparatorText("Framebuffer settings");
            //renderFramebufferPropertiesGui();

            /*
            ImGui::SeparatorText("Post-processing effects");
            int iDelete = -1;
            int iSrc = -1; 
            int iTrg = -1;
            int nPostProcesses = renderer.postProcesses.size();
            for (int i = 0; i < nPostProcesses; i++)
            {
                auto& postProcess = renderer.postProcesses[i];
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
                renderer.postProcesses.erase
                (
                    renderer.postProcesses.begin() + iDelete
                );
            else if (iSrc != iTrg)
                std::swap
                (
                    renderer.postProcesses[iSrc], 
                    renderer.postProcesses[iTrg]
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
                for (auto& postProcess : renderer.postProcesses)
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
                            renderer.postProcesses.emplace_back
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

void renderLayersTabBar
(
    AppData& appData
)
{
    auto& layers = appData.layers;
    static bool compilationErrors(false);
    static bool anyUncompiledChanges(false);
    /*if (Flags::requestRecompilation)
    {
        for (auto layer : layers)
        {
            layer->flags.uncompiledChanges = true;
        }
        Flags::requestRecompilation = false;
    }*/
    if (anyUncompiledChanges || compilationErrors) // Render compilation button 
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
            //for (auto& layer : layers)
            //    layer->compileShader(sharedUnifoms);
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
    anyUncompiledChanges = false;
    const auto& sharedErrors // First render errors in shared source -----------
    (
        appData.sharedSourceEditor->getErrorMarkers()
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
        const auto& sourceErrors(layer->sourceEditor->getErrorMarkers());
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
            layer->sourceEditor->isTextChanged() || 
            appData.sharedSourceEditor->isTextChanged()
        )
            layer->flags.uncompiledChanges = true;
        if 
        (
            appData.sharedSourceEditor->isTextChanged() ||
            layer->flags.uncompiledChanges
        )
            anyUncompiledChanges = true;
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
        {
            createNewLayer(appData);
            if (appData.renderer.isTiledRenderingEnabled)
            {    
                /*
                Layer::setRenderingTiles
                (
                    layers, 
                    Layer::Rendering::TileController::nTiles
                );
                */
            }
        }
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
                renderLayerTabBar(*layer, appData);
                ImGui::EndTabItem();
            }
            if (!open) // I.e., if 'x' is pressed to delete the tab
            {
                ImGui::OpenPopup("Layer deletion confirmation");
                layer->flags.isDeletionConfirmationPending = true;
                // For some reason, when the 'x' is pressed, the tab in question
                // is moved 
                reorderable = false;
            }
            if 
            (
                layer->flags.isDeletionConfirmationPending && 
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
                    layer->flags.isDeletionConfirmationPending = false;
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
            setLayerDepth(*layers[swap.first], d1);
            setLayerDepth(*layers[swap.second], d2);
            swap = {0, 0};
        }
        ImGui::EndTabBar();
    }

    // Check if layers framebuffers should be cleared as a consequence of
    // a rendering restart. This flag is set in the lambda
    // Uniform::renderUniformsGui::renderSharedUniformsGui eventually called by
    // renderTabBarGui
    /*if (Layer::Flags::restartLayer::Renderer)
    {
        for (auto& layer : layers)
        {
            layer->renderer.framebufferA->clearColorBuffer();
            layer->renderer.framebufferB->clearColorBuffer();
        }
        Layer::Flags::restartLayer::Renderer = false;
    }*/
}

//----------------------------------------------------------------------------//

void renderLayerTabBar
(
    Layer& layer,
    AppData& appData
)
{
    static unsigned int gActiveTabId = 0;
    static unsigned int gActiveLayerId = 0;
    bool layerChanged = (gActiveLayerId != layer.id);
    if (layerChanged)
        gActiveLayerId = layer.id;
    if (ImGui::BeginTabBar("##layerTabBar"))
    {
        if (layerChanged && layer.activeGuiTabId != gActiveTabId)
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
            bool headerErrors(layer.headerErrors.size() > 0);
            bool madeReplacements = false;
                //layer.sourceEditor.renderFindReplaceToolGui();
            layer.flags.uncompiledChanges = 
                layer.flags.uncompiledChanges || madeReplacements;
            if (ImGui::TreeNode("Header"))
            {
                float indent(layer.sourceEditor->getLineIndexColumnWidth());
                ImGui::Unindent(); // Remove indent from Header TreeNode
                ImGui::Indent(indent);
                ImGui::PushStyleColor
                (
                    ImGuiCol_Text, 
                    ImGui::GetStyle().Colors[ImGuiCol_TextDisabled] // Gray
                );
                ImGui::Text(layer.sourceHeader.c_str());
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
                    ImGui::Text(layer.headerErrors.c_str());
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
            layer.sourceEditor->renderGui("##sourceEditor");
            gActiveTabId = 0;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Shared source"))
        {
            bool madeReplacements = 
                appData.sharedSourceEditor->renderFindReplaceToolGui();
            layer.flags.uncompiledChanges = 
                layer.flags.uncompiledChanges || madeReplacements;
            appData.sharedSourceEditor->renderGui("##sharedSourceEditor");
            gActiveTabId = 1;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Uniforms"))
        {
            ImGui::Text("Uniforms!");
            /*
            Uniform::renderUniformsGui
            (
                sharedUnifoms, 
                this,
                layers,
                resources
            );
            */
            gActiveTabId = 2;
            ImGui::EndTabItem();
        }
        layer.activeGuiTabId = gActiveTabId;
        ImGui::EndTabBar();
    }
}

//----------------------------------------------------------------------------//

void renderMenuBar
(
    AppData& appData
)
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
                /*
                ImGui::Text("Resolution         ");
                ImGui::SameLine();
                ImGui::PushItemWidth(8.0*ImGui::GetFontSize());
                glm::ivec2 resolution(sharedUniforms_->iResolution());
                if 
                (
                    ImGui::InputInt2
                    (
                        "##windowResolution", 
                        glm::value_ptr(resolution)
                    )
                )
                    sharedUniforms_->setResolution(resolution, false);
                ImGui::PopItemWidth();

                auto window = vir::Window::instance();
                ImGui::Text("VSync              ");
                ImGui::SameLine();
                if (ImGui::Checkbox("##windowVSync", &windowSettings_.isVSyncEnabled))
                    window->setVSync(windowSettings_.isVSyncEnabled);

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
                int nLayer::RendererTiles = Layer::Layer::Renderer::TileController::nTiles;
                if (sharedUniforms_->isLayer::RendererPaused())
                    ImGui::BeginDisabled();
                if (ImGui::InputInt("##nLayer::RendererTiles", &nLayer::RendererTiles))
                {
                    nLayer::RendererTiles = std::max(nLayer::RendererTiles, 1);
                    Layer::setLayer::RendererTiles(layers_, nLayer::RendererTiles);
                }
                if (sharedUniforms_->isLayer::RendererPaused())
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
                        &windowSettings_.lowerFpsLimit, 
                        0.f, 
                        0.f, 
                        "%.1f"
                    )
                )
                    windowSettings_.lowerFpsLimit = 
                        std::max(windowSettings_.lowerFpsLimit, 0.f);
                ImGui::SameLine();
                ImGui::PopItemWidth();
                ImGui::Text("fps");

                if 
                (
                    ImGui::Button
                    (
                        !sharedUniforms_->isLayer::RendererPaused() ? 
                        "Pause rendering" : "Resume rendering", 
                        ImVec2(-1, 0)
                    )
                )
                    sharedUniforms_->toggleLayer::RendererPaused();

                if (ImGui::Button("Capture mouse cursor", ImVec2(-1, 0)))
                    sharedUniforms_->setMouseCaptured(true);
                */
                ImGui::EndMenu();
            }

            for (auto& layer : appData.layers)
                renderLayerMenu(*layer, appData);
            /*
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
            /*
            Resource::renderResourcesMenuItemGui(resources_, layers_);
            shadersRequireRecompilation = 
                Layer::Layer::Renderer::sharedStorage->renderMenuItemGui();
            */
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Find"))
        {
            TextEditor::renderFindReplaceToolMenuGui();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Preferences"))
        {
            /*
            font_.renderMenuItemGui();
            project_.renderAutoSaveMenuItemGui();
            */
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            /*
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
                    vir::Layer::Renderer::instance()->
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
    /*
    if (Resource::isGuiDetachedFromMenu)
        Resource::renderResourcesGui(resources_, layers_);
    if (Layer::Layer::Renderer::sharedStorage->isGuiDetachedFromMenu())
        shadersRequireRecompilation = 
            Layer::Layer::Renderer::sharedStorage->renderGui();
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

    if (shadersRequireRecompilation)
    {
        for (auto layer : layers_)
        {
            layer->compileShader(*sharedUniforms_);
        }
    }
    */

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
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

}

}