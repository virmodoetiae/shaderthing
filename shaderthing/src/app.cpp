/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2025 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include <charconv>

#include "shaderthing/include/app.h"

#include "shaderthing/include/about.h"
#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/coderepository.h"
#include "shaderthing/include/examples.h"
#include "shaderthing/include/exporter.h"
#include "shaderthing/include/helpers.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/objectio.h"
#include "shaderthing/include/postprocess.h"
#include "shaderthing/include/resource.h"
#include "shaderthing/include/sharedstorage.h"
#include "shaderthing/include/shareduniforms.h"
#include "shaderthing/include/statusbar.h"

#include "vir/include/vir.h"

namespace ShaderThing
{

App::App()
{
    // Initialize vir lib
    vir::Settings settings = {};
    settings.windowName = "ShaderThing - "+project_.filename;
    settings.enableFaceCulling = false;
    vir::initialize(settings);

    auto window = vir::Window::instance();
    window->setIcon
    (
        (unsigned char*)ByteData::Icon::sTIconData,
        ByteData::Icon::sTIconSize,
        false
    );
    
    font_.initialize();

    newProject();

    // Main loop
    while(window->isOpen())
    {   
        processProjectActions();
        renderGui();
        update();

        auto result = Layer::renderShaders
        (
            layers_, 
            exporter_->isRunning() ? exporter_->framebuffer() : nullptr, 
            *sharedUniforms_,
            exporter_->isRunning() ? exporter_->nRenderPasses() : 1
        );
        if (exporter_->isRunning() && result.renderPassesComplete)
            exporter_->writeOutput();
        window->update(result.flipWindowBuffer);
    }
}

//----------------------------------------------------------------------------//

App::~App()
{
    DELETE_IF_NOT_NULLPTR(exporter_)
    DELETE_IF_NOT_NULLPTR(sharedUniforms_)
    for (auto resource : resources_)
    {
        DELETE_IF_NOT_NULLPTR(resource)
    }
    for (auto layer : layers_)
    {
        DELETE_IF_NOT_NULLPTR(layer)
    }
}

//----------------------------------------------------------------------------//

void App::update()
{
    exporter_->update(*sharedUniforms_, layers_, resources_);

    bool advanceFrame;
    float timeStep;
    if (exporter_->isRunning())
    {
        if (sharedUniforms_->iRenderPass() == (int)exporter_->nRenderPasses()-1)
        {
            advanceFrame = true;
            timeStep = exporter_->timeStep();
        }
        else
        {
            advanceFrame = false;
            timeStep = 0;
        }
    }
    else
    {
        timeStep = (sharedUniforms_->isTimeDeltaSmooth() ?
            vir::Window::instance()->time()->smoothOuterTimestep() : 
            vir::Window::instance()->time()->outerTimestep());
        if (Layer::Rendering::TileController::tiledRenderingEnabled)
        {
            static float cumulatedTimeStep = 0;
            if (!sharedUniforms_->isRenderingPaused())
                cumulatedTimeStep += timeStep;
            if (Layer::Rendering::TileController::tileIndex == 0)
            {
                timeStep = cumulatedTimeStep;
                cumulatedTimeStep = 0;
                advanceFrame = true;
            }
            else
            {
                timeStep = 0;
                advanceFrame = false;
            }
        }
        else
            advanceFrame = true;
    }

    sharedUniforms_->update(             {advanceFrame,             timeStep});
    Resource::       update( resources_, {sharedUniforms_->iTime(), timeStep});
    
    // Auto-save if applicable
    if 
    (
        project_.isAutoSaveEnabled && 
        project_.filepath.size() > 0 && 
        !exporter_->isRunning()
    )
    {
        if (project_.timeSinceLastSave > project_.autoSaveInterval)
            saveProject(project_.filepath+".bak", true);
        else
            project_.timeSinceLastSave += 
                vir::Window::instance()->time()->outerTimestep();
    }

    // Compute fps and set in window title, also, check if rendering should stop
    // if fps too low for too long
    static int elapsedFrames(0);
    static float elapsedTime(0);
    static int fpsUpdateCounter(0);
    static bool shouldStopRendering(true);
    float fpsUpdatePeriod = 0.5f;
    float maxLowFpsPeriod = 2.0f;

    elapsedFrames++;
    elapsedTime += vir::Window::instance()->time()->outerTimestep();
    
    if (elapsedTime >= fpsUpdatePeriod)
    {
        double fps = elapsedFrames/elapsedTime;
        if (Layer::Rendering::TileController::tiledRenderingEnabled)
        {
            double wFps = fps/Layer::Rendering::TileController::nTiles;
            imGuiTitle_ = 
                "Control panel - "+project_.filename+" (window: "+
                Helpers::format(wFps,1)+" fps | GUI: "+
                Helpers::format(fps,1)+" fps)"+"###CP";
        }
        else
            imGuiTitle_ = 
                "Control panel - "+project_.filename+" ("+
                Helpers::format(fps,1)+" fps)"+"###CP";
        elapsedFrames = 0;
        elapsedTime = 0;
        if (!exporter_->isRunning())
        {
            fpsUpdateCounter++;
            shouldStopRendering = 
                shouldStopRendering && fps < sharedUniforms_->lowerFpsLimit();
            if (fpsUpdateCounter >= int(maxLowFpsPeriod/fpsUpdatePeriod))
            {
                if (shouldStopRendering && !sharedUniforms_->isRenderingPaused())
                    sharedUniforms_->toggleRenderingPaused(true); 
                fpsUpdateCounter = 0;
                shouldStopRendering = true;
            }
        }
    }
}

//----------------------------------------------------------------------------//

void App::saveProject(const std::string& filepath, bool isAutosave) const
{
    auto project = ObjectIO(filepath.c_str(), ObjectIO::Mode::Write);
    
    project.write("UIScale", *font_.fontScale);
    project.write("autoSaveEnabled", project_.isAutoSaveEnabled);
    project.write("autoSaveInterval", project_.autoSaveInterval);
    project.write("vSyncEnabled", windowSettings_.isVSyncEnabled);
    
    Resource::       saveAll(resources_, project);
    sharedUniforms_->save   (            project);
    Layer::          saveAll(layers_,    project);
    exporter_->      save   (            project);
    PostProcess::    saveStaticData(     project);

    // TODO Could display an error via ImGui on failure
    project.writeContentsToDisk();

    project_.timeSinceLastSave = 0;

    StatusBar::queueTemporaryMessage
    (
        isAutosave ? "Project auto-saved" : "Project saved",
        StatusBar::defaultMessageDuration,
        0xff25ff50
    );
}

//----------------------------------------------------------------------------//
    
void App::loadProject(const std::string& filepathOrData, bool fromMemory)
{
    auto project = 
        fromMemory ? 
        ObjectIO(filepathOrData) : 
        ObjectIO(filepathOrData.c_str(), ObjectIO::Mode::Read);
    if (!project.isValid())
        return; // TODO Could display an error via ImGui
    
    *font_.fontScale = project.read<float>("UIScale");
    project_.isAutoSaveEnabled = project.readOrDefault<bool>
    (
        "autoSaveEnabled", 
        Project{}.isAutoSaveEnabled
    );
    project_.autoSaveInterval = project.readOrDefault<float>
    (
        "autoSaveInterval", 
        Project{}.autoSaveInterval
    );
    windowSettings_.isVSyncEnabled = 
        project.readOrDefault<bool>("vSyncEnabled", true);
    vir::Window::instance()->setVSync(windowSettings_.isVSyncEnabled);
    
    Resource::      loadAll(project,                            resources_);
    SharedUniforms::load   (project,           sharedUniforms_, resources_);
    Layer::         loadAll(project, layers_, *sharedUniforms_, resources_);
    Exporter::      load   (project, exporter_                            );
    PostProcess::   loadStaticData(project);
}

//----------------------------------------------------------------------------//

void App::newProject()
{
    project_ = Project{};
    *font_.fontScale = .6;

    windowSettings_ = WindowSettings{};
    vir::Window::instance()->setVSync(windowSettings_.isVSyncEnabled);

    DELETE_IF_NOT_NULLPTR(exporter_);
    DELETE_IF_NOT_NULLPTR(sharedUniforms_);
    for (auto resource : resources_)
    {
        DELETE_IF_NOT_NULLPTR(resource)
    }
    resources_.clear();
    for (auto layer : layers_)
    {
        DELETE_IF_NOT_NULLPTR(layer)
    }
    layers_.clear();
    
    vir::Window::instance()->setSize(512, 512);

    sharedUniforms_ = new SharedUniforms();
    Layer::Rendering::sharedStorage.reset();
    Layer::resetSharedSourceEditor();
    layers_.emplace_back(new Layer(layers_, *sharedUniforms_));
    Layer::setRenderingTiles(layers_, 1); // Turn tiled rendering off
    exporter_ = new Exporter();
}

//----------------------------------------------------------------------------//

void App::processProjectActions()
{
    switch (project_.action)
    {
        case Project::Action::None :
            return;
        case Project::Action::New :
            newProject();
            project_.action = Project::Action::None;
            break;
        case Project::Action::Save :
            saveProject(project_.filepath, false);
            project_.action = Project::Action::None;
            break;
        case Project::Action::Load :
        {
            if (!fileDialog_.validSelection())
            {
                if (!fileDialog_.isOpen())
                    project_.action = Project::Action::None;
                break;
            }
            auto filepath = fileDialog_.selection().front();
            project_.forceSaveAs = true;
            loadProject(filepath);
            project_.filepath = // Trim .bak if applicable
                (
                    filepath.size() >= 4 && 
                    filepath.substr(filepath.size() - 4) == ".bak"
                ) ? 
                filepath.substr(0, filepath.size() - 4) : 
                filepath;
            project_.filename = Helpers::filename(project_.filepath);
            fileDialog_.clearSelection();
            project_.action = Project::Action::None;
            break;
        }
        case Project::Action::LoadExample :
            project_.forceSaveAs = true;
            project_.filepath = Project{}.filepath;
            project_.filename = Project{}.filename;
            loadProject(*project_.exampleToBeLoaded, true);
            project_.exampleToBeLoaded = nullptr;
            project_.action = Project::Action::None;
            break;
        case Project::Action::SaveAs :
            if (!fileDialog_.validSelection())
            {
                if (!fileDialog_.isOpen())
                    project_.action = Project::Action::None;
                break;
            }
            project_.filepath = fileDialog_.selection().front();
            project_.filename = Helpers::filename(project_.filepath);
            project_.forceSaveAs = false;
            saveProject(project_.filepath, false);
            fileDialog_.clearSelection();
            project_.action = Project::Action::None;
            break;
    }
    vir::Window::instance()->setTitle("ShaderThing - "+project_.filename);
}

//----------------------------------------------------------------------------//

void App::Project::renderAutoSaveMenuItemGui()
{
    if (ImGui::BeginMenu("Auto-save"))
    {
        bool canBeEnabled = filepath.size() > 0;
        if (!canBeEnabled)
            ImGui::BeginDisabled();
        ImGui::Text("Enabled  ");
        ImGui::SameLine();
        if (!canBeEnabled)
        {
            bool flag(false);
            ImGui::Checkbox("##checkAutoSave", &flag);
            if 
            (
                !canBeEnabled && 
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && 
                ImGui::BeginTooltip()
            )
            {
                ImGui::EndDisabled();
                ImGui::Text
                (
R"(The auto-save feature can be enabled only once the
project has been manually saved for the first time)"
                );
                ImGui::BeginDisabled();
                ImGui::EndTooltip();
            }
        }
        else
            ImGui::Checkbox("##checkAutoSave", &isAutoSaveEnabled);
        if (canBeEnabled && !isAutoSaveEnabled)
            ImGui::BeginDisabled();
        ImGui::Text("Interval ");
        ImGui::SameLine();
        static int index(1);
        static float intervals[5] {.5f, 1.f, 2.f, 5.f, 10.f};
        if (int(intervals[index]*60) != int(autoSaveInterval))
        {
            for (index=0; index<5; index++)
            {
                if (autoSaveInterval <= intervals[index]*60.f)
                    break;
            }
        }
        if (ImGui::SmallButton(ICON_FA_MINUS))
            index = std::max(index-1, 0);
        ImGui::SameLine();
        if (ImGui::SmallButton(ICON_FA_PLUS))
            index = std::min(index+1, 4);
        ImGui::SameLine();
        if (index == 0)
            ImGui::Text("30 sec");
        else
            ImGui::Text("%d min", int(intervals[index]));
        autoSaveInterval = intervals[index]*60.f;
        if (!canBeEnabled || !isAutoSaveEnabled)
            ImGui::EndDisabled();
        ImGui::EndMenu();
    }
}

