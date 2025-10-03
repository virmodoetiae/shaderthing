#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/corelogic.h"
#include "shaderthing/include/helpers.h"
#include "shaderthing/include/structs.h"
#include "shaderthing/include/oo/texteditor.h"
#include "shaderthing/include/oo/statusbar.h"
#include "vir/include/vir.h"

namespace ShaderThing
{

UPtr<vir::Shader> Layer::Renderer::textureMapperShader;

//----------------------------------------------------------------------------//

void initialize(AppData& appData)
{
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

    initializeSharedUniforms(appData);

    // Initialize shared texture mapper shader
    std::string vertexSource = assembleVertexShaderSource(appData);
    std::string fragmentSource =
        vir::Shader::currentContextShadingLanguageDirectives()+
R"(out  vec4      fragColor;
in      vec2      qc;
in      vec2      tc;
uniform sampler2D tx;
void main(){fragColor = texture(tx, tc);})";
    Layer::Renderer::textureMapperShader =
        vir::Shader::create
        (
            vertexSource,
            fragmentSource,
            vir::Shader::ConstructFrom::SourceCode
        );
    Layer::Renderer::textureMapperShader->bind();
    Layer::Renderer::textureMapperShader->bindUniformBlock
    (
        appData.sharedUniforms.vBuffer->name(),
        appData.sharedUniforms.vBufferBindingPoint
    );
    Layer::Renderer::textureMapperShader->setUniformInt("tx", 0);

    // Create default layer
    createNewLayer(appData);
};

//----------------------------------------------------------------------------//

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

//----------------------------------------------------------------------------//

void setupNewProject(AppData& appData)
{
    appData.layers.clear();
    initializeSharedUniforms(appData);
    createNewLayer(appData);
}

//----------------------------------------------------------------------------//

void preRenderUpdate(AppData& appData)
{
    /*
    appData.deferredActionBuffer.process();

    // exporter_->update(*sharedUniforms_, layers_, resources_);

    bool advanceFrame;
    float timeStep;
    if (appData.exporter.isActive)
    {
        if 
        (
            appData.renderer.renderPass == 
            appData.exporter.settings.nRenderPasses-1
        )
        {
            advanceFrame = true;
            timeStep = appData.exporter.timeStep;
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
    */
}

//----------------------------------------------------------------------------//

void setWindowResolution
(
    AppData& appData, 
    glm::ivec2 resolution, 
    const bool windowFrameManuallyDragged,
    const bool prepareForExport
)
{
    // Limit resolution if not about to export
    static const auto window = vir::Window::instance();
    if (!prepareForExport)
    {
        auto monitorScale = window->contentScale();
        glm::ivec2 minResolution = {120*monitorScale.x, 1};
        glm::ivec2 maxResolution = window->primaryMonitorResolution();
        resolution.x = 
            std::max(std::min(resolution.x, maxResolution.x), minResolution.x);
        resolution.y = 
            std::max(std::min(resolution.y, maxResolution.y), minResolution.y);
    }

    auto& su = appData.sharedUniforms;

    // Store in iResolution & update aspectRatio
    su.iResolution = resolution;
    su.iAspectRatio = ((float)resolution.x)/resolution.y;

    // If not preparing for export, reset export resolution and its scale if
    // the window is resized in any way (either manullay or via the GUI). Not
    // necessary but I like this behavior better
    if (!prepareForExport)
    {
        appData.exporter.settings.outputResolution = resolution;
        appData.exporter.settings.outputResolutionScale = 1.f;
    }

    // Update screen camera
    su.screenCamera->setViewportHeight
    (
        std::min(1.0f, 1.0f/su.iAspectRatio)
    );
    auto iMVP = su.screenCamera->projectionViewMatrix();
    su.screenCamera->update();

    //iMVP_ = screenCamera_->projectionViewMatrix();
    su.vBuffer->markUniformForSubmission(su.iMVPUniform.get());
    su.fBuffer->markUniformForSubmission(su.iAspectRatioUniform.get());
    su.fBuffer->markUniformForSubmission(su.iResolutionUniform.get());
    
    // Set the actual window resolution and propagate event if not preparing
    // for export
    if (!prepareForExport && !windowFrameManuallyDragged)
        window->setSize
        (
            resolution.x,
            resolution.y
        );
}

//----------------------------------------------------------------------------//

RenderResult renderShaders(AppData& appData)
{
    return {true, true};
}

//----------------------------------------------------------------------------//

