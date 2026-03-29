#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/corelogic.h"
#include "shaderthing/include/helpers.h"
#include "shaderthing/include/structs.h"
#include "shaderthing/include/oo/eventmanager.h"
#include "shaderthing/include/oo/texteditor.h"
#include "shaderthing/include/oo/statusbar.h"
#include "shaderthing/include/oo/uniform.h"
#include "vir/include/vir.h"

namespace ShaderThing
{

UPtr<vir::Shader> Layer::Rendering::textureMapperShader;

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
    Layer::Rendering::textureMapperShader =
        vir::Shader::create
        (
            vertexSource,
            fragmentSource,
            vir::Shader::ConstructFrom::SourceCode
        );
    Layer::Rendering::textureMapperShader->bind();
    Layer::Rendering::textureMapperShader->bindUniformBlock
    (
        appData.sharedUniforms.vertex.uniformBuffer->name(),
        appData.sharedUniforms.vertex.uniformBufferBindingPoint
    );
    Layer::Rendering::textureMapperShader->setUniformInt("tx", 0);

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

    // Init random random number
    su.iRandom = std::uniform_real_distribution<float>(0, 1)(su.rndGenerator);

    // Init uniform buffers, bind to designated binding points and set
    // initial data
    su.vertex.uniformBuffer = 
            vir::DynamicUniformBuffer::create(64, "vertexSharedUniformBlock");
    su.vertex.uniformBuffer->bind();
    su.vertex.uniformBufferBindingPoint = 1;
    su.vertex.uniformBuffer->setBindingPoint
    (
        su.vertex.uniformBufferBindingPoint
    );

    su.iMVPUniform = Uniform::create(su.vertex).getWeak();
    su.iMVPUniform->name = "iMVP";
    su.iMVPUniform->setValuePtr
    (
        &(su.screenCamera->projectionViewMatrix()), 
        Uniform::Type::Mat4
    );
    su.iMVPUniform->gui.showBounds = false;

    su.fragment.uniformBuffer = 
            vir::DynamicUniformBuffer::create(8196, "sharedUniformBlock");
    su.fragment.uniformBuffer->bind();
    su.fragment.uniformBufferBindingPoint = 0;
    su.fragment.uniformBuffer->setBindingPoint
    (
        su.fragment.uniformBufferBindingPoint
    );

    // Init uniform wrappers
    su.iFrameUniform = Uniform::create(su.fragment).getWeak();
    su.iFrameUniform->name = "iFrame";
    su.iFrameUniform->setValuePtr
    (
        &appData.rendering.frameIndex, 
        Uniform::Type::Int
    );
    su.iFrameUniform->gui.showBounds = false;

    su.iRenderPassUniform = Uniform::create(su.fragment).getWeak();
    su.iRenderPassUniform->name = "iRenderPass";
    su.iRenderPassUniform->setValuePtr
    (
        &appData.rendering.passIndex, 
        Uniform::Type::Int
    );
    su.iRenderPassUniform->gui.showBounds = false;
    
    su.iTimeUniform = Uniform::create(su.fragment).getWeak();
    su.iTimeUniform->name = "iTime";
    su.iTimeUniform->setValuePtr(&su.iTime, Uniform::Type::Float);
    
    su.iTimeDeltaUniform = Uniform::create(su.fragment).getWeak();
    su.iTimeDeltaUniform->name = "iTimeDelta";
    su.iTimeDeltaUniform->setValuePtr(&su.iTimeDelta, Uniform::Type::Float);
    su.iTimeDeltaUniform->gui.showBounds = false;

    su.iRandomUniform = Uniform::create(su.fragment).getWeak();
    su.iRandomUniform->name = "iRandom";
    su.iRandomUniform->setValuePtr(&su.iRandom, Uniform::Type::Float);
    su.iRandomUniform->gui.showBounds = false;

    su.iUserActionUniform = Uniform::create(su.fragment).getWeak();
    su.iUserActionUniform->name = "iUserAction";
    su.iUserActionUniform->setValuePtr(&su.iUserAction, Uniform::Type::Bool);
    su.iUserActionUniform->gui.showBounds = false;

    su.iExportUniform = Uniform::create(su.fragment).getWeak();
    su.iExportUniform->name = "iExport";
    su.iExportUniform->setValuePtr
    (
        &appData.exporter.isActive, 
        Uniform::Type::Bool
    );
    su.iExportUniform->gui.showBounds = false;

    su.iWASDUniform = Uniform::create(su.fragment).getWeak();
    su.iWASDUniform->name = "iWASD";
    su.iWASDUniform->setValuePtr(&su.iWASD, Uniform::Type::Float3);

    su.iLookUniform = Uniform::create(su.fragment).getWeak();
    su.iLookUniform->name = "iLook";
    su.iLookUniform->setValuePtr(&su.iLook, Uniform::Type::Float3);
    su.iLookUniform->gui.showBounds = false;

