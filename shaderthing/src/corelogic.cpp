#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/corelogic.h"
#include "shaderthing/include/helpers.h"
#include "shaderthing/include/structs.h"
#include "shaderthing/include/oo/texteditor.h"
#include "vir/include/vir.h"

namespace ShaderThing
{

void createNewLayer(AppData& appData)
{
    unsigned int id = Helpers::findSmallestFreeLayerId(appData.layers);
    auto& layer = *appData.layers.emplace_back(vir::makeUnique<Layer>(id));
    layer.name = "Layer "+std::to_string(id);
    layer.sourceEditor = vir::makeUnique<TextEditor>();
}

//----------------------------------------------------------------------------//

void initialize(AppData& appData)
{
    // Initialize vir lib
    vir::Settings settings = {};
    settings.windowName = "ShaderThing";// - "+project_.filename;
    settings.enableFaceCulling = false;
    vir::initialize(settings);
    
    // ImGui setup
    ImGuiIO& io = ImGui::GetIO();
    // Do not save config to .ini file
    io.IniFilename = NULL;
    io.ConfigDockingTransparentPayload = true;
    // Custom tab bar color styling
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
    
    // Font setup
    auto& font = appData.font;
    float baseFontSize = 26.f;
    font.imFontConfig.PixelSnapH = true;
    font.imFontConfig.OversampleV = 3.0;
    font.imFontConfig.OversampleH = 3.0;
    font.imFontConfig.RasterizerMultiply = 1.0;
    // The 26-36.5 ratio between Western writing systems' characters and
    // Asian logograms/characters is set so that the latter are (almost)
    // exactly twice as wide as the former, for readability, valid for the
    // selected fonts at hand
    font.imFont = io.Fonts->AddFontFromMemoryCompressedTTF
    (
        (void*)ByteData::Font::CousineRegularData,
        ByteData::Font::CousineRegularSize, 
        baseFontSize,
        &font.imFontConfig,
        io.Fonts->GetGlyphRangesDefault()
    );
    font.imFontConfig.MergeMode = true;
    font.imFontConfig.RasterizerMultiply = 1.25;
    io.Fonts->AddFontFromMemoryCompressedTTF
    (
        (void*)ByteData::Font::CousineRegularData, 
        ByteData::Font::CousineRegularSize,
        baseFontSize,
        &font.imFontConfig,
        io.Fonts->GetGlyphRangesCyrillic()
    );
    io.Fonts->AddFontFromMemoryCompressedTTF
    (
        (void*)ByteData::Font::CousineRegularData, 
        ByteData::Font::CousineRegularSize,
        baseFontSize,
        &font.imFontConfig,
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
    font.imFont->Scale = 0.6;
    font.scale = &font.imFont->Scale;

    // Set window icon
    auto window = vir::Window::instance();
    window->setIcon
    (
        (unsigned char*)ByteData::Icon::sTIconData,
        ByteData::Icon::sTIconSize,
        false
    );

    // Initialize shared source editor
    appData.sharedSourceEditor = vir::makeUnique<TextEditor>();

    // Create new project
    setupNewProject(appData);
};

void renderShaders(AppData& appData)
{

}

void setupNewProject(AppData& appData)
{
    appData.layers.clear();
    createNewLayer(appData);
}

void setLayerDepth(Layer& layer, const float depth)
{
    layer.depth = depth;
    // And more!
}

}