void postRenderUpdate(AppData& appData)
{
    auto& su = appData.sharedUniforms;
    
    // TODO
    //exporter_->update(*sharedUniforms_, layers_, resources_);

    bool advanceFrame;
    float timeStep;
    if (false)//(exporter_->isRunning())
    {
        if 
        (
            appData.renderer.renderPass == 
            appData.exporter.settings.nRenderPasses-1
        )
        {
            advanceFrame = true;
            timeStep = appData.exporter.timeStep;
        }
        else
        {
            advanceFrame = false;
            timeStep = 0;
        }
    }
    else
    {
        timeStep = (su.flags.isTimeDeltaSmooth ?
            vir::Window::instance()->time()->smoothOuterTimestep() : 
            vir::Window::instance()->time()->outerTimestep());
        if (appData.renderer.isTiledRenderingEnabled)
        {
            static float cumulatedTimeStep = 0;
            if (!appData.renderer.isPaused)
                cumulatedTimeStep += timeStep;
            if (appData.renderer.tileIndex == 0)
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

    if (!su.flags.isTimePaused)
    {
        su.iTime += timeStep;
        if (advanceFrame)
            su.iTimeDelta = timeStep;
    }
    else if 
    (
        advanceFrame && 
        (su.flags.stepToNextFrame || su.flags.stepToNextTimeStep)
    )
        su.iTime += su.iTimeDelta;

    const glm::vec2& timeLoopBounds(su.iTimeUniform->gui.bounds);
    if (su.flags.isTimeLooped && su.iTime >= timeLoopBounds.y)
    {
        auto duration = timeLoopBounds.y-timeLoopBounds.x;
        auto fraction = 
            (su.iTime-timeLoopBounds.y)/std::max(duration, 1e-6f);
        fraction -= (int)fraction;
        su.iTime = timeLoopBounds.x + duration*fraction;
    }
    
    if 
    (
        advanceFrame && 
        !(appData.renderer.isPaused && !su.flags.stepToNextFrame)
    )
        ++appData.renderer.frame;

    if (su.flags.resetFrameCounterPreOrPostExport)
    {
        appData.renderer.frame = 0;
        su.flags.resetFrameCounterPreOrPostExport = false;
    }
    if (su.flags.resetFrameCounter)
    {
        appData.renderer.frame = 0;
        if (su.flags.isTimeResetOnFrameCounterReset)
            su.iTime = 0;
        su.flags.resetFrameCounter = false;
    }

    // The shaderCamera has its own event listeners, but all of its updates are
    // deferred (just like here nothing is processed/sent to the GPU in the
    // event callback), so we update it here and check whether the GPU data
    // should be updated as well
    su.shaderCamera->update();

    // Re-gen random number
    if (!su.flags.isRandomNumberGeneratorPaused)
        su.iRandom = 0.5; // TODO random_->generateFloat();

    if 
    (
        su.iWASD != su.shaderCamera->position() ||
        su.iLook != su.shaderCamera->z()
    )
    {
        su.iWASD = su.shaderCamera->position();
        su.iLook = su.shaderCamera->z();
        su.iUserAction = true;
        su.flags.updateDataRangeII = true;
    }
    
    // Data range I is always updated, data range III is updated on the spot
    // in setResolution, the keyboard data range is updated on the spot in
    // onReceive(KeyPressEvent/KeyReleaseEvent)
    su.fBuffer->markContiguousUniformsForSubmission
    (
        su.iFrameUniform.get(), 
        su.iRandomUniform.get()
    );
    /*
    if (!flags_.updateDataRangeII)
        fBuffer_->setData(&fBlock_, FragmentBlock::dataRangeISize(), 0);
    else
    {
        fBuffer_->setData(&fBlock_, FragmentBlock::dataRangeIISize(), 0);
        flags_.updateDataRangeII = false;
    }*/
    if (su.flags.updateDataRangeII)
    {
        su.fBuffer->markContiguousUniformsForSubmission
        (
            su.iUserActionUniform.get(), 
            su.iMouseUniform.get()
        );
        su.flags.updateDataRangeII = false;
    }

    su.vBuffer->submitUniforms();
    su.fBuffer->submitUniforms();

    if (su.iUserAction) // Always reset
    {
        su.flags.updateDataRangeII = true;
        su.iUserAction = false;
    }

    // TODO
    //Resource::       update( resources_, {sharedUniforms_->iTime(), timeStep});
    
    // Auto-save if applicable 
    /* TODO
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
    */

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
        if (appData.renderer.isTiledRenderingEnabled)
        {
            double wFps = fps/appData.renderer.nTiles;
            appData.controlPanelTitle = 
                "Control panel - "+appData.project.filename+" (window: "+
                Helpers::format(wFps,1)+" fps | GUI: "+
                Helpers::format(fps,1)+" fps)"+"###CP";
        }
        else
            appData.controlPanelTitle = 
                "Control panel - "+appData.project.filename+" ("+
                Helpers::format(fps,1)+" fps)"+"###CP";
        elapsedFrames = 0;
        elapsedTime = 0;
        if (!appData.exporter.isActive)
        {
            fpsUpdateCounter++;
            shouldStopRendering = 
                shouldStopRendering && fps < appData.renderer.lowerFpsLimit;
            if (fpsUpdateCounter >= int(maxLowFpsPeriod/fpsUpdatePeriod))
            {
                if (shouldStopRendering && !appData.renderer.isPaused)
                    toggleRenderingPaused(appData, true); 
                fpsUpdateCounter = 0;
                shouldStopRendering = true;
            }
        }
    }
}

//----------------------------------------------------------------------------//

void toggleRenderingPaused(AppData& appData, bool dueToLowFps)
{
    appData.renderer.isPaused = 
        !appData.renderer.isPaused;
    
    if (appData.renderer.isPaused)
    {
        appData.sharedUniforms.flags.isTimePausedBecauseRenderingPaused = 
            !appData.sharedUniforms.flags.isTimePaused;
        appData.sharedUniforms.flags.isTimePaused = true;
    }
    else if (appData.sharedUniforms.flags.isTimePausedBecauseRenderingPaused)
        appData.sharedUniforms.flags.isTimePaused = false;

    // It would be better to update the StatusBar messages elsewhere, but
    // whatever
    if (appData.renderer.isPaused)
    {
        StatusBar::removeMessageFromQueue("Rendering resumed");
        StatusBar::queueMessage
        (
            dueToLowFps ? 
"Rendering paused because of low FPS. Resume in Properties->Window" :
"Rendering paused"
        );
    }
    else
    {
        StatusBar::removeMessageFromQueue
        (
"Rendering paused because of low FPS. Resume in Properties->Window"
        );
        StatusBar::removeMessageFromQueue
        (
"Rendering paused"
        );
        StatusBar::queueTemporaryMessage
        (
            "Rendering resumed", 
            StatusBar::defaultMessageDuration,
            0xff25ff50
        );
    }
}

//----------------------------------------------------------------------------//

void createNewLayer(AppData& appData, bool compileShader)
{
    // Create and new layer to layers
    unsigned int id = Helpers::findSmallestFreeLayerId(appData.layers);
    auto& layer = *appData.layers.emplace_back(vir::makeUnique<Layer>(id));
    layer.name = "Layer "+std::to_string(id);

    auto window = vir::Window::instance();
    setLayerResolution
    (
        layer, 
        {window->width(), window->height()}, 
        appData.renderer.isTiledRenderingEnabled,
        false
    );

    // Init quad for rendering
    setLayerDepth(layer, (float)appData.layers.size()/Layer::nMaxLayers);

    // Initi unfiorm buffer storage
    layer.renderer.uniformBuffer = 
        vir::DynamicUniformBuffer::create(1024, "privateUniformBlock");
    // First two points taken by shared vertex shader uniform block and shared
    // fragment uniform block
    unsigned int bindingPoint = 2+id;
    layer.renderer.uniformBufferBindingPoint = bindingPoint;
    layer.renderer.uniformBuffer->setBindingPoint(bindingPoint);

    // Set default fragment source in editor
    layer.sourceEditor.setText
    (
R"(void main()
{
/*  Quick description of some important shader inputs and uniforms:

    >>  qc (quad coordinates) represents the coordinates of the current pixel
        (i.e., fragment) in a Euclidian reference frame with the origin at the 
        window center. The magnitude of qc varies from -0.5 to 0.5 along the 
        longest side of the window
    
    >>  tc (texture coordinates) represents the coordinates of the current pixel
        (i.e., fragment) in an affine reference frame with the origin at the
        window bottom-left corner, and where (1, 1) is always at the window top-
        right corner, regardless of the current window size or aspect ratio

    >>  iTime is the elapsed wall time. It can be modified in the 'Uniforms' tab
    
    >>  for a full list of all available uniforms, expand the shader 'Header'
        at the top of the source code. This is inclusive of user-created 
        uniforms in the 'Uniforms' tab*/

    // Output pixel color (all components are in the [0, 1] range)
    fragColor = vec4
    ( 
        .4+.250*sin(2.*(qc.x+iTime)), // Red
        .5+.125*cos(3.*(tc.y+iTime)), // Green
        .75,                          // Blue
        1.                            // Alpha (transparency)
    );
})"
    );
    layer.sourceEditor.resetTextChanged();

    if (compileShader)
        ShaderThing::compileShader(layer, appData);
}

