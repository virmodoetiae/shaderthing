/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2024 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include <charconv>

#include "shaderthing-p/include/app.h"

#include "shaderthing-p/include/bytedata.h"
#include "shaderthing-p/include/coderepository.h"
#include "shaderthing-p/include/exporter.h"
#include "shaderthing-p/include/helpers.h"
#include "shaderthing-p/include/layer.h"
#include "shaderthing-p/include/objectio.h"
#include "shaderthing-p/include/resource.h"
#include "shaderthing-p/include/shareduniforms.h"

#include "vir/include/vir.h"

namespace ShaderThing
{

App::App()
{
    // Initialize vir lib
    vir::Settings settings = {};
    settings.windowName = "ShaderThing";
    settings.enableFaceCulling = false;
    vir::initialize(settings);
    
    // Setup ImGui
    initializeGUI();

    newProject();

    // Main loop
    auto window = vir::GlobalPtr<vir::Window>::instance();
    while(window->isOpen())
    {   
        renderGUI();
        Layer::renderShaders
        (
            layers_, 
            exporter_->isRunning() ? exporter_->framebuffer() : nullptr, 
            *sharedUniforms_,
            exporter_->isRunning() ? exporter_->nRenderPasses() : 1
        );
        update();
        window->update(!sharedUniforms_->isRenderingPaused());
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
    static auto* window(vir::GlobalPtr<vir::Window>::instance());
    float timeStep = window->time()->outerTimestep();
    
    // If exporting, this update will set timeStep to the requested export
    // timeStep (i.e., inverse of export fps if exporting an animation (gif or
    // video frames), left unmodified otherwise)
    exporter_->      update(*sharedUniforms_, layers_,             {timeStep});
    sharedUniforms_->update(                                       {timeStep});
    Resource::       update( resources_, {sharedUniforms_->iTime(), timeStep});

    // Compute FPS and set in window title
    static float fps(60.0f);
    static int elapsedFrames(0);
    static float elapsedTime(0);
    elapsedFrames++;
    elapsedTime += window->time()->outerTimestep();
    if (elapsedFrames >= int(fps/2.0f)) // Update title every ~1/2 second
    {
        fps = elapsedFrames/elapsedTime;
        //static char bfps[8];
        //*(std::to_chars(bfps,bfps+8,fps,std::chars_format::fixed,1)).ptr='\0';
        //std::string sfps(bfps);
        window->setTitle("ShaderThing ("+Helpers::format(fps, 1)+" FPS)");
        elapsedFrames = 0;
        elapsedTime = 0;
    }

    // Process project actions
    switch (project_.action)
    {
        case Project::Action::None :
            return;
        case Project::Action::New :
            newProject();
            project_.action = Project::Action::None;
            break;
        case Project::Action::Save :
            saveProject(project_.filepath);
            project_.action = Project::Action::None;
            break;
        case Project::Action::Load :
            if (!fileDialog_.validSelection())
            {
                if (!fileDialog_.isOpen())
                    project_.action = Project::Action::None;
                break;
            }
            project_.filepath = fileDialog_.selection().front();
            project_.forceSaveAs = true;
            loadProject(project_.filepath);
            fileDialog_.clearSelection();
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
            project_.forceSaveAs = false;
            saveProject(project_.filepath);
            fileDialog_.clearSelection();
            project_.action = Project::Action::None;
            break;
    }

}

//----------------------------------------------------------------------------//

void App::saveProject(const std::string& filepath) const
{
    auto project = ObjectIO(filepath.c_str(), ObjectIO::Mode::Write);
    project.write("UIScale", *gui_.fontScale);
    
    sharedUniforms_->save(            project);
    Layer::          save(layers_,    project);
    Resource::       save(resources_, project);
}

//----------------------------------------------------------------------------//
    
void App::loadProject(const std::string& filepath)
{
    auto project = ObjectIO(filepath.c_str(), ObjectIO::Mode::Read);
    *gui_.fontScale = project.read<float>("UIScale");
    SharedUniforms::load(   project,           sharedUniforms_);
    Resource::      loadAll(project,                            resources_);
    Layer::         loadAll(project, layers_, *sharedUniforms_, resources_);
}

//----------------------------------------------------------------------------//

void App::newProject()
{
    project_ = {};
    *gui_.fontScale = .6;

    DELETE_IF_NOT_NULLPTR(exporter_);
    exporter_ = new Exporter();
    
    DELETE_IF_NOT_NULLPTR(sharedUniforms_);
    sharedUniforms_ = new SharedUniforms();
    
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
    layers_.emplace_back(new Layer(layers_, *sharedUniforms_));
}

//----------------------------------------------------------------------------//

void App::initializeGUI()
{
    // Disable reading/writing from/to imgui.ini
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.ConfigDockingTransparentPayload = true;
    
    // Load ImGui font
    static bool fontLoaded(false);
    static ImFont* font = nullptr;
    static ImFontConfig config; 
    if (!fontLoaded)
    {
        float baseFontSize = 26.f;
        config.PixelSnapH = true;
        config.OversampleV = 3.0;
        config.OversampleH = 3.0;
        config.RasterizerMultiply = 1.0;
        // The 26-36.5 ratio between Western writing systems' characters and
        // asian logograms/characters is set so that the latter are (almost)
        // exactly twice as wide as the former, for readability, valid for the
        // selected fonts at hand
        font = io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)ByteData::Font::CousineRegularData,
            ByteData::Font::CousineRegularSize, 
            baseFontSize,
            &config,
            io.Fonts->GetGlyphRangesDefault()
        );
        config.MergeMode = true;
        config.RasterizerMultiply = 1.25;
        io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)ByteData::Font::CousineRegularData, 
            ByteData::Font::CousineRegularSize,
            baseFontSize,
            &config,
            io.Fonts->GetGlyphRangesCyrillic()
        );
        io.Fonts->AddFontFromMemoryCompressedTTF
        (
            (void*)ByteData::Font::CousineRegularData, 
            ByteData::Font::CousineRegularSize,
            baseFontSize,
            &config,
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
        gui_.fontScale = &font->Scale;
    }
}

