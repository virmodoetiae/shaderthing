#include "vir/include/vir.h"
#include "shaderthing-p/include/app.h"
#include "shaderthing-p/include/bytedata.h"
#include "shaderthing-p/include/layer.h"
#include "shaderthing-p/include/resource.h"
#include "shaderthing-p/include/shareduniforms.h"
#include "shaderthing-p/include/coderepository.h"
#include "shaderthing-p/include/filedialog.h"

#include <charconv>

namespace ShaderThing
{

App::App()
{
    // Initialize vir lib
    vir::initialize
    (
        vir::PlatformType::GLFWOpenGL,
        512,
        512,
        "ShaderThing"
    );
    sharedUniforms_ = new SharedUniforms();
    layers_.emplace_back(new Layer(layers_, *sharedUniforms_));
    initializeGUI();

    auto fileDialog = FileDialog();
    fileDialog.open("png,jpg,jpeg");

    // Main loop
    auto window = vir::GlobalPtr<vir::Window>::instance();
    while(window->isOpen())
    {   
        if (fileDialog.validSelection())
            std::cout << fileDialog.filepath() << std::endl;
        renderGUI();
        Layer::renderShaders(layers_, nullptr, *sharedUniforms_);
        update();
        window->update(!sharedUniforms_->isRenderingPaused());
    }
}

//----------------------------------------------------------------------------//

App::~App()
{
    DELETE_IF_NOT_NULLPTR(sharedUniforms_)
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
    
    sharedUniforms_->update({timeStep});

    for (auto resource : resources_)
        resource->update({sharedUniforms_->iTime(), timeStep});

    // Compute FPS and set in window title
    static float fps(60.0f);
    static int elapsedFrames(0);
    static float elapsedTime(0);
    elapsedFrames++;
    elapsedTime += window->time()->outerTimestep();
    if (elapsedFrames >= int(fps/2.0f)) // Update every ~1/2 second
    {
        fps = elapsedFrames/elapsedTime;
        static char bfps[8];
        *(std::to_chars(bfps,bfps+8,fps,std::chars_format::fixed,1)).ptr = '\0';
        std::string sfps(bfps);
        window->setTitle("ShaderThing ("+sfps+" FPS)");
        elapsedFrames = 0;
        elapsedTime = 0;
    }
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
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N"))
            {
            }
            if (ImGui::MenuItem("Load", "Ctrl+O"))
            {
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
            }
            if (ImGui::MenuItem("Save as", "Ctrl+Shift+S"))
            {
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export"))
            {
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
}

//----------------------------------------------------------------------------//

}