//----------------------------------------------------------------------------//

void setLayerDepth(Layer& layer, const float depth)
{
    layer.depth = depth;
    if (layer.renderer.quad.valid())
        layer.renderer.quad->update
        (
            layer.renderer.quad->width(),
            layer.renderer.quad->height(),
            depth
        );
    else
    {
        auto viewport = Helpers::normalizedWindowResolution();
        layer.renderer.quad = 
            vir::makeUnique<vir::TiledQuad>(viewport.x, viewport.y, depth);
    }
}

//----------------------------------------------------------------------------//

void setLayerFramebufferWrapMode(Layer& layer, int i, WrapMode mode)
{
    layer.renderer.framebufferA->setColorBufferWrapMode(i, mode);
    layer.renderer.framebufferB->setColorBufferWrapMode(i, mode);
}

//----------------------------------------------------------------------------//

void setLayerFramebufferMagFilterMode(Layer& layer, FilterMode mode)
{
    layer.renderer.framebufferA->setColorBufferMagFilterMode(mode);
    layer.renderer.framebufferB->setColorBufferMagFilterMode(mode);
}

//----------------------------------------------------------------------------//

void setLayerFramebufferMinFilterMode(Layer& layer, FilterMode mode)
{
    layer.renderer.framebufferA->setColorBufferMinFilterMode(mode);
    layer.renderer.framebufferB->setColorBufferMinFilterMode(mode);
}