//----------------------------------------------------------------------------//

void App::renderGUI()
{
    vir::ImGuiRenderer::newFrame();
    ImGui::SetNextWindowSize(ImVec2(750,900), ImGuiCond_FirstUseEver);
    static ImGuiWindowFlags flags
    (
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse
    );
    ImGui::Begin("Control panel", NULL, flags);
    
    renderMenuBarGUI();
    Layer::renderLayersTabBarGUI(layers_, *sharedUniforms_, resources_);

    ImGui::End();
    vir::ImGuiRenderer::render();
}

//----------------------------------------------------------------------------//

void App::renderMenuBarGUI()
{
    float fontSize = ImGui::GetFontSize();

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
                    {"ShaderThing file (*.stf)", "*.stf"},
                    ".",
                    false
                );
                project.action = Project::Action::Load;
                break;
            case Project::Action::Save :
                if (project.filepath.size() == 0 || project.forceSaveAs)
                {
                    fileDialog.runSaveFileDialog
                    (
                        "Save project",
                        {"ShaderThing file (*.stf)", "*.stf"},
                        project.filepath.size() == 0 ? 
                        "new_project.stf" :
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
                    project.filepath
                );
                project.action = Project::Action::SaveAs;
                break;
            default :
                return;
        }
    };

    bool newProjectConfirmation = false;
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N"))
                newProjectConfirmation = true;
            if (ImGui::MenuItem("Load", "Ctrl+O"))
                setProjectAction(Project::Action::Load, project_, fileDialog_);
            if (ImGui::MenuItem("Save", "Ctrl+S"))
                setProjectAction(Project::Action::Save, project_, fileDialog_);
            if (ImGui::MenuItem("Save as", "Ctrl+Shift+S"))
                setProjectAction(Project::Action::SaveAs,project_,fileDialog_);
            ImGui::Separator();
            if (ImGui::BeginMenu("Export"))
            {
                exporter_->renderGUI(*sharedUniforms_, layers_);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Settings"))
        {
            sharedUniforms_->renderWindowResolutionMenuGUI();
            for (auto layer : layers_)
                layer->renderSettingsMenuGUI(resources_);
            ImGui::Separator();
            if (ImGui::BeginMenu("Font"))
            {
                ImGui::Text("Scale");
                ImGui::SameLine();
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat
                (
                    "##fontScale",
                    gui_.fontScale,
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
                    
                    ImGui::Text("Latin    ");
                    ImGui::SameLine();
                    ImGui::BeginDisabled();
                    ImGui::Checkbox("##loadLatin", &alwaysBuiltSet);
                    ImGui::EndDisabled();

                    ImGui::Text("Cyrillic ");
                    ImGui::SameLine();
                    ImGui::BeginDisabled();
                    ImGui::Checkbox("##loadCyrillic", &alwaysBuiltSet);
                    ImGui::EndDisabled();

                    ImGui::Text("Greek    ");
                    ImGui::SameLine();
                    ImGui::BeginDisabled();
                    ImGui::Checkbox("##loadGreek", &alwaysBuiltSet);
                    ImGui::EndDisabled();

                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            /*
            if (ImGui::BeginMenu("Misc"))
            {
                ImGui::Text("Time reset on rendering restart");
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                {
                    ImGui::Text(
R"(If true, the iTime uniform shared by all layers will be reset to 0 every time
the rendering is restarted, i.e., via: 1) shader recompilation (Ctrl+B) or 2) 
resetting the iFrame uniform (Ctrl+R))"
                    );
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                ImGui::Checkbox
                (
                    "##timeResetOnRenderingRestart",
                    &app->sharedUniforms.flags.isTimeResetOnRenderingRestart
                );
                ImGui::EndMenu();
            }
            */
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Resources"))
        {
            Resource::renderResourcesMenuItemGUI(resources_, layers_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Find"))
        {
            TextEditor::renderFindReplaceToolMenuGUI();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::BeginMenu("Code repository"))
            {
                CodeRepository::renderGUI();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("About ShaderThing"))
            {
                //
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("System info"))
            {
                ImGui::Text("Graphics card in use:");
                ImGui::SameLine();
                ImGui::Text
                (
                    vir::GlobalPtr<vir::Renderer>::instance()->
                    deviceName().c_str()
                );
                ImGui::Text("Graphics context:    ");
                ImGui::SameLine();
                ImGui::Text
                (
                    vir::GlobalPtr<vir::Window>::instance()->
                    context()->name().c_str()
                );
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    if (Resource::isGuiDetachedFromMenu)
        Resource::renderResourcesGUI(resources_, layers_);
    
    if (project_.action == Project::Action::None)
    {
        if (Helpers::isCtrlKeyPressed(ImGuiKey_N))
            newProjectConfirmation = true;
        else if (Helpers::isCtrlKeyPressed(ImGuiKey_O))
            setProjectAction(Project::Action::Load, project_, fileDialog_);
        else if (Helpers::isCtrlKeyPressed(ImGuiKey_S))
            setProjectAction(Project::Action::Save, project_, fileDialog_);
        else if (Helpers::isCtrlShiftKeyPressed(ImGuiKey_S))
            setProjectAction(Project::Action::SaveAs, project_, fileDialog_);
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