    su.iMouseUniform = Uniform::create(su.fragment).getWeak();
    su.iMouseUniform->name = "iMouse";
    su.iMouseUniform->setValuePtr(&su.iMouse, Uniform::Type::Float4);
    su.iMouseUniform->gui.showBounds = false;

    su.iAspectRatioUniform = Uniform::create(su.fragment).getWeak();
    su.iAspectRatioUniform->name = "iWindowAspectRatio";
    su.iAspectRatioUniform->setValuePtr(&su.iAspectRatio, Uniform::Type::Float);
    su.iAspectRatioUniform->gui.showBounds = false;

    su.iResolutionUniform = Uniform::create(su.fragment).getWeak();
    su.iResolutionUniform->name = "iWindowResolution";
    su.iResolutionUniform->setValuePtr(&su.iResolution, Uniform::Type::Float2);
    su.iResolutionUniform->gui.showBounds = false;

    su.iKeyboardUniform = Uniform::create(su.fragment).getWeak();
    su.iKeyboardUniform->name = "iKeyboard";
    su.iKeyboardUniform->setValuePtr(&su.iKeyboard, Uniform::Type::Int3, 256);
    su.iKeyboardUniform->gui.showBounds = false;
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
    appData.deferredActionBuffer.process();
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
        appData.exporter.outputResolution = resolution;
        appData.exporter.outputResolutionScale = 1.f;
    }

    // Update screen camera
    su.screenCamera->setViewportHeight
    (
        std::min(1.0f, 1.0f/su.iAspectRatio)
    );
    su.screenCamera->update();

    //iMVP_ = screenCamera_->projectionViewMatrix();
    su.vertex.uniformBuffer->markUniformForSubmission(su.iMVPUniform.get());
    su.fragment.uniformBuffer->markUniformForSubmission(su.iAspectRatioUniform.get());
    su.fragment.uniformBuffer->markUniformForSubmission(su.iResolutionUniform.get());
    
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
            appData.rendering.passIndex == 
            appData.exporter.nRenderPasses-1
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
        timeStep = (su.isTimeDeltaSmooth ?
            vir::Window::instance()->time()->smoothOuterTimestep() : 
            vir::Window::instance()->time()->outerTimestep());
        if (appData.rendering.isTiledRenderingEnabled)
        {
            static float cumulatedTimeStep = 0;
            if (!appData.rendering.isPaused)
                cumulatedTimeStep += timeStep;
            if (appData.rendering.tileIndex == 0)
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

    if (!su.isTimePaused)
    {
        su.iTime += timeStep;
        if (advanceFrame)
            su.iTimeDelta = timeStep;
    }
    else if 
    (
        advanceFrame && 
        (
            appData.rendering.toggles.stepToNextFrame || 
            su.toggles.stepToNextTimeStep
        )
    )
        su.iTime += su.iTimeDelta;

    const glm::vec2& timeLoopBounds(su.iTimeUniform->gui.bounds);
    if (su.isTimeLooped && su.iTime >= timeLoopBounds.y)
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
        !(
            appData.rendering.isPaused && 
            !appData.rendering.toggles.stepToNextFrame
        )
    )
        ++appData.rendering.frameIndex;

    if (appData.rendering.toggles.resetFrameCounterPreOrPostExport)
    {
        appData.rendering.frameIndex = 0;
        appData.rendering.toggles.resetFrameCounterPreOrPostExport = false;
    }
    if (appData.rendering.toggles.resetFrameCounter)
    {
        appData.rendering.frameIndex = 0;
        if (su.isTimeResetOnFrameCounterReset)
            su.iTime = 0;
        appData.rendering.toggles.resetFrameCounter = false;
    }

    // The shaderCamera has its own event listeners, but all of its updates are
    // deferred (just like here, nothing is processed/sent to the GPU in the
    // event callback), so we update it here and check whether the GPU data
    // should be updated as well
    su.shaderCamera->update();

    // Re-gen random number
    if (!su.isRandomNumberGeneratorPaused)
        su.iRandom = std::uniform_real_distribution<float>(0, 1)
        (
            su.rndGenerator
        );

    if 
    (
        su.iWASD != su.shaderCamera->position() ||
        su.iLook != su.shaderCamera->z()
    )
    {
        su.iWASD = su.shaderCamera->position();
        su.iLook = su.shaderCamera->z();
        su.iUserAction = true;
        su.toggles.updateDataRangeII = true;
    }
    
    // Data range I is always updated, data range III is updated on the spot
    // in setResolution, the keyboard data range is updated on the spot in
    // onReceive(KeyPressEvent/KeyReleaseEvent)
    su.fragment.uniformBuffer->markContiguousUniformsForSubmission
    (
        su.iFrameUniform.get(), 
        su.iRandomUniform.get()
    );
    if (su.toggles.updateDataRangeII)
    {
        su.fragment.uniformBuffer->markContiguousUniformsForSubmission
        (
            su.iUserActionUniform.get(), 
            su.iMouseUniform.get()
        );
        su.toggles.updateDataRangeII = false;
    }

    su.vertex.uniformBuffer->submitUniforms();
    su.fragment.uniformBuffer->submitUniforms();

    if (su.iUserAction) // Always reset
    {
        su.toggles.updateDataRangeII = true;
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
        if (appData.rendering.isTiledRenderingEnabled)
        {
            double wFps = fps/appData.rendering.nTiles;
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
                shouldStopRendering && fps < appData.rendering.lowerFpsLimit;
            if (fpsUpdateCounter >= int(maxLowFpsPeriod/fpsUpdatePeriod))
            {
                if (shouldStopRendering && !appData.rendering.isPaused)
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
    appData.rendering.isPaused = 
        !appData.rendering.isPaused;
    
    if (appData.rendering.isPaused)
    {
        appData.sharedUniforms.isTimePausedBecauseRenderingPaused = 
            !appData.sharedUniforms.isTimePaused;
        appData.sharedUniforms.isTimePaused = true;
    }
    else if 
    (
        appData.sharedUniforms.isTimePausedBecauseRenderingPaused
    )
        appData.sharedUniforms.isTimePaused = false;

    // It would be better to update the StatusBar messages elsewhere, but
    // whatever
    if (appData.rendering.isPaused)
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
    auto& layer = appData.layers.emplace_back(vir::makeUnique<Layer>(id));
    layer->name = "Layer "+std::to_string(id);

    auto window = vir::Window::instance();
    setLayerResolution
    (
        layer, 
        {window->width(), window->height()}, 
        appData.rendering.isTiledRenderingEnabled,
        false
    );

    // Init quad for rendering
    setLayerDepth(layer, (float)appData.layers.size()/Layer::nMaxLayers);

    // Init unfiorm buffer storage
    layer->uniformBuffer = 
        vir::DynamicUniformBuffer::create(1024, "privateUniformBlock");
    // First two points taken by shared vertex shader uniform block and shared
    // fragment uniform block
    unsigned int bindingPoint = 2+id;
    layer->uniformBufferBindingPoint = bindingPoint;
    layer->uniformBuffer->setBindingPoint(bindingPoint);

    // Add default uniforms
    {
        auto& u = Uniform::create(layer);
        u->managedType = Uniform::ManagedType::LayerAspectRatio;
        u->name = "iAspectRatio";
        u->setValuePtr(&layer->aspectRatio, Uniform::Type::Float);
        u->gui.showBounds = false;
        layer->rendering.iAspectRatioUniform = u.getWeak();
    }
    {
        auto& u = Uniform::create(layer);
        u->managedType = Uniform::ManagedType::LayerResolution;
        u->name = "iResolution";
        u->setValuePtr(&layer->resolution, Uniform::Type::Float2);
        u->gui.bounds = glm::vec2(1.0f, 4096.0f);
        u->gui.showBounds = false;
        layer->rendering.iResolutionUniform = u.getWeak();
    }

    // Set default fragment source in editor
    layer->sourceEditor.setText
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
    layer->sourceEditor.resetTextChanged();

    if (compileShader)
        ShaderThing::compileShader(layer, appData);

    if (appData.rendering.isTiledRenderingEnabled)  
        setRenderingTiles(appData, appData.rendering.nTiles);
}

//----------------------------------------------------------------------------//

void setLayerDepth(UPtr<Layer>& layer, const float depth)
{
    layer->depth = depth;
    if (layer->rendering.quad.valid())
        layer->rendering.quad->update
        (
            layer->rendering.quad->width(),
            layer->rendering.quad->height(),
            depth
        );
    else
    {
        auto viewport = Helpers::normalizedWindowResolution();
        layer->rendering.quad = 
            vir::makeUnique<vir::TiledQuad>(viewport.x, viewport.y, depth);
    }
}

//----------------------------------------------------------------------------//

void setLayerFramebufferWrapMode(UPtr<Layer>& layer, int i, WrapMode mode)
{
    layer->rendering.framebufferA->setColorBufferWrapMode(i, mode);
    layer->rendering.framebufferB->setColorBufferWrapMode(i, mode);
}

//----------------------------------------------------------------------------//

void setLayerFramebufferMagFilterMode(UPtr<Layer>& layer, FilterMode mode)
{
    layer->rendering.framebufferA->setColorBufferMagFilterMode(mode);
    layer->rendering.framebufferB->setColorBufferMagFilterMode(mode);
}

//----------------------------------------------------------------------------//

void setLayerFramebufferMinFilterMode(UPtr<Layer>& layer, FilterMode mode)
{
    layer->rendering.framebufferA->setColorBufferMinFilterMode(mode);
    layer->rendering.framebufferB->setColorBufferMinFilterMode(mode);
}

//----------------------------------------------------------------------------//

void rebuildLayerFramebuffers
(
    UPtr<Layer>& layer,
    const vir::TextureBuffer::InternalFormat& internalFormat, 
    const glm::ivec2& resolution,
    const bool isTiledRenderingEnabled
)
{
    auto& rendering = layer->rendering;
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
            Layer::Rendering::textureMapperShader->bind();
            Layer::Rendering::textureMapperShader->setUniformInt("tx", 0);
            framebuffer->bindColorBuffer(0);
            
            // This rendering step is to copy the original framebuffer contents
            // to the new framebuffer according to the original framebuffer
            // filtering options
            if (quad != nullptr)
                vir::Renderer::instance()->submit
                (
                    *quad, 
                    Layer::Rendering::textureMapperShader.get(),
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
        rendering.framebufferA, 
        rendering.quad, 
        internalFormat, 
        glm::max(resolution, {1,1})
    );
    rebuildFramebuffer
    (
        rendering.framebufferB, 
        rendering.quad, 
        internalFormat, 
        glm::max(resolution, {1,1})
    );
    rendering.backFramebuffer = rendering.framebufferA.get();
    rendering.frontFramebuffer = rendering.framebufferB.get();
    rendering.resourceFramebuffer = 
        isTiledRenderingEnabled ?
            rendering.frontFramebuffer :
            rendering.backFramebuffer;
}

//----------------------------------------------------------------------------//

void clearLayerFramebuffers(UPtr<Layer>& layer)
{
    layer->rendering.framebufferA->clearColorBuffer();
    layer->rendering.framebufferB->clearColorBuffer();
}

//----------------------------------------------------------------------------//

std::string assembleFragmentShaderHeader
(
    const UPtr<Layer>& layer, 
    const AppData& appData
)
{
    std::string header =
        vir::Shader::currentContextShadingLanguageDirectives() +
        "in      vec2   qc;\nin      vec2   tc;\nout     vec4   fragColor;\n" +
        appData.sharedStorage.shaderSource() +
        appData.sharedUniforms.fragment.uniformBuffer->shaderSource() +
        "\n";
    unsigned int nLines = 0;
    unsigned int imageBindingPoint = 0;
    auto writeResourceUniformsToHeader = []
    (
        const UPtrVector<Uniform>& uniforms, 
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
                    // This logic should be handled differently at the vir:: 
                    // level and exposed via Resource::, not here
                    if (resource->isInternalFormatUnsigned())
                        uniformTypeName = "u"+uniformTypeName;
                    header += "uniform "+uniformTypeName+" "+u->name+";\n";
                    ++nLines;
                    // Also update name of linked resolution uniform
                    if (u->resourceResolutionUniform().valid())
                        u->resourceResolutionUniform()->name = u->name +
                            "Resolution";
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
                    if (u->resourceResolutionUniform().valid())
                        u->resourceResolutionUniform()->name = u->name +
                            "Resolution";
                    break;
                }
                default :
                    break;
            }
        }
    };
    writeResourceUniformsToHeader
    (
        appData.sharedUniforms.fragment.uniforms,
        header,
        nLines,
        imageBindingPoint
    );
    writeResourceUniformsToHeader
    (
        layer->uniforms,
        header,
        nLines,
        imageBindingPoint
    );
    header += layer->uniformBuffer->shaderSource();
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
)" + appData.sharedUniforms.vertex.uniformBuffer->shaderSource() +
R"(
void main(){
    gl_Position = iMVP*vec4(iqc, 1.);
    qc = iqc.xy;
    tc = itc;})"
    );
    return vertexSource;
}