//----------------------------------------------------------------------------//

void rebuildLayerFramebuffers
(
    Layer& layer,
    const vir::TextureBuffer::InternalFormat& internalFormat, 
    const glm::ivec2& resolution,
    const bool isTiledRenderingEnabled
)
{
    auto& renderer = layer.renderer;
    auto rebuildFramebuffer = []
    (
        UPtr<vir::Framebuffer>& framebuffer, 
        UPtr<vir::TiledQuad>& quad,
        const vir::TextureBuffer::InternalFormat& internalFormat, 
        const glm::ivec2& resolution
    )
    {
        if (framebuffer != nullptr)
        {
            auto wrapModeX = framebuffer->colorBufferWrapMode(0);
            auto wrapModeY = framebuffer->colorBufferWrapMode(1);
            auto minFilterMode = framebuffer->colorBufferMinFilterMode();
            auto magFilterMode = framebuffer->colorBufferMagFilterMode();
            
            // Preserve original framebuffer contents after resizing
            auto newFramebuffer = vir::Framebuffer::create
            (
                resolution.x,
                resolution.y,
                internalFormat
            );
            Layer::Renderer::textureMapperShader->bind();
            Layer::Renderer::textureMapperShader->setUniformInt("tx", 0);
            framebuffer->bindColorBuffer(0);
            
            // This rendering step is to copy the original framebuffer contents
            // to the new framebuffer according to the original framebuffer
            // filtering options
            if (quad != nullptr)
                vir::Renderer::instance()->submit
                (
                    *quad, 
                    Layer::Renderer::textureMapperShader.get(),
                    newFramebuffer.get()
                );
            framebuffer->unbind();
            framebuffer = std::move(newFramebuffer);

            framebuffer->setColorBufferWrapMode(0, wrapModeX);
            framebuffer->setColorBufferWrapMode(1, wrapModeY);
            framebuffer->setColorBufferMinFilterMode(minFilterMode);
            framebuffer->setColorBufferMagFilterMode(magFilterMode);
        }
        else
            framebuffer = vir::Framebuffer::create
            (
                resolution.x, 
                resolution.y, 
                internalFormat
            );
    };
    rebuildFramebuffer
    (
        renderer.framebufferA, 
        renderer.quad, 
        internalFormat, 
        glm::max(resolution, {1,1})
    );
    rebuildFramebuffer
    (
        renderer.framebufferB, 
        renderer.quad, 
        internalFormat, 
        glm::max(resolution, {1,1})
    );
    renderer.backFramebuffer = renderer.framebufferA.get();
    renderer.frontFramebuffer = renderer.framebufferB.get();
    renderer.resourceFramebuffer = 
        isTiledRenderingEnabled ?
            renderer.frontFramebuffer :
            renderer.backFramebuffer;
}

//----------------------------------------------------------------------------//

void clearLayerFramebuffers(Layer& layer)
{
    layer.renderer.framebufferA->clearColorBuffer();
    layer.renderer.framebufferB->clearColorBuffer();
}

//----------------------------------------------------------------------------//

std::string assembleFragmentShaderHeader
(
    const Layer& layer, 
    const AppData& appData
)
{
    std::string header =
        vir::Shader::currentContextShadingLanguageDirectives() +
        "in      vec2   qc;\nin      vec2   tc;\nout     vec4   fragColor;\n" +
        appData.sharedStorage.shaderSource() +
        appData.sharedUniforms.fBuffer->shaderSource() +
        "\n";
    /*
    unsigned int imageBindingPoint = 0;
    auto writeResourceUniformsToHeader = []
    (
        const std::vector<vir::UniquePtr<Uniform>>& uniforms, 
        std::string& header,
        unsigned int& nLines,
        unsigned int& imageBindingPoint
    )
    {
        for (auto& u : uniforms)
        {
            // If the uniform has no name, I can't add it to the source
            if (u->name.size() == 0)
                continue;
            std::string uniformTypeName = 
                vir::Shader::uniformTypeToName[u->type()];
            switch (u->type())
            {
                case vir::Uniform::Type::Image2D :
                case vir::Uniform::Type::Image3D :
                case vir::Uniform::Type::ImageCube :
                {
                    auto resource = u->getValuePtr<Resource>();
                    if (resource == nullptr)
                        break;
                    header += 
                        "layout(binding="+std::to_string(imageBindingPoint++)+
                        ", "+resource->internalFormatName()+") ";
                    // This logic should be handled different at the vir:: 
                    // level and exposed via Resource::, not here
                    if (resource->isInternalFormatUnsigned())
                        uniformTypeName = "u"+uniformTypeName;
                    header += "uniform "+uniformTypeName+" "+u->name+";\n";
                    ++nLines;
                    // Also update name of linked resolution uniform
                    u->updateResourceResolutionName();
                    break;
                }
                case vir::Uniform::Type::Sampler2D :
                case vir::Uniform::Type::Sampler3D :
                case vir::Uniform::Type::SamplerCube :
                {
                    auto resource = u->getValuePtr<Resource>();
                    if (resource == nullptr)
                        break;
                    // This logic should be handled different at the vir:: 
                    // level and exposed via Resource::, not here
                    if (resource->isInternalFormatUnsigned())
                        uniformTypeName = "u"+uniformTypeName;
                    header += "uniform "+uniformTypeName+" "+u->name+";\n";
                    ++nLines;
                    // Also update name of linked resolution uniform
                    u->updateResourceResolutionName();
                    break;
                }
                default :
                    break;
            }
        }
    };
    writeResourceUniformsToHeader
    (
        sharedUniforms.userUniforms(),
        header,
        nLines,
        imageBindingPoint
    );
    writeResourceUniformsToHeader
    (
        uniforms_,
        header,
        nLines,
        imageBindingPoint
    );
    */
    header += layer.renderer.uniformBuffer->shaderSource();
    return header;
}

