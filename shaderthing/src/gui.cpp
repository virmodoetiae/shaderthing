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

void renderControlPanel(AppState& appState)
{
    // Move to deferred update
    //font_.checkLoadJapaneseAndOrSimplifiedChinese();
    
    vir::ImGuiRenderer::newFrame();
    
    ImGui::SetNextWindowSize(ImVec2(750,750), ImGuiCond_FirstUseEver);
    static ImGuiWindowFlags flags
    (
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse
    );
    ImGui::Begin(appState.controlPanelTitle.c_str(), NULL, flags);

    // Refresh icon if needed
    static bool isIconSet(false);
    static bool isWindowDocked(ImGui::IsWindowDocked());
    if (!isIconSet || isWindowDocked != ImGui::IsWindowDocked())
    {
        isIconSet = vir::ImGuiRenderer::setWindowIcon
        (
            appState.controlPanelTitle.c_str(), 
            ByteData::Icon::sTIconData, 
            ByteData::Icon::sTIconSize,
            false
        );
        isWindowDocked = ImGui::IsWindowDocked();
    }
    
    renderMenuBar(appState);
    renderLayersTabBar(appState);

    ImGui::End();
    
    vir::ImGuiRenderer::render();
}

//----------------------------------------------------------------------------//

void renderMenuBar(AppState& appState)
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
            {
                appState.fileDialog.runOpenFileDialog
                (
                    "Open project",
                    {"ShaderThingOld file (*.stf)", "*.stf *.stf.bak"},
                    ".",
                    false
                );
                appState.deferredActionBuffer.add
                (
                    [&appState]()
                    {
                        auto filepath = appState.fileDialog.selection().front();
                        appState.project.forceSaveAs = true;
                        loadFrom(appState,filepath,false);
                        appState.project.filepath = // Trim .bak if applicable
                            (
                                filepath.size() >= 4 && 
                                filepath.substr(filepath.size() - 4) == ".bak"
                            ) ? 
                            filepath.substr(0, filepath.size() - 4) : 
                            filepath;
                        appState.project.filename = Helpers::filename
                        (
                            appState.project.filepath
                        );
                        appState.fileDialog.clearSelection();
                    },
                    [&appState]()
                    {
                        return appState.fileDialog.validSelection();
                    }
                );
            }//setProjectAction(Project::Action::Load, project_, fileDialog_);
            if (ImGui::MenuItem("Save", "Ctrl+S"))
                {}//setProjectAction(Project::Action::Save, project_, fileDialog_);
            if (ImGui::MenuItem("Save as", "Ctrl+Shift+S"))
            {
                appState.fileDialog.runSaveFileDialog
                (
                    "Save project",
                    {"ShaderThing file (*.stf)", "*.stf"},
                    appState.project.filepath.size() == 0 ? 
                    appState.project.filename.c_str() :
                    appState.project.filepath.c_str()
                );
                appState.deferredActionBuffer.add
                (
                    [&appState]()
                    {
                        saveTo
                        (
                            appState, 
                            appState.fileDialog.selection().front()
                        );
                    },
                    [&appState]()
                    {
                        return appState.fileDialog.validSelection();
                    }
                );
            }//setProjectAction(Project::Action::SaveAs,project_,fileDialog_);
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
                auto& su = *(appState.sharedUniforms);
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
                        appState,
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
                        &appState.renderState.isVSyncEnabled
                    )
                )
                    window->setVSync(appState.renderState.isVSyncEnabled);

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
                int nRenderingTiles = appState.renderState.nTiles;
                if (appState.renderState.isPaused)
                    ImGui::BeginDisabled();
                if (ImGui::InputInt("##nRenderingTiles", &nRenderingTiles))
                {
                    setRenderingTiles(appState, nRenderingTiles);
                }
                if (appState.renderState.isPaused)
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
                        &appState.renderState.lowerFpsLimit, 
                        0.f, 
                        0.f, 
                        "%.1f"
                    )
                )
                    appState.renderState.lowerFpsLimit = 
                        std::max(appState.renderState.lowerFpsLimit, 0.f);
                ImGui::SameLine();
                ImGui::PopItemWidth();
                ImGui::Text("fps");

                if 
                (
                    ImGui::Button
                    (
                        !appState.renderState.isPaused ? 
                        "Pause rendering" : "Resume rendering", 
                        ImVec2(-1, 0)
                    )
                )
                    toggleRenderingPaused(appState, false);

                if (ImGui::Button("Capture mouse cursor", ImVec2(-1, 0)))
                    setMouseCaptured(appState, true);

                ImGui::EndMenu();
            }

            for (auto& layer : appState.layers)
                layer->renderMenuItemGui();
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
            renderResourcesMenuItem(appState);
            shadersRequireRecompilation = 
                appState.sharedStorage->renderMenuItemGui();
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
        renderResourcesTable(appState);
    if (appState.sharedStorage->isGuiDetachedFromMenu())
        shadersRequireRecompilation = 
            appState.sharedStorage->renderGui();
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
        for (auto& layer : appState.layers)
        {
            layer->compileShader();
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
            appState.deferredActionBuffer.add
            (
                [&appState]()
                {
                    initialize(appState);
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

void renderLayersTabBar(AppState& appState)
{
    auto& layers = appState.layers;
    /*
    if (appState.renderState.toggles.requestFullRecompilation)
    {
        for (auto& layer : layers)
        {
            layer->hasUncompiledEdits = true;
        }
        appState.renderState.toggles.requestFullRecompilation = false;
    }
    */
    appState.layersHaveUncompiledEdits = false;
    appState.layersHaveCompilationErrors = false;
    for (auto& layer : layers)
    {
        appState.layersHaveUncompiledEdits = 
            appState.layersHaveUncompiledEdits ||
            layer->hasUncompiledEdits;
        appState.layersHaveCompilationErrors = 
            appState.layersHaveCompilationErrors ||
            layer->hasCompilationErrors();
    }
    if 
    (
        appState.layersHaveUncompiledEdits || 
        appState.layersHaveCompilationErrors
    ) // Render compilation button 
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
                layer->compileShader();
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
    if (appState.layersHaveCompilationErrors)
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, {1,0,0,1});
        errorColorPushed = true;
        ImGui::Text("Compilation errors in:");
    }
    Layer::renderSharedCompilationErrorsGui();
    for (auto& layer : layers) 
    {
        layer->renderCompilationErrorsGui();
    }
    if (appState.layersHaveCompilationErrors)
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
            createNewLayer(appState);
        auto tabBar = ImGui::GetCurrentTabBar();
        std::pair<unsigned int, unsigned int> swap {0,0};
        for (int i = 0; i < (int)layers.size(); i++)
        {
            bool open = true;
            auto& layer = layers[i];
            std::string tabLabel = layer->name()+"###"+layer->imGuiTabId;
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
                layer->renderTabGui();
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
                    layer->name().c_str()
                );
                ImGui::Text("This action cannot be undone!");
                if (ImGui::Button("Delete"))
                {
                    appState.deferredActionBuffer.add
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
            const auto d1 = layers[swap.first]->depth();
            const auto d2 = layers[swap.second]->depth();
            std::swap
            (
                layers[swap.first], 
                layers[swap.second]
            );
            layers[swap.first]->setDepth(d1);
            layers[swap.second]->setDepth(d2);
            swap = {0, 0};
        }
        ImGui::EndTabBar();
    }
}