//----------------------------------------------------------------------------//

void App::Font::initialize()
{
    // Disable reading/writing from/to imgui.ini
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.ConfigDockingTransparentPayload = true;
    
    // Load ImGui font
    static bool fontLoaded(false);
    if (!fontLoaded)
    {
        float baseFontSize = 26.f;
        fontConfig.PixelSnapH = true;
        fontConfig.OversampleV = 3.0;
        fontConfig.OversampleH = 3.0;
        fontConfig.RasterizerMultiply = 1.0;
        // The 26-36.5 ratio between Western writing systems' characters and
        // Asian logograms/characters is set so that the latter are (almost)
        // exactly twice as wide as the former, for readability, valid for the
        // selected fonts at hand
        font = io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)ByteData::Font::CousineRegularData,
            ByteData::Font::CousineRegularSize, 
            baseFontSize,
            &fontConfig,
            io.Fonts->GetGlyphRangesDefault()
        );
        fontConfig.MergeMode = true;
        fontConfig.RasterizerMultiply = 1.25;
        io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)ByteData::Font::CousineRegularData, 
            ByteData::Font::CousineRegularSize,
            baseFontSize,
            &fontConfig,
            io.Fonts->GetGlyphRangesCyrillic()
        );
        io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)ByteData::Font::CousineRegularData, 
            ByteData::Font::CousineRegularSize,
            baseFontSize,
            &fontConfig,
            io.Fonts->GetGlyphRangesGreek()
        );

        // Font icons from FontAwesome5 (free)
        float iconFontSize = baseFontSize*2.f/3.f;
        static const ImWchar iconRanges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
        ImFontConfig iconConfig; 
        iconConfig.MergeMode = true; 
        iconConfig.PixelSnapH = true; 
        iconConfig.GlyphMinAdvanceX = iconFontSize;
        io.Fonts->AddFontFromMemoryCompressedTTF
        ( 
            (void*)ByteData::Font::FontAwesome5FreeSolid900Data, 
            ByteData::Font::FontAwesome5FreeSolid900Size, 
            iconFontSize,
            &iconConfig, 
            iconRanges
        );
        io.Fonts->Build();
        fontLoaded = true;
        font->Scale = 0.6;
        fontScale = &font->Scale;
    }
}