//----------------------------------------------------------------------------//

std::string assembleVertexShaderSource(const AppData& appData)
{
    std::string vertexSource
    (
        vir::Shader::currentContextShadingLanguageDirectives() +
R"(layout (location=0) in vec3 iqc;
layout (location=1) in vec2 itc;
out vec2 qc;
out vec2 tc;
)" + appData.sharedUniforms.vBuffer->shaderSource() +
R"(
void main(){
    gl_Position = iMVP*vec4(iqc, 1.);
    qc = iqc.xy;
    tc = itc;})"
    );
    return vertexSource;
}

//----------------------------------------------------------------------------//

bool compileShader(Layer& layer, AppData& appData, bool setBlankShaderOnError)
{
    layer.sourceHeader = assembleFragmentShaderHeader(layer, appData);
    unsigned int nHeaderLines = Helpers::countNewLines(layer.sourceHeader);
    unsigned int nSharedLines = appData.sharedSourceEditor.getTotalLines()+1;
    std::string vertexSource = assembleVertexShaderSource(appData);
    std::string fragmentSource = 
        (
            layer.sourceHeader +
            appData.sharedSourceEditor.getText()+"\n"+
            layer.sourceEditor.getText()
        );
    auto shader = vir::Shader::create
    (
        vertexSource,
        fragmentSource,
        vir::Shader::ConstructFrom::SourceCode
    );
    if (shader->valid())
    {
        //delete rendering_.shader;
        layer.headerErrors.clear();
        layer.sourceEditor.setErrorMarkers({});
        appData.sharedSourceEditor.setErrorMarkers({});
        layer.cache.uncompiledUniforms.erase
        (
            std::remove_if
            (
                layer.cache.uncompiledUniforms.begin(),
                layer.cache.uncompiledUniforms.end(),
                [](auto& u){return u->name.size()>0;}
            ),
            layer.cache.uncompiledUniforms.end()
        );
        layer.flags.uncompiledChanges = false;
        shader->bindUniformBlock
        (
            layer.renderer.uniformBuffer->name(), 
            layer.renderer.uniformBufferBindingPoint
        );
        shader->bindUniformBlock
        (
            appData.sharedUniforms.fBuffer->name(),
            appData.sharedUniforms.fBufferBindingPoint
        );
        shader->bindUniformBlock
        (
            appData.sharedUniforms.vBuffer->name(),
            appData.sharedUniforms.vBufferBindingPoint
        );
        appData.sharedStorage.bindShader(shader.get());
        shader->bind();
        layer.renderer.shader = std::move(shader);
        return true;
    }
    // Else if shader not valid
    std::map<int, std::string> sourceErrors, sharedErrors;
    for (const auto& error : shader->compilationErrors().fragmentErrors)
    {
        int sourceLineNo(error.first - nSharedLines - nHeaderLines + 1);
        int sharedLineNo(error.first - nHeaderLines);
        if (sourceLineNo > 0)
            sourceErrors.insert({sourceLineNo, error.second});
        else if (sharedLineNo > 0)
            sharedErrors.insert({sharedLineNo, error.second});
        else 
        {
            if (layer.headerErrors.size() > 0)
                layer.headerErrors += "\n";
            layer.headerErrors += "Header: " + error.second;
        }
    }
    auto setEditorErrors = []
    (
        TextEditor& editor,
        const std::map<int, std::string>& errors
    )
    {
        editor.setErrorMarkers(errors);
        if (errors.size() > 0)
            editor.setCursorPosition({errors.begin()->first, 0});
    };
    setEditorErrors(layer.sourceEditor, sourceErrors);
    setEditorErrors(appData.sharedSourceEditor, sharedErrors);
    if (setBlankShaderOnError)
    {
        // Initialize the shader with a blank shader source if any compilation
        // errors are detected (back-end-only, the user will still see the 
        // source of the failed-compilation shader with the full list of 
        // compilation errors and markers)
        std::string vertexSource = assembleVertexShaderSource(appData);
        std::string fragmentSource = 
            vir::Shader::currentContextShadingLanguageDirectives() +
R"(out vec4 fragColor;
in     vec2 qc;
in     vec2 tc;
void main(){fragColor = vec4(0, 0, 0, .5);})";
        layer.renderer.shader = 
            vir::Shader::create
            (
                vertexSource,
                fragmentSource,
                vir::Shader::ConstructFrom::SourceCode
            );
    }
    return false;
}