//----------------------------------------------------------------------------//

bool compileShader(UPtr<Layer>& layer, AppData& appData, bool setBlankShaderOnError)
{
    layer->sourceHeader = assembleFragmentShaderHeader(layer, appData);
    unsigned int nHeaderLines = Helpers::countNewLines(layer->sourceHeader);
    unsigned int nSharedLines = appData.sharedSourceEditor.getTotalLines()+1;
    std::string vertexSource = assembleVertexShaderSource(appData);
    std::string fragmentSource = 
        (
            layer->sourceHeader +
            appData.sharedSourceEditor.getText()+"\n"+
            layer->sourceEditor.getText()
        );
    auto shader = vir::Shader::create
    (
        vertexSource,
        fragmentSource,
        vir::Shader::ConstructFrom::SourceCode
    );
    if (shader->valid())
    {
        layer->headerErrors.clear();
        layer->sourceEditor.setErrorMarkers({});
        appData.sharedSourceEditor.setErrorMarkers({});
        layer->cache.uncompiledUniforms.erase
        (
            std::remove_if
            (
                layer->cache.uncompiledUniforms.begin(),
                layer->cache.uncompiledUniforms.end(),
                [](WPtr<Uniform>& u)
                {
                    if (u.valid())
                        return u->name.size()>0;
                    else
                        return true;
                }
            ),
            layer->cache.uncompiledUniforms.end()
        );
        layer->hasUncompiledEdits = false;
        shader->bindUniformBlock
        (
            layer->uniformBuffer->name(), 
            layer->uniformBufferBindingPoint
        );
        shader->bindUniformBlock
        (
            appData.sharedUniforms.fragment.uniformBuffer->name(),
            appData.sharedUniforms.fragment.uniformBufferBindingPoint
        );
        shader->bindUniformBlock
        (
            appData.sharedUniforms.vertex.uniformBuffer->name(),
            appData.sharedUniforms.vertex.uniformBufferBindingPoint
        );
        appData.sharedStorage.bindShader(shader.get());
        shader->bind();
        layer->rendering.shader = std::move(shader);
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
            if (layer->headerErrors.size() > 0)
                layer->headerErrors += "\n";
            layer->headerErrors += "Header: " + error.second;
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
    setEditorErrors(layer->sourceEditor, sourceErrors);
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
        layer->rendering.shader = 
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
    UPtr<Layer>& layer,
    glm::ivec2 resolution,
    const bool isTiledRenderingEnabled,
    const bool windowFrameManuallyDragged,
    const bool tryEnfoceWindowAspectRatio,
    const bool setExportResolution
)
{
    auto& lResolution = layer->resolution;
    auto& lResolutionRatio = layer->resolutionRatio;
    auto& lExportData = layer->exportData;
    static const auto* window(vir::Window::instance());
    glm::vec2 windowResolution(window->width(), window->height());
    if (windowFrameManuallyDragged)
    {
        resolution = 
            glm::max(lResolutionRatio*(glm::vec2)resolution+.5f, {1,1});
        auto viewport = Helpers::normalizedWindowResolution();
        layer->rendering.quad->update(viewport.x, viewport.y, layer->depth);
        if (!layer->rescaleWithWindow)
            return;
    }
    else if (!window->iconified())
        lResolutionRatio = (glm::vec2)resolution/windowResolution;

    if (resolution == (glm::ivec2)lResolution)
        return;

    if 
    (
        tryEnfoceWindowAspectRatio &&
        layer->isAspectRatioBoundToWindow &&
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
    layer->aspectRatio = lResolution.x/lResolution.y;

    if (setExportResolution)
        lExportData.resolution =
            lResolution*
            lExportData.resolutionScale*
            lExportData.windowResolutionScale + .5f;

    rebuildLayerFramebuffers
    (
        layer,
        layer->rendering.backFramebuffer == nullptr ?
        vir::TextureBuffer::InternalFormat::RGBA_SF_32 :
        layer->rendering.backFramebuffer->colorBufferInternalFormat(),
        lResolution,
        isTiledRenderingEnabled
    );
    if (!layer->rendering.shader.valid())
        return;
    layer->rendering.shader->bind();
    if (layer->rendering.iAspectRatioUniform.valid())
        layer->rendering.iAspectRatioUniform->
            markForSubmissionToAllClientBuffers();
    if (layer->rendering.iResolutionUniform.valid())
        layer->rendering.iResolutionUniform->
            markForSubmissionToAllClientBuffers();
}

void setRenderingTiles
(
    AppData& appData, 
    int nTiles
)
{
    if (appData.layers.size() == 0)
        return;
    nTiles = std::max(nTiles, 1);
    appData.rendering.isTiledRenderingEnabled = nTiles > 1;
    appData.rendering.nTilesCache = appData.rendering.nTiles;
    appData.rendering.nTiles = nTiles;
    appData.rendering.tileIndex = 0;
    double largestLayerSize = 0.0; // Mpx
    for (auto& layer : appData.layers)
    {
        double layerSize = 
            ((double)layer->resolution.x/1024.0)*
            ((double)layer->resolution.y/1024.0);
        if (layerSize > largestLayerSize)
            largestLayerSize = layerSize;
    }
    for (auto& layer : appData.layers)
    {
        double layerSize = 
            ((double)layer->resolution.x/1024.0)*
            ((double)layer->resolution.y/1024.0);
        unsigned int nt = 
            std::max
            (
                (unsigned int)nTiles*
                (unsigned int)(layerSize/largestLayerSize),
                1u
            );
        auto& tiles = layer->rendering.tiles;
        if (layer->resolution.x >= layer->resolution.y)
        {
            tiles.direction = Layer::Rendering::Tiles::Direction::Horizontal;
            nt = std::min(nt, (unsigned int)layer->resolution.x);
            layer->rendering.quad->update(nt, 1);
        }
        else
        {
            tiles.direction = Layer::Rendering::Tiles::Direction::Vertical;
            nt = std::min(nt, (unsigned int)layer->resolution.y);
            layer->rendering.quad->update(1, nt);
        }
        tiles.size = nt;
    }
}

void renderLayerShader
(
    UPtr<Layer>& layer,
    vir::Framebuffer* target,
    const bool clearTarget,
    AppData& appData
)
{
    auto& rendering = layer->rendering;
    auto isTiledRenderingEnabled = appData.rendering.isTiledRenderingEnabled;
    auto tileIndex = appData.rendering.tileIndex;
    auto flipBuffers = [&rendering, isTiledRenderingEnabled]()
    {
        rendering.backFramebuffer = 
            rendering.backFramebuffer == rendering.framebufferB.get() ? 
            rendering.framebufferA.get() :
            rendering.framebufferB.get();

        rendering.frontFramebuffer = 
            rendering.backFramebuffer == rendering.framebufferB.get() ? 
            rendering.framebufferA.get() :
            rendering.framebufferB.get();

        rendering.resourceFramebuffer = 
            isTiledRenderingEnabled ?
            rendering.frontFramebuffer :
            rendering.backFramebuffer;
    };

    bool allowClearTargetAndPostProcess = true;

    if (isTiledRenderingEnabled)
    {
        if (tileIndex == 0)
            flipBuffers();
        else if 
        (
            tileIndex > rendering.tiles.size-1
        )
            return; // Don't render anything
        else
            allowClearTargetAndPostProcess = false;
        if 
        (
            rendering.tiles.direction == 
            Layer::Rendering::Tiles::Direction::Horizontal
        )
            rendering.quad->selectVisibleTile
            (
                tileIndex, 
                0
            );
        else 
            rendering.quad->selectVisibleTile
            (
                0, 
                tileIndex
            );
    }
    else
        flipBuffers();
    
    // Set sampler-type uniforms found in both this layer's uniforms as well
    // as the shared user-added uniforms
    rendering.shader->bind();

    //TODO, set sampler/image uniforms once Resource-stuff implemented
    unsigned int textureUnit = 0;
    unsigned int imageUnit = 0;
    auto setSamplerUniforms = []
    (
        const UPtrVector<Uniform>& uniforms,
        UPtr<Layer>& layer, 
        SharedUniforms& sharedUniforms,
        unsigned int& textureUnit,
        unsigned int& imageUnit
    )
    {
        const auto& shader = layer->rendering.shader;
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
                // TODO Check what this first condition was for
                // u->specialType != Uniform::SpecialType::None || 
                u->name.size() == 0 || !(isSampler || isImage)
            )
                continue;
            
            // Sampler or image case
            auto resource = u->getValuePtr<Resource>();
            if (resource == nullptr)
                continue;
            
            // Update resource resolution
            auto ubo = u->isSharedByUser ? 
                sharedUniforms.fragment.uniformBuffer.get() : 
                layer->uniformBuffer.get();
            auto rru = u->resourceResolutionUniform();
            if (!rru.valid())
                continue; // TODO log or handle
            if (rru->type() == Uniform::Type::Float2)
            {
                auto* value = rru->getValuePtr<glm::vec2>();
                if 
                (
                    value->x != resource->width() || 
                    value->y != resource->height()
                )
                {
                    value->x = resource->width();
                    value->y = resource->height();
                    ubo->markUniformForSubmission(rru.get());
                }
            }
            else if (rru->type() == Uniform::Type::Float3)
            {
                auto* value = rru->getValuePtr<glm::vec3>();
                if 
                (
                    value->x != resource->width() || 
                    value->y != resource->height() ||
                    value->z != resource->depth()
                )
                {
                    value->x = resource->width();
                    value->y = resource->height();
                    value->z = resource->depth();
                    ubo->markUniformForSubmission(rru.get());
                }
            }
            
            // When reading from your own framebuffer, you should always read
            // from the buffer to which you are NOT writing to (the back buffer
            // is the one that is always being written, so read from the front
            // one)
            if (resource->name() == layer->name)
            {
                vir::Framebuffer* sourceFramebuffer = 
                    layer->rendering.frontFramebuffer;
                // TODO Add back when postProcessing implemented
                // for (auto& postProcess : layer->rendering.postProcesses)
                // {
                //     if 
                //     (
                //         postProcess->isActive() && 
                //         postProcess->outputFramebuffer() != nullptr
                //     )
                //         sourceFramebuffer = postProcess->outputFramebuffer();
                // }
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
        appData.sharedUniforms.fragment.uniforms, 
        layer, 
        appData.sharedUniforms, 
        textureUnit, 
        imageUnit
    );
    setSamplerUniforms
    (
        layer->uniforms, 
        layer, 
        appData.sharedUniforms, 
        textureUnit, 
        imageUnit
    );

    layer->uniformBuffer->submitUniforms();
    
    // Re-direct rendering & disable blending if not rendering to the window
    static auto globalRendering = vir::Renderer::instance();
    bool blendingEnabled = true;
    vir::Framebuffer* target0(target);
    if (rendering.target != Layer::Rendering::Target::Window)
    {
        target = rendering.backFramebuffer;
        globalRendering->setBlending(false);
        blendingEnabled = false;
    }

    // Actual render call
    globalRendering->submit
    (
        *rendering.quad,
        rendering.shader.get(), // TODO
        target,
        allowClearTargetAndPostProcess && 
        (
            clearTarget || // Or force clear if not rendering to window
            rendering.target != Layer::Rendering::Target::Window
        )
    );
    appData.sharedStorage.gpuMemoryBarrier();

    // Re-enable blending before either leaving or redirecting the rendered 
    // texture to the main window
    if (!blendingEnabled)
        globalRendering->setBlending(true);

    /* // TODO Post-processing
    // Apply post-processing effects, if any
    if (allowClearTargetAndPostProcess)
    {
        for (auto& postProcess : rendering.postProcesses)
            postProcess->run();
    }*/

    if 
    (
        rendering.target != 
        Layer::Rendering::Target::InternalFramebufferAndWindow
    )
        return;

    Layer::Rendering::textureMapperShader->bind();
    rendering.resourceFramebuffer->bindColorBuffer(0);
    Layer::Rendering::textureMapperShader->setUniformInt("tx", 0);
    globalRendering->submit
    (
        *rendering.quad, 
        Layer::Rendering::textureMapperShader.get(),
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
        !appData.rendering.isPaused || 
        appData.rendering.toggles.stepToNextFrame;
    bool frameRendered = true;
    unsigned int iRenderPass = appData.rendering.passIndex;

    if (renderFrame)
    {
        if (target != nullptr && iRenderPass == 0) // I.e., if exporting
        {
            for (auto& layer : appData.layers) // Apply clear policy
            {
                switch (layer->exportData.clearPolicy)
                {
                case Layer::ExportData::FramebufferClearPolicy::
                    None :
                    continue;
                case Layer::ExportData::FramebufferClearPolicy::
                    ClearOnFirstFrameExport:
                    if (appData.rendering.passIndex == 0)
                    {
                        layer->rendering.framebufferA->clearColorBuffer();
                        layer->rendering.framebufferB->clearColorBuffer();
                    }
                    break;
                case Layer::ExportData::FramebufferClearPolicy::
                    ClearOnEveryFrameExport:
                    if (iRenderPass == 0)
                    {
                        layer->rendering.framebufferA->clearColorBuffer();
                        layer->rendering.framebufferB->clearColorBuffer();
                    }
                    break;
                }
            }
        }

        clearTarget = true;
        for (auto& layer : appData.layers)
        {
            renderLayerShader(layer, target, clearTarget, appData);
            // At the end of this loop, the status of clearTarget will 
            // represent whether the main window has been cleared of its 
            // contents at least once (true if NOT cleared at least once)
            if 
            (
                clearTarget &&
                layer->rendering.target != 
                    Layer::Rendering::Target::InternalFramebuffer
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
        if (appData.rendering.isTiledRenderingEnabled)
        {
            if 
            (
                ++appData.rendering.tileIndex == 
                appData.rendering.nTiles
            )
            {
                appData.rendering.tileIndex = 0;
                nextRenderPass = true;
            }
            else
                frameRendered = false;
        }
        else
            nextRenderPass = true;
        
        if (nextRenderPass)
        {
            if (appData.rendering.passIndex < nRenderPasses-1)
                ++appData.rendering.passIndex;
            else
                appData.rendering.passIndex = 0;
            sharedUniforms.fragment.uniformBuffer->markUniformForSubmission
            (
                sharedUniforms.iRenderPassUniform.get()
            );
        }
    }
    else
        frameRendered = false;

    // If the window has not been cleared at least once, or if I am not
    // rendering to the window at all (i.e., if renderTarget != nullptr, which 
    // is only true during exports), then render a dummy/void/blank window, 
    // simply to avoid visual artifacts when nothing is rendering to the main
    // window. Both the blank shader and quad are managed statically here, for
    // convenience
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
                appData.sharedUniforms.fragment.uniformBuffer->name(),
                appData.sharedUniforms.fragment.uniformBufferBindingPoint
            );
            shader->bindUniformBlock
            (
                appData.sharedUniforms.vertex.uniformBuffer->name(),
                appData.sharedUniforms.vertex.uniformBufferBindingPoint
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
    return RenderResult
    {
        iRenderPass == nRenderPasses-1, 
        frameRendered
    };
}

//----------------------------------------------------------------------------//

void addLayerToResources(UPtr<Layer>& layer, UPtrVector<Resource>& resources)
{
    for (int i=0; i<(int)resources.size(); i++)
    {
        UPtr<Resource>& resource = resources[i];
        if (resource->type() != Resource::Type::Framebuffer)
            continue;
        if (resource->name() == layer->name)
            return;
    }
    UPtr<Resource>& resource = 
        resources.emplace_back(LayerResource::create(layer));
    resource->setName(&(layer->name));
}

//----------------------------------------------------------------------------//

void removeLayerFromResources
(
    UPtr<Layer>& layer, 
    UPtrVector<Resource>& resources
)
{
    for (int i=0; i<(int)resources.size(); i++)
    {
        UPtr<Resource>& resource = resources[i];
        if (resource->type() != Resource::Type::Framebuffer)
            continue;
        if (resource->name() == layer->name)
        {
            resources.erase(resources.begin()+i);
            return;
        }
    }
}

//----------------------------------------------------------------------------//

void toggleKeyboardInputs(AppData& appData)
{
    appData.sharedUniforms.isKeyboardInputEnabled = 
        !appData.sharedUniforms.isKeyboardInputEnabled;
    auto eventManager = GPtr<EventManager>::get();
    if (appData.sharedUniforms.isKeyboardInputEnabled)
    {
        eventManager->resumeEventReception(vir::Event::Type::KeyPress);
        eventManager->resumeEventReception(vir::Event::Type::KeyRelease);
    }
    else
    {
        eventManager->pauseEventReception(vir::Event::Type::KeyPress);
        eventManager->pauseEventReception(vir::Event::Type::KeyRelease);
    }
}

//----------------------------------------------------------------------------//

void toggleMouseInputs(AppData& appData)
{
    // Mouse-related event-reception not 'really' paused as it does some 
    // important pre-processing required to possibly block input propagation 
    // to the input camera
    appData.sharedUniforms.isMouseInputEnabled = 
        !appData.sharedUniforms.isMouseInputEnabled;
}

//----------------------------------------------------------------------------//

void toggleCameraMouseInputs(AppData& appData)
{
    auto& su = appData.sharedUniforms;
    auto& camera = su.shaderCamera.dynamicUpcastTo<vir::InputCamera>();
    su.isCameraMouseInputEnabled = !su.isCameraMouseInputEnabled;
    if (su.isCameraMouseInputEnabled)
        camera->resumeEventReception(vir::Event::Type::MouseMotion);
    else
        camera->pauseEventReception(vir::Event::Type::MouseMotion);
}

//----------------------------------------------------------------------------//

void toggleCameraKeyboardInputs(AppData& appData)
{
    auto& su = appData.sharedUniforms;
    auto& camera = su.shaderCamera.dynamicUpcastTo<vir::InputCamera>();
    su.isCameraKeyboardInputEnabled = !su.isCameraKeyboardInputEnabled;
    if (su.isCameraKeyboardInputEnabled)
        camera->resumeEventReception(vir::Event::Type::KeyPress);
    else
        camera->pauseEventReception(vir::Event::Type::KeyPress);
}

//----------------------------------------------------------------------------//

void setMouseInputsClamped(AppData& appData, bool flag)
{
    auto& su = appData.sharedUniforms;
    su.isMouseInputClampedToWindow = flag;
    if (flag)
    {
        su.iMouse.x = 
            std::max(std::min(su.iMouse.x, su.iResolution.x), 0.f);
        su.iMouse.y = 
            std::max(std::min(su.iMouse.y, su.iResolution.y), 0.f);
    }
    su.toggles.updateDataRangeII = true;
}

//----------------------------------------------------------------------------//

void setMouseCaptured(AppData& appData, bool flag)
{
    auto window = vir::Window::instance();
    auto eventManager = vir::GlobalPtr<EventManager>::get();
    static std::string mouseCapturedMessage = 
        "Mouse cursor captured by window (press ESC to free)";
    if (!flag)
    {
        window->setCursorStatus(vir::Window::CursorStatus::Normal);
        StatusBar::removeMessageFromQueue(mouseCapturedMessage);
        StatusBar::queueTemporaryMessage("Mouse cursor freed");
    }
    if (flag)
    {
        auto position = 
            vir::Window::instance()->position
            (
                vir::Window::PositionOf::Center
            );
        auto pauseForOneBroadcast = []
        (
            vir::Event::Receiver* receiver, 
            vir::Event::Type type
        )
        {
            if (!receiver->isEventReceptionPaused(type))
                receiver->pauseEventReception(type, 1);
        };
        pauseForOneBroadcast(eventManager, vir::Event::Type::MouseMotion);
        pauseForOneBroadcast(eventManager, vir::Event::Type::MouseButtonPress);
        pauseForOneBroadcast(eventManager, vir::Event::Type::MouseButtonRelease);
        pauseForOneBroadcast
        (
            (vir::InputCamera*)appData.sharedUniforms.shaderCamera.get(), 
            vir::Event::Type::MouseMotion
        );
        window->setCursorStatus(vir::Window::CursorStatus::Hidden);
        vir::InputState::instance()->setMousePositionNativeOS
        (
            vir::MousePosition(position), 5
        );
        vir::Event::Broadcaster::instance()->broadcastNativeQueue();
        window->setCursorStatus(vir::Window::CursorStatus::Captured);
        vir::InputState::instance()->leftMouseButtonClickNativeOS(5);
        vir::Event::Broadcaster::instance()->broadcastNativeQueue();
        StatusBar::queueMessage(mouseCapturedMessage);
    }

}

}