//----------------------------------------------------------------------------//

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

void renderUniformsTab(Layer* layer, AppState& appState)
{
    //--------------------------------------------------------------------------
    auto& su = *(appState.sharedUniforms);
    float fontSize = ImGui::GetFontSize();
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
            row = renderBuiltInSharedUniforms(appState);

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
            renderUniformTableRow
            (
                uniform,
                layer,
                appState,
                row++,
                i == nSharedUniforms-1
            );
        }

        // Finally, render the user-created layer-specific uniforms
        for(unsigned int i=0; i<layer->uniforms.size(); i++)
        {
            auto& uniform = layer->uniforms[i];
            renderUniformTableRow
            (
                uniform,
                layer,
                appState,
                row++,
                false,
                showSharedAndDefaultUniforms
            );
        }

        // Render the "Create new uniform" button
        START_ROW(row, column)
        START_COLUMN(column)
        if (ImGui::Button(ICON_FA_PLUS, ImVec2(-1, 0)))
        {
            auto pLayer = layer->weakFromThis();
            auto& u = Uniform::create(pLayer);
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
}

//----------------------------------------------------------------------------//

// Render the default/built-in shared uniforms as a table and return the row
// count
int renderBuiltInSharedUniforms(AppState& appState)
{
    auto& su = *(appState.sharedUniforms);
    int row = 0;
    int column;
    float fontSize = ImGui::GetFontSize();
    float halfButtonSize(1.7*fontSize);

    // iFrame --------------------------------------------------------------
    START_ROW(row, column)
    NEXT_COLUMN(column)
    if (ImGui::Button(ICON_FA_UNDO, ImVec2(halfButtonSize, 0)))
    {
        // Restart rendering
        appState.deferredActionBuffer.add
        (
            [&appState]()
            {
                appState.renderState.frameIndex = 0;
                if (appState.sharedUniforms->isTimeResetOnFrameCounterReset)
                    appState.sharedUniforms->iTime = 0;
                for (auto& layer : appState.layers)
                {
                    layer->clearFramebuffers();
                }
            }
        );
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
            appState.renderState.isPaused ? 
            ICON_FA_PLAY : 
            ICON_FA_PAUSE, 
            ImVec2(-1, 0)
        )
    )
    {
        toggleRenderingPaused(appState, false);
        // When stopping rendering while tiled rendering is enabled,
        // make sure to render all the tiles to reach the end of the
        // shader frame
        if
        (
            appState.renderState.isPaused && 
            appState.renderState.isTiledRenderingEnabled
        )
            appState.renderState.toggles.stepToNextFrame = true;
    }
    if (appState.renderState.isPaused)
    {
        if (ImGui::Button(ICON_FA_STEP_FORWARD, {-1,0}))
            appState.renderState.toggles.stepToNextFrame = true;
        else
        {
            // If tiled rendering is enabled, stepping by one shader frame
            // means stepping by nTiles app frames (since each app frame 
            // only renders a single shader tile), so the frame step is over
            // only once all tiles have been rendered (i.e., when tileIndex
            // is reset to 0)
            if (appState.renderState.isTiledRenderingEnabled)
            {
                if (appState.renderState.tileIndex == 0)
                    appState.renderState.toggles.stepToNextFrame = false;
            }
            else
                appState.renderState.toggles.stepToNextFrame = false;
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
    ImGui::Text("%d", appState.renderState.frameIndex);
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
        ) && !appState.renderState.isPaused
    )
        su.isTimePaused = 
            !su.isTimePaused;
    if (su.isTimePaused)
    {
        if (ImGui::Button(ICON_FA_STEP_FORWARD, {-1,0}))
            appState.renderState.toggles.stepToNextFrame = true;
        else 
            appState.renderState.toggles.stepToNextFrame = false;
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
        appState.renderState.isPaused || 
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
        toggleKeyboardInputs(appState);
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
        toggleMouseInputs(appState);
    if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1, 0)))
        ImGui::OpenPopup("##iMouseSettings");
    if (ImGui::BeginPopup("##iMouseSettings"))
    {
        bool enabled = su.isMouseInputEnabled;
        std::string text = enabled ? "Disable inputs" : "Enable inputs";
        if (ImGui::Button(text.c_str(), ImVec2(20*fontSize, 0)))
            toggleMouseInputs(appState);
        ImGui::Text("Clamp value to window resolution ");
        ImGui::SameLine();
        bool status = su.isMouseInputClampedToWindow;
        ImGui::Checkbox("##iMouseSettings_ClampValue", &status);
        if (status != su.isMouseInputClampedToWindow)
            setMouseInputsClamped(appState, status);
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
            toggleCameraMouseInputs(appState);
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
            toggleCameraKeyboardInputs(appState);
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
    UPtr<Uniform>& uniform,
    Layer* layer,
    AppState& appState,
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
    bool typeChanged = false;
    auto& su = *(appState.sharedUniforms);
    auto& resources = appState.resources;
    
    START_ROW(row, column)

    START_COLUMN(column) // Action column --------------------------------------
    float y0 = 0;
    if (!managed)
    {
        float halfButtonSize(1.7*fontSize);
        if (ImGui::Button(ICON_FA_TRASH, ImVec2(halfButtonSize, 0)))
        {
            updateLayersDueToUniformDeletion(uniform, appState);
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
            // TODO Test this approach of updating sharedness status
            uniform->isSharedByUser = !uniform->isSharedByUser;
            appState.deferredActionBuffer.add
            (
                [&uniform, layer, &appState]()
                {
                    if (uniform->isSharedByUser)
                        uniform->setOwner(&appState.sharedUniforms->fragment);
                    else 
                        uniform->setOwner(layer);
                    for (auto& l : appState.layers)
                    {
                        l->compileShader();
                    }
                }
            );
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
        ImGui::Text(uniform->name().c_str());
    else
    {
        if (ImGui::InputText("##uniformName", &uniform->name()))
        {
            uniform->markForSubmissionToAllClientBuffers();
            Helpers::enforceUniqueName
            (
                uniform->name(),
                layer->uniforms,
                uniform.get(),
                true
            );
            updateLayersDueToUniformTypeOrNameChanged
            (
                uniform, 
                appState,
                false // Do NOT recompile shaders automatically on rename
            );
        }
    }
    bool named(uniform->name().size() > 0);
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
            updateLayersDueToUniformTypeOrNameChanged
            (
                uniform, 
                appState,
                true // Recompile shaders automatically on type change
            );
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
                appState.renderState.toggles.requestFullRecompilation =        \
                    appState.renderState.toggles.requestFullRecompilation ||   \
                    (resource->isInternalFormatUnsigned() !=                   \
                    r->isInternalFormatUnsigned() && named);                   \
                if (resource->isUsedByUniform(uniform.get()))                  \
                    resource->removeClientUniform(uniform.get());              \
            }                                                                  \
            else if (named)                                                    \
                appState.renderState.toggles.requestFullRecompilation = true;  \
            if (!r->isUsedByUniform(uniform.get()))                            \
                r->addClientUniform(uniform.get());                            \
            appState.deferredActionBuffer.add                                  \
            (                                                                  \
                [&uniform, &r]()                                               \
                {uniform->setResourcePtr(r);}                                  \
            );                                                                 \
            su.iUserAction = true;                                             \
            su.toggles.updateDataRangeII = true;                               \
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

    return typeChanged;
    
}; // End of renderUniform

//----------------------------------------------------------------------------//
// Resources -----------------------------------------------------------------//
//----------------------------------------------------------------------------//

void renderResourcesMenuItem(AppState& appState)
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
            renderResourcesTable(appState);
            ImGui::EndMenu();
        }
        else
            Resource::gui.isOpen = false;
        return;
    }
    ImGui::MenuItem("Resource manager", NULL, &Resource::gui.isOpen);
}