//----------------------------------------------------------------------------//

void setLayerResolution
(
    Layer& layer,
    glm::ivec2 resolution,
    const bool isTiledRenderingEnabled,
    const bool windowFrameManuallyDragged,
    const bool tryEnfoceWindowAspectRatio,
    const bool setExportResolution
)
{
    auto& lResolution = layer.resolution;
    auto& lResolutionRatio = layer.resolutionRatio;
    auto& lExportData = layer.exportData;
    static const auto* window(vir::Window::instance());
    glm::vec2 windowResolution(window->width(), window->height());
    if (windowFrameManuallyDragged)
    {
        resolution = 
            glm::max(lResolutionRatio*(glm::vec2)resolution+.5f, {1,1});
        auto viewport = Helpers::normalizedWindowResolution();
        layer.renderer.quad->update(viewport.x, viewport.y, layer.depth);
        if (!layer.flags.rescaleWithWindow)
            return;
    }
    else if (!window->iconified())
        lResolutionRatio = (glm::vec2)resolution/windowResolution;

    if (resolution == (glm::ivec2)lResolution)
        return;

    if 
    (
        tryEnfoceWindowAspectRatio &&
        layer.flags.isAspectRatioBoundToWindow &&
        !window->iconified()
    )
    {
        float windowAspectRatio = window->aspectRatio();
        if (resolution.x == (int)lResolution.x)
        {
            lResolution.x = (int)(resolution.y*windowAspectRatio+.5f);
            lResolution.y = resolution.y;
        }
        else if (resolution.y == (int)lResolution.y)
        {
            lResolution.y = (int)(resolution.x/windowAspectRatio+.5f);
            lResolution.x = resolution.x;
        }
        lResolutionRatio = lResolution/windowResolution;
    }
    else
        lResolution = resolution;
    layer.aspectRatio = lResolution.x/lResolution.y;

    if (setExportResolution)
        lExportData.resolution =
            lResolution*
            lExportData.resolutionScale*
            lExportData.windowResolutionScale + .5f;

    rebuildLayerFramebuffers
    (
        layer,
        layer.renderer.backFramebuffer == nullptr ?
        vir::TextureBuffer::InternalFormat::RGBA_SF_32 :
        layer.renderer.backFramebuffer->colorBufferInternalFormat(),
        lResolution,
        isTiledRenderingEnabled
    );
    if (!layer.renderer.shader.valid())
        return;
    layer.renderer.shader->bind();
    //uniformBuffer_->markUniformForSubmission(uniforms_[0].get()); // iAspectRatio
    //uniformBuffer_->markUniformForSubmission(uniforms_[1].get()); // iResolution
}

