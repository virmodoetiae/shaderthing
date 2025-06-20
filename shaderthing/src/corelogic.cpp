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

void initializeSharedUniforms(AppData& appData)
{
    SharedUniforms& su = appData.sharedUniforms;

    // Init CPU block data
    static const auto window = vir::Window::instance();
    if (!window->iconified())
       su.iResolution = {window->width(), window->height()};
    su.iAspectRatio = su.iResolution.x/su.iResolution.y;
    for (int i=0; i<256; i++)
        su.iKeyboard[i] = glm::ivec3({0,0,0});

    // Init cameras
    su.screenCamera = vir::makeUnique<vir::Camera>();
    su.shaderCamera = vir::makeUnique<vir::InputCamera>();
    su.screenCamera->setProjectionType
    (
        vir::Camera::ProjectionType::Orthographic
    );
    su.screenCamera->setViewportHeight
    (
        std::min(1.0f, 1.0f/su.iAspectRatio)
    );
    su.screenCamera->setPosition({0, 0, 1});
    su.screenCamera->setPlanes(.01f, 100.f);
    su.shaderCamera->setZPlusIsLookDirection(true);
    su.shaderCamera->setDirection(su.iLook);
    su.shaderCamera->setPosition(su.iWASD);
    su.screenCamera->update();
    su.shaderCamera->update();
    //iMVP_ = screenCamera_->projectionViewMatrix();

    // Init random number generator and set initial random number
    //if (random_ == nullptr)
    //    random_ = new Random();
    //iRandom_ = random_->generateFloat();

    // Init uniform buffers, bind to designated binding points and set
    // initial data
    su.vBuffer = 
            vir::DynamicUniformBuffer::create(64, "vertexSharedUniformBlock");
    su.vBuffer->bind();
    su.vBuffer->setBindingPoint(su.vBufferBindingPoint);

    su.iMVPUniform = vir::makeUnique<Uniform>();
    su.iMVPUniform->name = "iMVP";
    su.iMVPUniform->setValuePtr
    (
        &(su.screenCamera->projectionViewMatrix()), 
        Uniform::Type::Mat4
    );
    su.iMVPUniform->gui.showBounds = false;
    su.vBuffer->addUniform(su.iMVPUniform);

    su.fBuffer = 
            vir::DynamicUniformBuffer::create(8196, "sharedUniformBlock");
    su.fBuffer->bind();
    su.fBuffer->setBindingPoint(su.fBufferBindingPoint);

    // Init uniform wrappers
    su.iFrameUniform = vir::makeUnique<Uniform>();
    su.iFrameUniform->name = "iFrame";
    su.iFrameUniform->setValuePtr(&appData.renderer.frame, Uniform::Type::Int);
    su.iFrameUniform->gui.showBounds = false;
    //su.iFrameUniform->specialType = Uniform::SpecialType::Frame;
    su.fBuffer->addUniform(su.iFrameUniform);

    su.iRenderPassUniform = vir::makeUnique<Uniform>();
    su.iRenderPassUniform->name = "iRenderPass";
    su.iRenderPassUniform->setValuePtr
    (
        &appData.renderer.renderPass, 
        Uniform::Type::Int
    );
    su.iRenderPassUniform->gui.showBounds = false;
    //su.iRenderPassUniform->specialType = Uniform::SpecialType::RenderPass;
    su.fBuffer->addUniform(su.iRenderPassUniform);
    
    su.iTimeUniform = vir::makeUnique<Uniform>();
    su.iTimeUniform->name = "iTime";
    su.iTimeUniform->setValuePtr(&su.iTime, Uniform::Type::Float);
    //su.iTimeUniform->specialType = Uniform::SpecialType::Time;
    su.fBuffer->addUniform(su.iTimeUniform);
    
    su.iTimeDeltaUniform = vir::makeUnique<Uniform>();
    su.iTimeDeltaUniform->name = "iTimeDelta";
    su.iTimeDeltaUniform->setValuePtr(&su.iTimeDelta, Uniform::Type::Float);
    su.iTimeDeltaUniform->gui.showBounds = false;
    su.fBuffer->addUniform(su.iTimeDeltaUniform);

    su.iRandomUniform = vir::makeUnique<Uniform>();
    su.iRandomUniform->name = "iRandom";
    su.iRandomUniform->setValuePtr(&su.iRandom, Uniform::Type::Float);
    su.iRandomUniform->gui.showBounds = false;
    su.fBuffer->addUniform(su.iRandomUniform);

    su.iUserActionUniform = vir::makeUnique<Uniform>();
    su.iUserActionUniform->name = "iUserAction";
    su.iUserActionUniform->setValuePtr(&su.iUserAction, Uniform::Type::Bool);
    su.iUserActionUniform->gui.showBounds = false;
    //su.iUserActionUniform->specialType = Uniform::SpecialType::UserAction;
    su.fBuffer->addUniform(su.iUserActionUniform);

    su.iExportUniform = vir::makeUnique<Uniform>();
    su.iExportUniform->name = "iExport";
    su.iExportUniform->setValuePtr
    (
        &appData.exporter.isActive, 
        Uniform::Type::Bool
    );
    su.iExportUniform->gui.showBounds = false;
    su.fBuffer->addUniform(su.iExportUniform);

    su.iWASDUniform = vir::makeUnique<Uniform>();
    su.iWASDUniform->name = "iWASD";
    su.iWASDUniform->setValuePtr(&su.iWASD, Uniform::Type::Float3);
    //su.iWASDUniform->specialType = Uniform::SpecialType::CameraPosition;
    su.fBuffer->addUniform(su.iWASDUniform);

    su.iLookUniform = vir::makeUnique<Uniform>();
    su.iLookUniform->name = "iLook";
    su.iLookUniform->setValuePtr(&su.iLook, Uniform::Type::Float3);
    su.iLookUniform->gui.showBounds = false;
    //su.iLookUniform->specialType = Uniform::SpecialType::CameraDirection;
    su.fBuffer->addUniform(su.iLookUniform);

    su.iMouseUniform = vir::makeUnique<Uniform>();
    su.iMouseUniform->name = "iMouse";
    su.iMouseUniform->setValuePtr(&su.iMouse, Uniform::Type::Float4);
    su.iMouseUniform->gui.showBounds = false;
    //su.iMouseUniform->specialType = Uniform::SpecialType::Mouse;
    su.fBuffer->addUniform(su.iMouseUniform);

    su.iAspectRatioUniform = vir::makeUnique<Uniform>();
    su.iAspectRatioUniform->name = "iWindowAspectRatio";
    su.iAspectRatioUniform->setValuePtr(&su.iAspectRatio, Uniform::Type::Float);
    su.iAspectRatioUniform->gui.showBounds = false;
    //su.iAspectRatioUniform->specialType = Uniform::SpecialType::WindowAspectRatio;
    su.fBuffer->addUniform(su.iAspectRatioUniform);

    su.iResolutionUniform = vir::makeUnique<Uniform>();
    su.iResolutionUniform->name = "iWindowResolution";
    su.iResolutionUniform->setValuePtr(&su.iResolution, Uniform::Type::Float2);
    su.iResolutionUniform->gui.showBounds = false;
    //su.iResolutionUniform->specialType = Uniform::SpecialType::WindowResolution;
    su.fBuffer->addUniform(su.iResolutionUniform);

    su.iKeyboardUniform = vir::makeUnique<Uniform>();
    su.iKeyboardUniform->name = "iKeyboard";
    su.iKeyboardUniform->setValuePtr(&su.iKeyboard, Uniform::Type::Int3, 256);
    su.iKeyboardUniform->gui.showBounds = false;
    //su.iKeyboardUniform->specialType = Uniform::SpecialType::Keyboard;
    su.fBuffer->addUniform(su.iKeyboardUniform);
}

void renderShaders(AppData& appData)
{

}

void setupNewProject(AppData& appData)
{
    appData.layers.clear();
    createNewLayer(appData);
    initializeSharedUniforms(appData);
}

void setLayerDepth(Layer& layer, const float depth)
{
    layer.depth = depth;
    // And more!
}

}