//----------------------------------------------------------------------------//

void App::Font::checkLoadJapaneseAndOrSimplifiedChinese()
{
    auto& io = ImGui::GetIO();
    if (loadJapanese && !isJapaneseLoaded)
    {
        io.Fonts->AddFontFromMemoryCompressedTTF
        ( 
            (void*)ByteData::Font::DroidSansFallbackData, 
            ByteData::Font::DroidSansFallbackSize,
            36.5,
            &fontConfig, 
            io.Fonts->GetGlyphRangesJapanese()
        );
        io.Fonts->Build();
        vir::ImGuiRenderer::destroyDeviceObjects();
        isJapaneseLoaded = true;
    }
    if (loadSimplifiedChinese && !isSimplifiedChineseLoaded)
    {
        io.Fonts->AddFontFromMemoryCompressedTTF
        ( 
            (void*)ByteData::Font::DroidSansFallbackData, 
            ByteData::Font::DroidSansFallbackSize,
            36.5,
            &fontConfig, 
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon()
        );
        io.Fonts->Build();
        vir::ImGuiRenderer::destroyDeviceObjects();
        isSimplifiedChineseLoaded = true;
    }
}

//----------------------------------------------------------------------------//

void App::Font::renderMenuItemGui()
{
    if (ImGui::BeginMenu("Font"))
    {
        ImGui::Text("%s", "Scale");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat
        (
            "##fontScale",
            fontScale,
            0.005f,
            0.5f,
            1.0f,
            "%.1f"
        );
        ImGui::PopItemWidth();
        ImGui::Separator();
        if (ImGui::BeginMenu("Load character sets"))
        {
            bool alwaysBuiltSet(true);
            
            ImGui::Text("%s", "Latin            ");
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Checkbox("##loadLatin", &alwaysBuiltSet);
            ImGui::EndDisabled();

            ImGui::Text("%s", "Cyrillic         ");
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Checkbox("##loadCyrillic", &alwaysBuiltSet);
            ImGui::EndDisabled();

            ImGui::Text("%s", "Greek            ");
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::Checkbox("##loadGreek", &alwaysBuiltSet);
            ImGui::EndDisabled();

            ImGui::Text("%s", "Japanese         ");
            ImGui::SameLine();
            if (isJapaneseLoaded)
                ImGui::BeginDisabled();
            ImGui::Checkbox("##loadJapanese", &loadJapanese);
            if (isJapaneseLoaded)
                ImGui::EndDisabled();

            ImGui::Text("%s", "Chinese (simpl.) ");
            ImGui::SameLine();
            if (isSimplifiedChineseLoaded)
                ImGui::BeginDisabled();
            ImGui::Checkbox
            (
                "##loadSimplifiedChinese", 
                &loadSimplifiedChinese
            );
            if (isSimplifiedChineseLoaded)
                ImGui::EndDisabled();

            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

//----------------------------------------------------------------------------//

void App::renderGui()
{
    font_.checkLoadJapaneseAndOrSimplifiedChinese();
    
    vir::ImGuiRenderer::newFrame();

    // Slightly edit tab bar colors for better visibility
    bool static tabBarColorsSet = false;
    if (!tabBarColorsSet)
    {
        auto scaleColor = [](unsigned int cid, float s)
        {
            auto& style = ImGui::GetStyle();
            ImVec4& c = style.Colors[cid];
            c.x*=s;
            c.y*=s;
            c.z*=s;
            c.w*=s;
        };
        scaleColor(ImGuiCol_Tab, .8);
        scaleColor(ImGuiCol_TabActive, 1.05);
        scaleColor(ImGuiCol_TabHovered, 1.05);
        tabBarColorsSet = true;
    }
    
    ImGui::SetNextWindowSize(ImVec2(750,750), ImGuiCond_FirstUseEver);
    static ImGuiWindowFlags flags
    (
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse
    );
    ImGui::Begin(imGuiTitle_.c_str(), NULL, flags);

    // Refresh icon if needed
    static bool isIconSet(false);
    static bool isWindowDocked(ImGui::IsWindowDocked());
    if (!isIconSet || isWindowDocked != ImGui::IsWindowDocked())
    {
        isIconSet = vir::ImGuiRenderer::setWindowIcon
        (
            imGuiTitle_.c_str(), 
            ByteData::Icon::sTIconData, 
            ByteData::Icon::sTIconSize,
            false
        );
        isWindowDocked = ImGui::IsWindowDocked();
    }
    
    renderMenuBarGui();
    Layer::renderLayersTabBarGui(layers_, *sharedUniforms_, resources_);

    ImGui::End();
    
    vir::ImGuiRenderer::render();
}

//----------------------------------------------------------------------------//

void App::renderMenuBarGui()
{
    auto setProjectAction = []
    (
        const Project::Action& action, 
        Project& project, 
        FileDialog& fileDialog
    )
    {
        switch(action)
        {
            case Project::Action::New :
                project.action = Project::Action::New;
                break;
            case Project::Action::Load :
                fileDialog.runOpenFileDialog
                (
                    "Open project",
                    {"ShaderThing file (*.stf)", "*.stf *.stf.bak"},
                    ".",
                    false
                );
                project.action = Project::Action::Load;
                break;
            case Project::Action::LoadExample :
                project.action = Project::Action::LoadExample;
                break;
            case Project::Action::Save :
                if (project.filepath.size() == 0 || project.forceSaveAs)
                {
                    fileDialog.runSaveFileDialog
                    (
                        "Save project",
                        {"ShaderThing file (*.stf)", "*.stf"},
                        project.filepath.size() == 0 ? 
                        project.filename.c_str() :
                        project.filepath.c_str()
                    );
                    project.action = Project::Action::SaveAs;
                }
                else
                    project.action = Project::Action::Save;
                break;
            case Project::Action::SaveAs :
                fileDialog.runSaveFileDialog
                (
                    "Save project",
                    {"ShaderThing file (*.stf)", "*.stf"},
                    project.filepath.size() == 0 ? 
                    project.filename.c_str() :
                    project.filepath.c_str()
                );
                project.action = Project::Action::SaveAs;
                break;
            default :
                return;
        }
    };

    if (Helpers::isCtrlKeyPressed(ImGuiKey_R)) // Ctrl+R
    {
        sharedUniforms_->resetFrameCounter();
        Layer::Flags::restartRendering = true;
    }

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
                setProjectAction(Project::Action::Load, project_, fileDialog_);
            if (ImGui::MenuItem("Save", "Ctrl+S"))
                setProjectAction(Project::Action::Save, project_, fileDialog_);
            if (ImGui::MenuItem("Save as", "Ctrl+Shift+S"))
                setProjectAction(Project::Action::SaveAs,project_,fileDialog_);
            ImGui::Separator();
            if (ImGui::BeginMenu("Export"))
            {
                exporter_->renderGui(*sharedUniforms_, layers_);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Properties"))
        {
            if (ImGui::BeginMenu("Window", !vir::Window::instance()->iconified()))
            {
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
R"(Frame rate multiplier for the graphical-user-interface. By default, the GUI
frame rate is tied to the shader rendering frame rate in the main window. When
rendering computationally intensive shaders, the GUI frame rate is affected as 
well, resulting in a worsened user experience. Set this multiplier to values
larger than one to recover the GUI frame rate, at the expense, however, of a
further reduction of the shader rendering frame rate)");
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                ImGui::PushItemWidth(8.f*ImGui::GetFontSize());
                int nRenderingTiles = Layer::Rendering::TileController::nTiles;
                if (sharedUniforms_->isRenderingPaused())
                    ImGui::BeginDisabled();
                if (ImGui::InputInt("##nRenderingTiles", &nRenderingTiles))
                {
                    nRenderingTiles = std::max(nRenderingTiles, 1);
                    Layer::setRenderingTiles(layers_, nRenderingTiles);
                }
                if (sharedUniforms_->isRenderingPaused())
                    ImGui::EndDisabled();
                
                ImGui::Text("Pause render below ");
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                {
                    ImGui::Text(
R"(Frame rate (in frames per second, fps) of the graphical user interface (GUI)
below which shader rendering is paused, to prevent e.g., making the app 
unresponsive should the shader(s) be accidentally made too computationally 
intensive. This feature is disabled during project exports)");
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
                        !sharedUniforms_->isRenderingPaused() ? 
                        "Pause rendering" : "Resume rendering", 
                        ImVec2(-1, 0)
                    )
                )
                    sharedUniforms_->toggleRenderingPaused();

                if (ImGui::Button("Capture mouse cursor", ImVec2(-1, 0)))
                    sharedUniforms_->setMouseCaptured(true);

                ImGui::EndMenu();
            }

            for (auto layer : layers_)
                layer->renderPropertiesMenuGui(resources_);
            ImGui::Separator();
            Layer::renderShaderLanguangeExtensionsMenuGui
            (
                layers_, 
                *sharedUniforms_
            );
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Resources"))
        {
            Resource::renderResourcesMenuItemGui(resources_, layers_);
            shadersRequireRecompilation = 
                Layer::Rendering::sharedStorage->renderMenuItemGui();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Find"))
        {
            TextEditor::renderFindReplaceToolMenuGui();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Preferences"))
        {
            font_.renderMenuItemGui();
            project_.renderAutoSaveMenuItemGui();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
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
                    vir::Renderer::instance()->
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
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    if (Resource::isGuiDetachedFromMenu)
        Resource::renderResourcesGui(resources_, layers_);
    if (Layer::Rendering::sharedStorage->isGuiDetachedFromMenu())
        shadersRequireRecompilation = 
            Layer::Rendering::sharedStorage->renderGui();
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
            setProjectAction(Project::Action::New, project_, fileDialog_);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------//

}