void renderLayerShader
(
    Layer& layer,
    vir::Framebuffer* target,
    const bool clearTarget,
    AppData& appData
)
{
    auto& renderer = layer.renderer;
    auto isTiledRenderingEnabled = appData.renderer.isTiledRenderingEnabled;
    auto tileIndex = appData.renderer.tileIndex;
    auto flipBuffers = [&renderer, isTiledRenderingEnabled]()
    {
        renderer.backFramebuffer = 
            renderer.backFramebuffer == renderer.framebufferB.get() ? 
            renderer.framebufferA.get() :
            renderer.framebufferB.get();

        renderer.frontFramebuffer = 
            renderer.backFramebuffer == renderer.framebufferB.get() ? 
            renderer.framebufferA.get() :
            renderer.framebufferB.get();

        renderer.resourceFramebuffer = 
            isTiledRenderingEnabled ?
            renderer.frontFramebuffer :
            renderer.backFramebuffer;
    };

    bool allowClearTargetAndPostProcess = true;

    if (isTiledRenderingEnabled)
    {
        if (tileIndex == 0)
            flipBuffers();
        else if 
        (
            tileIndex > renderer.tiles.size-1
        )
            return; // Don't render anything
        else
            allowClearTargetAndPostProcess = false;
        if 
        (
            renderer.tiles.direction == 
            Layer::Renderer::Tiles::Direction::Horizontal
        )
            renderer.quad->selectVisibleTile
            (
                tileIndex, 
                0
            );
        else 
            renderer.quad->selectVisibleTile
            (
                0, 
                tileIndex
            );
    }
    else
        flipBuffers();
    
    // Set sampler-type uniforms found in both this layer's uniforms as well
    // as the shared user-added uniforms
    renderer.shader->bind();
    unsigned int textureUnit = 0; 
    unsigned int imageUnit = 0; 

    // TODO set sampler uniforms
    /*
    auto setSamplerUniforms = []
    (
        const std::vector<vir::UniquePtr<Uniform>>& uniforms,
        Layer* layer, 
        SharedUniforms& sharedUniforms,
        unsigned int& textureUnit,
        unsigned int& imageUnit
    )
    {
        const auto& shader = layer->renderer.shader;
        for (auto& u : uniforms)
        {
            bool isSampler
            (
                u->type() == vir::Uniform::Type::Sampler2D ||
                u->type() == vir::Uniform::Type::Sampler3D ||
                u->type() == vir::Uniform::Type::SamplerCube
            );
            bool isImage
            (
                u->type() == vir::Uniform::Type::Image2D ||
                u->type() == vir::Uniform::Type::Image3D ||
                u->type() == vir::Uniform::Type::ImageCube
            );
            if 
            (
                u->specialType != Uniform::SpecialType::None ||
                u->name.size() == 0 || !(isSampler || isImage)
            )
                continue;
            
            // Sampler or image case
            auto resource = u->getValuePtr<Resource>();
            if (resource == nullptr)
                continue;
            auto ubo = u->isSharedByUser ? 
                sharedUniforms.uniformBuffer().get() : layer->uniformBuffer_.get();
            u->updateResourceResolution(ubo);
            // When reading from your own framebuffer, you should always read
            // from the buffer to which you are NOT writing to (the back buffer
            // is the one that is always being written, so read from the front
            // one)
            if (resource->name() == layer->gui_.name)
            {
                vir::Framebuffer* sourceFramebuffer = 
                    layer->rendering_.frontFramebuffer;
                for (auto& postProcess : layer->rendering_.postProcesses)
                {
                    if 
                    (
                        postProcess->isActive() && 
                        postProcess->outputFramebuffer() != nullptr
                    )
                        sourceFramebuffer = postProcess->outputFramebuffer();
                }
                if (isSampler)
                {
                    sourceFramebuffer->bindColorBuffer(textureUnit);
                    shader->setUniformInt(u->name, textureUnit++);
                }
                else if (isImage)
                {
                    sourceFramebuffer->bindColorBufferToImage
                    (
                        imageUnit, 
                        0, 
                        vir::TextureBuffer::ImageBindMode::ReadWrite
                    );
                    shader->setUniformInt(u->name, imageUnit++);
                }
            }
            else
            {
                if (isSampler)
                {
                    resource->bind(textureUnit);
                    shader->setUniformInt(u->name, textureUnit++);
                }
                else if (isImage)
                {
                    resource->bindImage
                    (
                        imageUnit, 
                        0, 
                        vir::TextureBuffer::ImageBindMode::ReadWrite
                    );
                    shader->setUniformInt(u->name, imageUnit++);
                }
            }
            // Set the (automatically managed) sampler2D/image2D resolution
            // uniform value. Should find a better way rather than setting this 
            // every render call
            if 
            (
                u->type() == vir::Uniform::Type::Sampler2D ||
                u->type() == vir::Uniform::Type::Image2D
            )
            {
                shader->setUniformFloat
                (
                    u->name+"AspectRatio", 
                    float(resource->width())/resource->height()
                );
                shader->setUniformFloat2
                (
                    u->name+"Resolution", 
                    {resource->width(), resource->height()}
                );
            }
            else if 
            (
                u->type() == vir::Uniform::Type::Sampler3D ||
                u->type() == vir::Uniform::Type::Image3D
            )
            {
                shader->setUniformFloat3
                (
                    u->name+"Resolution", 
                    {resource->width(), resource->height(), resource->depth()}
                );
            }
        }
    };
    setSamplerUniforms
    (
        sharedUniforms.userUniforms(), 
        this, 
        sharedUniforms, 
        textureUnit, 
        imageUnit
    );
    setSamplerUniforms
    (
        uniforms_, 
        this, 
        sharedUniforms, 
        textureUnit, 
        imageUnit
    );
    */
    renderer.uniformBuffer->submitUniforms();
    
    // Re-direct rendering & disable blending if not rendering to the window
    static auto globalRenderer = vir::Renderer::instance();
    bool blendingEnabled = true;
    vir::Framebuffer* target0(target);
    if (renderer.target != Layer::Renderer::Target::Window)
    {
        target = renderer.backFramebuffer;
        globalRenderer->setBlending(false);
        blendingEnabled = false;
    }

    // Actual render call
    globalRenderer->submit
    (
        *renderer.quad,
        renderer.shader.get(), // TODO
        target,
        allowClearTargetAndPostProcess && 
        (
            clearTarget || // Or force clear if not rendering to window
            renderer.target != Layer::Renderer::Target::Window
        )
    );
    appData.sharedStorage.gpuMemoryBarrier();

    // Re-enable blending before either leaving or redirecting the rendered 
    // texture to the main window
    if (!blendingEnabled)
        globalRenderer->setBlending(true);

    /* // TODO Post-processing
    // Apply post-processing effects, if any
    if (allowClearTargetAndPostProcess)
    {
        for (auto& postProcess : renderer.postProcesses)
            postProcess->run();
    }*/

    if 
    (
        renderer.target != 
        Layer::Renderer::Target::InternalFramebufferAndWindow
    )
        return;

    Layer::Renderer::textureMapperShader->bind();
    renderer.resourceFramebuffer->bindColorBuffer(0);
    Layer::Renderer::textureMapperShader->setUniformInt("tx", 0);
    globalRenderer->submit
    (
        *renderer.quad, 
        Layer::Renderer::textureMapperShader.get(), // TODO, do not pass raw ptrs
        target0,
        allowClearTargetAndPostProcess && clearTarget
    );
}