//----------------------------------------------------------------------------//

void renderResourcesTable(AppState& appState)
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

        const int nRows = appState.resources.size();
        for (int row=0; row<nRows; row++)
        {
            renderResourcesTableRow(appState, row);
        }
        renderAddResourceButton(appState, nRows);
        tableHeight = (ImGui::GetCursorPosY()-cursorPosY0);
        ImGui::EndTable();
    }

    if (Resource::gui.isDetachedFromControlPanel)
        ImGui::End();
}

//----------------------------------------------------------------------------//

void renderResourcesTableRow(AppState& appState, int row)
{
    UPtr<Resource>& resource = appState.resources[row];
    float fontSize = ImGui::GetFontSize();
    int column = 0;
    START_ROW(row, column)
    START_COLUMN(column) // Actions column -------------------------------------
    renderResourceActionsButton(appState, row);
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
                appState.resources,
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

void renderAddResourceButton(AppState& appState, int row)
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
            appState.deferredActionBuffer.add
            (
                [&appState]()
                {
                    auto filepath = Resource::fileDialog.selection().front();
                    auto& r = appState.resources.emplace_back
                    (
                        Texture2DResource::create(filepath)
                    );
                    std::string name = Helpers::filename(filepath);
                    Helpers::enforceUniqueName
                    (
                        name, 
                        appState.resources, 
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

void renderResourceActionsButton(AppState& appState, int row)
{
    if (row >= (int)appState.resources.size())
        return;
    UPtr<Resource>& resource = appState.resources[row];
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
            appState.resources[row].dynamicDowncastTo<LayerResource>();
        resource->layer()->renderFramebufferSettingsGui();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##resourceActions"))
    {
        auto size = ImVec2(12*ImGui::GetFontSize(), 0);
        if (ImGui::Button(ICON_FA_TRASH, size))
        {
            appState.deferredActionBuffer.add
            (
                [&appState, row]()
                {
                    // UPtr<Resource>& resource = appState.resources[row];
                    // Used to call legacy 'removeResourceFromUniforms' on all
                    // layers, tentatively removed
                    appState.resources.erase(appState.resources.begin()+row);
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