RenderResult renderShaders
(
    AppData& appData,
    vir::Framebuffer* target, 
    const unsigned int nRenderPasses
)
{
    auto& sharedUniforms = appData.sharedUniforms;
    static bool clearTarget = true;
    // TODO Fix behavior of stepping to next frame when tiled rendering is
    // enabled
    bool renderFrame = 
        !appData.renderer.isPaused || 
        sharedUniforms.flags.stepToNextFrame;
    bool frameRendered = true;
    unsigned int iRenderPass = appData.renderer.renderPass;

    if (renderFrame)
    {
        if (target != nullptr && iRenderPass == 0) // I.e., if exporting
        {
            for (auto& layer : appData.layers) // Apply clear policy
            {
                switch (layer->exportData.clearPolicy)
                {
                case Layer::ExportData::FramebufferClearPolicy::None :
                    continue;
                case Layer::ExportData::FramebufferClearPolicy::ClearOnFirstFrameExport:
                    if (appData.renderer.frame == 0)
                    {
                        layer->renderer.framebufferA->clearColorBuffer();
                        layer->renderer.framebufferB->clearColorBuffer();
                    }
                    break;
                case Layer::ExportData::FramebufferClearPolicy::ClearOnEveryFrameExport:
                    if (iRenderPass == 0)
                    {
                        layer->renderer.framebufferA->clearColorBuffer();
                        layer->renderer.framebufferB->clearColorBuffer();
                    }
                    break;
                }
            }
        }

        clearTarget = true;
        for (auto& layer : appData.layers)
        {
            renderLayerShader(*layer, target, clearTarget, appData);
            //layer->renderShader(target, clearTarget, sharedUniforms);
            // At the end of this loop, the status of clearTarget will 
            // represent whether the main window has been cleared of its 
            // contents at least once (true if not cleared at least once)
            if 
            (
                clearTarget &&
                layer->renderer.target != 
                    Layer::Renderer::Target::InternalFramebuffer
            )
                clearTarget = false;
        }

        bool nextRenderPass = false;
        // If tiled rendering is enabled, it means that the previous render loop
        // has rendered only the i-th tile of each layer (in pratice, this is 
        // achieved by rendering over a quad that covers only a portion of the
        // rendering target). Here, the index of the tile to be rendered is
        // advanced. The frame is considered fully rendered only if all tiles
        // have been rendered. During exports, tiled rendering is automatically
        // disabled in the exporter setup phase
        if (appData.renderer.isTiledRenderingEnabled)
        {
            if 
            (
                ++appData.renderer.tileIndex == 
                appData.renderer.nTiles
            )
            {
                appData.renderer.tileIndex = 0;
                nextRenderPass = true;
            }
            else
                frameRendered = false;
        }
        else
            nextRenderPass = true;
        
        if (nextRenderPass)
        {
            if (appData.renderer.renderPass < nRenderPasses-1)
                ++appData.renderer.renderPass;
            else
                appData.renderer.renderPass = 0;
            sharedUniforms.fBuffer->markUniformForSubmission(sharedUniforms.iRenderPassUniform.get());
        }
    }
    else
        frameRendered = false;

    // If the window has not been cleared at least once, or if I am not
    // rendering to the window at all (i.e., if renderTarget != nullptr, which 
    // is only true during exports), then render a dummy/void/blank window, 
    // simply to avoid visual artifacts when nothing is rendering to the main
    // window. As for the internalFramebufferShader in Layer::renderShader, the 
    // lifetimeof the shader (and quad) is managed statically within here simply
    // for convenience
    if (frameRendered && (clearTarget || target != nullptr))
    {
        static std::unique_ptr<vir::Quad> blankQuad(new vir::Quad(1, 1, 0));
        auto viewport = Helpers::normalizedWindowResolution();
        blankQuad->update(viewport.x, viewport.y, 0);
        auto constructBlankShader = [&appData]()
        {
            auto shader = 
                vir::Shader::create
                (
                    assembleVertexShaderSource(appData),
                    vir::Shader::currentContextShadingLanguageDirectives() +
R"(out vec4 fragColor;
in     vec2 qc;
in     vec2 tc;
void main(){fragColor = vec4(0, 0, 0, .5);})",
                    vir::Shader::ConstructFrom::SourceCode
                );
            shader->bindUniformBlock
            (
                appData.sharedUniforms.fBuffer->name(),
                appData.sharedUniforms.fBufferBindingPoint
            );
            shader->bindUniformBlock
            (
                appData.sharedUniforms.vBuffer->name(),
                appData.sharedUniforms.vBufferBindingPoint
            );
            return shader;
        };
        static auto blankShader = constructBlankShader();
        vir::Renderer::instance()->submit
        (
            *blankQuad.get(), 
            blankShader.get()
        );
    }

    return 
    {
        iRenderPass == nRenderPasses-1, 
        frameRendered
    };
}

}