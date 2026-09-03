#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/corelogic.h"
#include "shaderthing/include/helpers.h"
#include "shaderthing/include/structs.h"
#include "shaderthing/include/eventmanager.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/objectio.h"
#include "shaderthing/include/texteditor.h"
#include "shaderthing/include/statusbar.h"
#include "shaderthing/include/uniform.h"
#include "vir/include/vir.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//

void initialize(AppState& appState)
{
    static bool firstTime = true;

    // General appState reset
    appState.project = {};
    appState.renderState = {};
    Layer::resetSharedSourceEditor();
    appState.exporter.reset();
    appState.exporter = vir::makeUnique<Exporter>();
    appState.sharedStorage.reset();
    appState.sharedStorage = SharedStorage::create();
    appState.sharedUniforms.reset();
    appState.sharedUniforms = vir::makeUnique<SharedUniforms>();
    //appState.resources.clear();
    for (auto& layer : appState.layers)
    {
        removeLayerFromResources(layer->weakFromThis(), appState.resources);
    }
    appState.layers.clear();

    // Reset event manager
    if (GPtr<EventManager>::valid())
    {
        GPtr<EventManager>::reset();
    }

    // Window setup
    auto window = vir::Window::instance();
    window->setSize(512, 512);
    if (firstTime)
    {
        // Set window icon
        window->setIcon
        (
            (unsigned char*)ByteData::Icon::sTIconData,
            ByteData::Icon::sTIconSize,
            false
        );
    }

    // ImGui setup
    ImGuiIO& io = ImGui::GetIO();
    // Do not save config to .ini file
    if (firstTime)
    {
        io.IniFilename = NULL;
        io.ConfigDockingTransparentPayload = true;
    }
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
    auto& font = appState.font;
    float baseFontSize = 26.f;
    font.imFontConfig.PixelSnapH = true;
    font.imFontConfig.OversampleV = 3.0;
    font.imFontConfig.OversampleH = 3.0;
    font.imFontConfig.RasterizerMultiply = 1.0;
    // The 26-36.5 ratio between Western writing systems' characters and
    // Asian logograms/characters is set so that the latter are (almost)
    // exactly twice as wide as the former, for readability, valid for the
    // selected fonts at hand
    if (firstTime)
    {
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
    }
    font.imFont->Scale = 0.6;
    font.scale = &font.imFont->Scale;

    // Initialize shared uniforms
    initializeSharedUniforms(appState);

    // Create default layer
    createNewLayer(appState);

    // Initialize event manager
    GPtr<EventManager>(new EventManager(appState));

    if (firstTime)
        firstTime = false;
};

//----------------------------------------------------------------------------//

void initializeSharedUniforms(AppState& appState)
{
    SharedUniforms& su = *(appState.sharedUniforms);

    // Init CPU block data
    auto window = vir::Window::instance();
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

    su.iMVPUniform = Uniform::create(&su.vertex).getWeak();
    su.iMVPUniform->name() = "iMVP";
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
    su.iFrameUniform = Uniform::create(&su.fragment).getWeak();
    su.iFrameUniform->name() = "iFrame";
    su.iFrameUniform->setValuePtr
    (
        &appState.renderState.frameIndex, 
        Uniform::Type::Int
    );
    su.iFrameUniform->gui.showBounds = false;

    su.iRenderPassUniform = Uniform::create(&su.fragment).getWeak();
    su.iRenderPassUniform->name() = "iRenderPass";
    su.iRenderPassUniform->setValuePtr
    (
        &appState.renderState.passIndex, 
        Uniform::Type::Int
    );
    su.iRenderPassUniform->gui.showBounds = false;
    
    su.iTimeUniform = Uniform::create(&su.fragment).getWeak();
    su.iTimeUniform->name() = "iTime";
    su.iTimeUniform->setValuePtr(&su.iTime, Uniform::Type::Float);
    
    su.iTimeDeltaUniform = Uniform::create(&su.fragment).getWeak();
    su.iTimeDeltaUniform->name() = "iTimeDelta";
    su.iTimeDeltaUniform->setValuePtr(&su.iTimeDelta, Uniform::Type::Float);
    su.iTimeDeltaUniform->gui.showBounds = false;

    su.iRandomUniform = Uniform::create(&su.fragment).getWeak();
    su.iRandomUniform->name() = "iRandom";
    su.iRandomUniform->setValuePtr(&su.iRandom, Uniform::Type::Float);
    su.iRandomUniform->gui.showBounds = false;

    su.iUserActionUniform = Uniform::create(&su.fragment).getWeak();
    su.iUserActionUniform->name() = "iUserAction";
    su.iUserActionUniform->setValuePtr(&su.iUserAction, Uniform::Type::Bool);
    su.iUserActionUniform->gui.showBounds = false;

    su.iExportUniform = Uniform::create(&su.fragment).getWeak();
    su.iExportUniform->name() = "iExport";
    su.iExportUniform->setValuePtr
    (
        &appState.exporter->isActive, 
        Uniform::Type::Bool
    );
    su.iExportUniform->gui.showBounds = false;

    su.iWASDUniform = Uniform::create(&su.fragment).getWeak();
    su.iWASDUniform->name() = "iWASD";
    su.iWASDUniform->setValuePtr(&su.iWASD, Uniform::Type::Float3);

    su.iLookUniform = Uniform::create(&su.fragment).getWeak();
    su.iLookUniform->name() = "iLook";
    su.iLookUniform->setValuePtr(&su.iLook, Uniform::Type::Float3);
    su.iLookUniform->gui.showBounds = false;

    su.iMouseUniform = Uniform::create(&su.fragment).getWeak();
    su.iMouseUniform->name() = "iMouse";
    su.iMouseUniform->setValuePtr(&su.iMouse, Uniform::Type::Float4);
    su.iMouseUniform->gui.showBounds = false;

    su.iAspectRatioUniform = Uniform::create(&su.fragment).getWeak();
    su.iAspectRatioUniform->name() = "iWindowAspectRatio";
    su.iAspectRatioUniform->setValuePtr(&su.iAspectRatio, Uniform::Type::Float);
    su.iAspectRatioUniform->gui.showBounds = false;

    su.iResolutionUniform = Uniform::create(&su.fragment).getWeak();
    su.iResolutionUniform->name() = "iWindowResolution";
    su.iResolutionUniform->setValuePtr(&su.iResolution, Uniform::Type::Float2);
    su.iResolutionUniform->gui.showBounds = false;

    su.iKeyboardUniform = Uniform::create(&su.fragment).getWeak();
    su.iKeyboardUniform->name() = "iKeyboard";
    su.iKeyboardUniform->setValuePtr(&su.iKeyboard, Uniform::Type::Int3, 256);
    su.iKeyboardUniform->gui.showBounds = false;
}

//----------------------------------------------------------------------------//

void preRenderUpdate(AppState& appState)
{
    appState.deferredActionBuffer.process();
    // To the best of my own knowledge, this flag is only used to request
    // shader recompilation after changing the resource used by an image/sampler
    // uniform when the resource is of different signed-ness compared to the
    // pre-existing resource (i.e., chaning from usampler to sampler, or vice-
    // versa, which needs to be automatically managed). Ideally, this whole
    // things should be handled more elegantly in the CHECK_RESOURCE_SELECTED
    // macro in gui.cpp by appending this layer->compileShader() command
    // ONLY to layers that actually use the uniform that references said
    // changed resource. The main issue is that I cannot currently invoke
    // the recompilation of all layers from within in there anyway. So,
    // this whole requestFullRecompilation flag is just a work-around
    if (appState.renderState.toggles.requestFullRecompilation)
    {
        for (auto& layer : appState.layers)
        {
            layer->compileShader();
        }
        appState.renderState.toggles.requestFullRecompilation = false;
    }
}

//----------------------------------------------------------------------------//

void setWindowResolution
(
    AppState& appState, 
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

    auto& su = *(appState.sharedUniforms);

    // Store in iResolution & update aspectRatio
    su.iResolution = resolution;
    su.iAspectRatio = ((float)resolution.x)/resolution.y;

    // If not preparing for export, reset export resolution and its scale if
    // the window is resized in any way (either manullay or via the GUI). Not
    // necessary but I like this behavior better
    if (!prepareForExport)
    {
        appState.exporter->outputResolution = resolution;
        appState.exporter->outputResolutionScale = 1.f;
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

void postRenderUpdate(AppState& appState)
{
    auto& su = *(appState.sharedUniforms);
    
    // TODO
    //exporter_->update(*sharedUniforms_, layers_, resources_);

    bool advanceFrame;
    float timeStep;
    if (false)//(exporter_->isRunning())
    {
        if 
        (
            appState.renderState.passIndex == 
            appState.exporter->nRenderPasses-1
        )
        {
            advanceFrame = true;
            timeStep = appState.exporter->timeStep;
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
        if (appState.renderState.isTiledRenderingEnabled)
        {
            static float cumulatedTimeStep = 0;
            if (!appState.renderState.isPaused)
                cumulatedTimeStep += timeStep;
            if (appState.renderState.tileIndex == 0)
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
            appState.renderState.toggles.stepToNextFrame || 
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
            appState.renderState.isPaused && 
            !appState.renderState.toggles.stepToNextFrame
        )
    )
        ++appState.renderState.frameIndex;

    if (appState.renderState.toggles.resetFrameCounterPreOrPostExport)
    {
        appState.renderState.frameIndex = 0;
        appState.renderState.toggles.resetFrameCounterPreOrPostExport = false;
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

    // Compute fps and set in window title, also, check if renderState should stop
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
        if (appState.renderState.isTiledRenderingEnabled)
        {
            double wFps = fps/appState.renderState.nTiles;
            appState.controlPanelTitle = 
                "Control panel - "+appState.project.filename+" (window: "+
                Helpers::format(wFps,1)+" fps | GUI: "+
                Helpers::format(fps,1)+" fps)"+"###CP";
        }
        else
            appState.controlPanelTitle = 
                "Control panel - "+appState.project.filename+" ("+
                Helpers::format(fps,1)+" fps)"+"###CP";
        elapsedFrames = 0;
        elapsedTime = 0;
        if (!appState.exporter->isActive)
        {
            fpsUpdateCounter++;
            shouldStopRendering = 
                shouldStopRendering && fps < appState.renderState.lowerFpsLimit;
            if (fpsUpdateCounter >= int(maxLowFpsPeriod/fpsUpdatePeriod))
            {
                if (shouldStopRendering && !appState.renderState.isPaused)
                    toggleRenderingPaused(appState, true); 
                fpsUpdateCounter = 0;
                shouldStopRendering = true;
            }
        }
    }
}

//----------------------------------------------------------------------------//

void toggleRenderingPaused(AppState& appState, bool dueToLowFps)
{
    appState.renderState.isPaused = 
        !appState.renderState.isPaused;
    
    if (appState.renderState.isPaused)
    {
        appState.sharedUniforms->isTimePausedBecauseRenderingPaused = 
            !appState.sharedUniforms->isTimePaused;
        appState.sharedUniforms->isTimePaused = true;
    }
    else if 
    (
        appState.sharedUniforms->isTimePausedBecauseRenderingPaused
    )
        appState.sharedUniforms->isTimePaused = false;

    // It would be better to update the StatusBar messages elsewhere, but
    // whatever
    if (appState.renderState.isPaused)
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

void createNewLayer(AppState& appState, bool compileShader)
{
    unsigned int id = Helpers::findSmallestFreeLayerId(appState.layers);
    auto& layer = appState.layers.emplace_back(Layer::create(id, appState));
    if (compileShader)
        layer->compileShader();
    if (appState.renderState.isTiledRenderingEnabled)  
        setRenderingTiles(appState, appState.renderState.nTiles);
}

//------------------------------------------------------------------------------

void setRenderingTiles
(
    AppState& appState, 
    int nTiles
)
{
    if (appState.layers.size() == 0)
        return;
    nTiles = std::max(nTiles, 1);
    appState.renderState.isTiledRenderingEnabled = nTiles > 1;
    appState.renderState.nTilesCache = appState.renderState.nTiles;
    appState.renderState.nTiles = nTiles;
    appState.renderState.tileIndex = 0;
    double largestLayerSize = 0.0; // Mpx
    for (auto& layer : appState.layers)
    {
        double layerSize = 
            ((double)layer->resolution().x/1024.0)*
            ((double)layer->resolution().y/1024.0);
        if (layerSize > largestLayerSize)
            largestLayerSize = layerSize;
    }
    for (auto& layer : appState.layers)
    {
        double layerSize = 
            ((double)layer->resolution().x/1024.0)*
            ((double)layer->resolution().y/1024.0);
        unsigned int nt = 
            std::max
            (
                (unsigned int)nTiles*
                (unsigned int)(layerSize/largestLayerSize),
                1u
            );
        layer->setRenderingTilesNumber(nt);
    }
}

//----------------------------------------------------------------------------//

RenderResult renderShaders
(
    AppState& appState,
    vir::Framebuffer* target, 
    const unsigned int nRenderPasses
)
{
    auto& sharedUniforms = *(appState.sharedUniforms);
    static bool clearTarget = true;
    // TODO Fix behavior of stepping to next frame when tiled renderState is
    // enabled
    bool renderFrame = 
        !appState.renderState.isPaused || 
        appState.renderState.toggles.stepToNextFrame;
    bool frameRendered = true;
    unsigned int iRenderPass = appState.renderState.passIndex;

    if (renderFrame)
    {
        if (target != nullptr && iRenderPass == 0) // I.e., if exporting
        {
            for (auto& layer : appState.layers) // Apply clear policy
            {
                switch (layer->exportSettings.clearPolicy)
                {
                case Layer::ExportSettings::FramebufferClearPolicy::
                    None :
                    continue;
                case Layer::ExportSettings::FramebufferClearPolicy::
                    ClearOnFirstFrameExport:
                    if (appState.renderState.passIndex == 0)
                        layer->clearFramebuffers();
                    break;
                case Layer::ExportSettings::FramebufferClearPolicy::
                    ClearOnEveryFrameExport:
                    if (iRenderPass == 0)
                        layer->clearFramebuffers();
                    break;
                }
            }
        }

        clearTarget = true;
        for (auto& layer : appState.layers)
        {
            layer->renderShader(target, clearTarget);
            // At the end of this loop, the status of clearTarget will 
            // represent whether the main window has been cleared of its 
            // contents at least once (true if NOT cleared at least once)
            if 
            (
                clearTarget &&
                layer->renderingTarget() != 
                    Layer::RenderState::Target::InternalFramebuffer
            )
                clearTarget = false;
        }

        bool nextRenderPass = false;
        // If tiled renderState is enabled, it means that the previous render loop
        // has rendered only the i-th tile of each layer (in pratice, this is 
        // achieved by renderState over a quad that covers only a portion of the
        // renderState target). Here, the index of the tile to be rendered is
        // advanced. The frame is considered fully rendered only if all tiles
        // have been rendered. During exports, tiled renderState is automatically
        // disabled in the exporter setup phase
        if (appState.renderState.isTiledRenderingEnabled)
        {
            if 
            (
                ++appState.renderState.tileIndex == 
                appState.renderState.nTiles
            )
            {
                appState.renderState.tileIndex = 0;
                nextRenderPass = true;
            }
            else
                frameRendered = false;
        }
        else
            nextRenderPass = true;
        
        if (nextRenderPass)
        {
            if (appState.renderState.passIndex < nRenderPasses-1)
                ++appState.renderState.passIndex;
            else
                appState.renderState.passIndex = 0;
            sharedUniforms.fragment.uniformBuffer->markUniformForSubmission
            (
                sharedUniforms.iRenderPassUniform.get()
            );
        }
    }
    else
        frameRendered = false;

    // If the window has not been cleared at least once, or if I am not
    // renderState to the window at all (i.e., if renderTarget != nullptr, which 
    // is only true during exports), then render a dummy/void/blank window, 
    // simply to avoid visual artifacts when nothing is renderState to the main
    // window. Both the blank shader and quad are managed statically here, for
    // convenience
    if (frameRendered && (clearTarget || target != nullptr))
    {
        static std::unique_ptr<vir::Quad> blankQuad(new vir::Quad(1, 1, 0));
        auto viewport = Helpers::normalizedWindowResolution();
        blankQuad->update(viewport.x, viewport.y, 0);
        auto constructBlankShader = [&appState]()
        {
            auto shader = 
                vir::Shader::create
                (
                    Layer::vertexShaderSource(*appState.sharedUniforms),
                    vir::Shader::currentContextShadingLanguageDirectives() +
R"(out vec4 fragColor;
in     vec2 qc;
in     vec2 tc;
void main(){fragColor = vec4(0, 0, 0, .5);})",
                    vir::Shader::ConstructFrom::SourceCode
                );
            shader->bindUniformBlock
            (
                appState.sharedUniforms->fragment.uniformBuffer->name(),
                appState.sharedUniforms->fragment.uniformBufferBindingPoint
            );
            shader->bindUniformBlock
            (
                appState.sharedUniforms->vertex.uniformBuffer->name(),
                appState.sharedUniforms->vertex.uniformBufferBindingPoint
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

void addLayerToResources(WPtr<Layer> layer, UPtrVector<Resource>& resources)
{
    for (int i=0; i<(int)resources.size(); i++)
    {
        UPtr<Resource>& resource = resources[i];
        if (resource->type() != Resource::Type::Framebuffer)
            continue;
        if (resource->name() == layer->name())
            return;
    }
    UPtr<Resource>& resource = 
        resources.emplace_back(LayerResource::create(layer));
    resource->setName(layer->namePtr());
}

//----------------------------------------------------------------------------//

void removeLayerFromResources
(
    WPtr<Layer> layer, 
    UPtrVector<Resource>& resources
)
{
    for (int i=0; i<(int)resources.size(); i++)
    {
        UPtr<Resource>& resource = resources[i];
        if (resource->type() != Resource::Type::Framebuffer)
            continue;
        if (resource->name() == layer->name())
        {
            resources.erase(resources.begin()+i);
            return;
        }
    }
}

//----------------------------------------------------------------------------//

void toggleKeyboardInputs(AppState& appState)
{
    appState.sharedUniforms->isKeyboardInputEnabled = 
        !appState.sharedUniforms->isKeyboardInputEnabled;
    auto eventManager = GPtr<EventManager>::get();
    if (appState.sharedUniforms->isKeyboardInputEnabled)
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

void toggleMouseInputs(AppState& appState)
{
    // Mouse-related event-reception not 'really' paused as it does some 
    // important pre-processing required to possibly block input propagation 
    // to the input camera
    appState.sharedUniforms->isMouseInputEnabled = 
        !appState.sharedUniforms->isMouseInputEnabled;
}

//----------------------------------------------------------------------------//

void toggleCameraMouseInputs(AppState& appState)
{
    auto& su = *(appState.sharedUniforms);
    auto& camera = su.shaderCamera.dynamicDowncastTo<vir::InputCamera>();
    su.isCameraMouseInputEnabled = !su.isCameraMouseInputEnabled;
    if (su.isCameraMouseInputEnabled)
        camera->resumeEventReception(vir::Event::Type::MouseMotion);
    else
        camera->pauseEventReception(vir::Event::Type::MouseMotion);
}

//----------------------------------------------------------------------------//

void toggleCameraKeyboardInputs(AppState& appState)
{
    auto& su = *(appState.sharedUniforms);
    auto& camera = su.shaderCamera.dynamicDowncastTo<vir::InputCamera>();
    su.isCameraKeyboardInputEnabled = !su.isCameraKeyboardInputEnabled;
    if (su.isCameraKeyboardInputEnabled)
        camera->resumeEventReception(vir::Event::Type::KeyPress);
    else
        camera->pauseEventReception(vir::Event::Type::KeyPress);
}

//----------------------------------------------------------------------------//

void setMouseInputsClamped(AppState& appState, bool flag)
{
    auto& su = *(appState.sharedUniforms);
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

void setMouseCaptured(AppState& appState, bool flag)
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
            (vir::InputCamera*)appState.sharedUniforms->shaderCamera.get(), 
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

//----------------------------------------------------------------------------//

void saveTo
(
    AppState& appState, 
    const std::string& filepath, 
    bool triggeredByAutosave
)
{
    auto io = ObjectIO(filepath.c_str(), ObjectIO::Mode::Write);
    
    // Objects are written maintaining the same save file structure as the old
    // (i.e., v.0/1.x.x) ShaderThing to maintain compatibility

    io.write("UIScale", *appState.font.scale);
    io.write("autoSaveEnabled", appState.project.isAutoSaveEnabled);
    io.write("autoSaveInterval", appState.project.autoSaveInterval);
    io.write("vSyncEnabled", appState.renderState.isVSyncEnabled);
    //
    io.writeObjectStart("resources");
    for (auto& resource : appState.resources)
        resource->saveTo(io);
    io.writeObjectEnd();
    //
    auto& su = *(appState.sharedUniforms);
    io.writeObjectStart("sharedUniforms");
    io.write("windowResolution", su.iResolution);
    // TODO
    // io.write("exportWindowResolutionScale", su.exportData.resolutionScale);
    io.write("time", su.iTime);
    io.write("timePaused", 
        su.isTimePaused && su.isTimePausedBecauseRenderingPaused ? 
        false : su.isTimePaused);
    io.write("timeLooped", su.isTimeLooped);
    io.write("timeBounds", su.iTimeUniform->gui.bounds);
    io.write("randomGeneratorPaused", su.isRandomNumberGeneratorPaused);
    io.write("iWASD", su.shaderCamera->position());
    io.write("iWASDSensitivity", su.shaderCamera->keySensitivityRef());
    io.write("iWASDInputEnabled", su.isCameraKeyboardInputEnabled);
    io.write("iLook", su.shaderCamera->z());
    io.write("iLookSensitivity", su.shaderCamera->mouseSensitivityRef());
    io.write("iLookInputEnabled", su.isCameraMouseInputEnabled);
    io.write("iLookInputRequiresLMBHold", su.cameraMouseInputRequiresLMBHold);
    io.write("iMouseInputEnabled", su.isMouseInputEnabled);
    io.write("iMouseInputClampedToWindow", su.isMouseInputClampedToWindow);
    io.write("mouseInputRequiresLMBHold", su.mouseInputRequiresLMBHold);
    io.write("iKeyboardInputEnabled", su.isKeyboardInputEnabled);
    io.write("smoothTimeDelta", su.isTimeDeltaSmooth);
    io.write("resetTimeOnFrameCounterReset", su.isTimeResetOnFrameCounterReset);
    io.write("cursorStatus", 
        vir::Window::instance()->cursorStatus() == 
        vir::Window::CursorStatus::Captured);
    // Write custom user-made shared uniforms
    io.writeObjectStart("uniforms");
    for 
    (
        unsigned int i=su.userUniformsStartIndex; 
                     i<su.fragment.uniforms.size(); 
                     i++)
    {
        su.fragment.uniforms[i]->saveTo(io);
    }
    io.writeObjectEnd(); // End of uniforms
    io.writeObjectEnd(); // End of sharedUniforms
    //
    io.writeObjectStart("layers");
    for (auto& layer : appState.layers)
    {
        layer->saveTo(io);
    }
    io.writeObjectEnd(); // End of layers

    /*
    // Resource::       saveAll(resources_, project);
    // sharedUniforms_->save   (            project);
    // Layer::          saveAll(layers_,    project);
    exporter_->      save   (            project);
    PostProcess::    saveStaticData(     project);
    */
   
    // TODO Could display an error via ImGui on failure
    io.writeContentsToDisk();

    appState.project.timeSinceLastSave = 0;

    StatusBar::queueTemporaryMessage
    (
        triggeredByAutosave ? "Project auto-saved" : "Project saved",
        StatusBar::defaultMessageDuration,
        0xff25ff50
    );
}

//----------------------------------------------------------------------------//

void loadFrom
(
    AppState& appState,
    const std::string& filepathOrData,
    bool fromMemory
)
{
    /*
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
    */

    auto project = 
        fromMemory ? 
        ObjectIO(filepathOrData) : 
        ObjectIO(filepathOrData.c_str(), ObjectIO::Mode::Read);
    if (!project.isValid())
        return; // TODO Could display an error via ImGui

    *appState.font.scale = project.read<float>("UIScale");
    appState.project.isAutoSaveEnabled = project.readOrDefault<bool>
    (
        "autoSaveEnabled", 
        Project{}.isAutoSaveEnabled
    );
    appState.project.autoSaveInterval = project.readOrDefault<float>
    (
        "autoSaveInterval", 
        Project{}.autoSaveInterval
    );
    appState.renderState.isVSyncEnabled = 
        project.readOrDefault<bool>("vSyncEnabled", true);
    vir::Window::instance()->setVSync(appState.renderState.isVSyncEnabled);

    // Load Resources ----------------------------------------------------------
    
    appState.resources.clear();
    auto ioType = [](const ObjectIO& io)
    {
        auto typeName = io.read("type", false);
        for (auto entry : Resource::typeToName)
        {
            if (std::string(typeName) != std::string(entry.second))
                continue;
            return entry.first;
        }
        return Resource::Type();
    };
    auto ioResources = project.readObject("resources");

    // First, load all resources of type Texture2D as these might be referenced
    // by AnimatedTexture2D or Cubemap resources
    for (auto ioResourceName : ioResources.members())
    {
        auto ioResource = ioResources.readObject(ioResourceName);
        auto type = ioType(ioResource);
        if (type != Resource::Type::Texture2D)
            continue;
        appState.resources.emplace_back
        (
            Texture2DResource::loadFrom(ioResource)
        );
    }
    // Then load all the remaining resources
    for (auto ioResourceName : ioResources.members())
    {
        auto ioResource = ioResources.readObject(ioResourceName);
        auto type = ioType(ioResource);
        switch (type)
        {
            default :
                continue;
            case Resource::Type::Texture3D :
                appState.resources.emplace_back
                (
                    Texture3DResource::loadFrom(ioResource)
                );
                break;
            case Resource::Type::AnimatedTexture2D :
                appState.resources.emplace_back
                (
                    AnimatedTexture2DResource::loadFrom
                    (
                        ioResource, 
                        appState.resources
                    )
                );
                break;
            case Resource::Type::Cubemap :
                appState.resources.emplace_back
                (
                    CubemapResource::loadFrom(ioResource, appState.resources)
                );
                break;
        }
    }

    // Load SharedUniforms -----------------------------------------------------

    appState.sharedUniforms.reset();
    appState.sharedUniforms = vir::makeUnique<SharedUniforms>();
    initializeSharedUniforms(appState);
    auto su = appState.sharedUniforms.get();
    auto ioSu = project.readObject("sharedUniforms");
    auto resolution = (glm::ivec2)ioSu.read<glm::vec2>("windowResolution");
    setWindowResolution(appState, resolution, false);
    su->iTime = ioSu.read<float>("time");
    //su->resetFrameCounter = false;
    su->iTimeUniform->gui.bounds = ioSu.read<glm::vec2>("timeBounds");
    su->iWASD = ioSu.read<glm::vec3>("iWASD");
    su->iLook = ioSu.read<glm::vec3>("iLook");
    su->isTimePaused = ioSu.read<bool>("timePaused");
    su->isTimeLooped = ioSu.read<bool>("timeLooped");
    su->isTimeDeltaSmooth = 
        ioSu.readOrDefault<bool>("smoothTimeDelta", false);
    su->isTimeResetOnFrameCounterReset = 
        ioSu.read<bool>("resetTimeOnFrameCounterReset");
    su->isRandomNumberGeneratorPaused = 
        ioSu.readOrDefault<bool>("randomGeneratorPaused", false);
    su->shaderCamera->setDirection(su->iLook);
    su->shaderCamera->setPosition(su->iWASD);
    su->shaderCamera->setKeySensitivity(ioSu.read<float>("iWASDSensitivity"));
    su->shaderCamera->setMouseSensitivity(ioSu.read<float>("iLookSensitivity"));
    su->shaderCamera->update();
    if (!ioSu.read<bool>("iWASDInputEnabled"))
        toggleCameraKeyboardInputs(appState);
    if (!ioSu.read<bool>("iLookInputEnabled"))
        toggleCameraMouseInputs(appState);
    su->cameraMouseInputRequiresLMBHold = 
        ioSu.readOrDefault<bool>("iLookInputRequiresLMBHold", true);
    if (!ioSu.read<bool>("iMouseInputEnabled"))
        toggleMouseInputs(appState);
    setMouseInputsClamped
    (
        appState,
        ioSu.readOrDefault<bool>("iMouseInputClampedToWindow", false)
    );
    su->mouseInputRequiresLMBHold = 
        ioSu.readOrDefault<bool>("mouseInputRequiresLMBHold", true);
    if (!ioSu.read<bool>("iKeyboardInputEnabled"))
        toggleKeyboardInputs(appState);
    
    // TODO Reimplement
    /*
    su->exportData_.resolutionScale = 
        ioSu.read<float>("exportWindowResolutionScale");
    su->exportData_.resolution = 
        (glm::vec2)su->iResolution_*
        su->exportData_.resolutionScale + .5f;
    */
    if (ioSu.readOrDefault<bool>("cursorStatus", false))
        setMouseCaptured(appState, true);
    std::map<Uniform*, std::string> uninitializedSharedResourceLayers = {};
    Uniform::loadAllFrom
    (
        ioSu,
        &su->fragment,
        appState.resources,
        uninitializedSharedResourceLayers
    );
    su->toggles.updateDataRangeII = true;

    // Load Layers -------------------------------------------------------------

    // Clear state
    appState.layers.clear();

    if (project.hasMember("graphicsExtensions"))
    {
        std::vector<std::string> enabledExtensions = 
            project.read<std::vector<std::string>>("graphicsExtensions");
        for (auto& extension : enabledExtensions)
            vir::Shader::
            setExtensionStatusInCurrentContextShadingLanguageDirectives
            (
                extension, 
                true
            );
    }

    if (project.hasMember("sharedFragmentSource"))
        Layer::resetSharedSourceEditor
        (
            project.read("sharedFragmentSource", false)
        );
    else
        Layer::resetSharedSourceEditor();

    appState.sharedStorage = SharedStorage::loadFrom(project);
    
    auto ioLayers = project.readObject("layers");
    unsigned int layerId = 0;
    for (auto ioLayerName : ioLayers.members())
    {   
        auto ioLayer = ioLayers.readObject(ioLayerName);
        appState.layers.emplace_back
        (
            Layer::loadFrom(ioLayer, layerId++, appState)
        );
    };

    // Some layers might use other layers as resource uniforms. However, if
    // layer I using layer J as a resource uniform is created before layer J,
    // the uniform value (i.e., the ptr to the resource-type layer) cannot be
    // initialized because J has not been created yet. This bookkeping is
    // done within Layer::loadFrom(), and it populates 
    // layer->cache.uninitializedResourceLayers. After all layers have been
    // loaded, the proceed with re-establishing dependencies for resource
    // uniforms that consist of layers.
    for (auto& layer : appState.layers)
    {
        for (auto& entry : layer->cache.uninitializedResourceLayers)
        {
            auto* uniform = entry.first;
            auto& layerName = entry.second;
            for (auto& resource : appState.resources)
            {
                if (resource->name() != layerName)
                    continue;
                uniform->setResourcePtr(resource);
            }
        }
        layer->cache.uninitializedResourceLayers.clear();
    }

    // Repeat for shared resource layers
    for (auto& entry : uninitializedSharedResourceLayers)
    {
        auto* uniform = entry.first;
        auto& layerName = entry.second;
        for (auto& resource : appState.resources)
        {
            if (resource->name() != layerName)
                continue;
            uniform->setResourcePtr(resource);
        }
    }
    uninitializedSharedResourceLayers.clear();

    // Reset the lists of layers using each image- or sampler-type resource
    // within each resource
    for (auto& layer : appState.layers)
    {
        for (auto& uniform : layer->uniforms)
        {
            if 
            (
                uniform->type() == Uniform::Type::Sampler2D ||
                uniform->type() == Uniform::Type::Sampler3D ||
                uniform->type() == Uniform::Type::SamplerCube ||
                uniform->type() == Uniform::Type::Image2D ||
                uniform->type() == Uniform::Type::Image3D ||
                uniform->type() == Uniform::Type::ImageCube
            )
            {
                auto resource = uniform->getValuePtr<Resource>();
                if (resource != nullptr)
                {
                    if (!resource->isUsedByUniform(uniform.get()))
                        resource->addClientUniform(uniform.get());
                }
            }
        }
    }

    // Compile all layer shaders after dependencies have been re-established
    for (auto& layer : appState.layers)
    {
        // If the project was saved in a state such that the shader has 
        // compilation errors, then initialize the shader with the blank shader 
        // source (back-end-only, the user will still see the source of the 
        // saved shader with the  usual list of compilation errors and markers).
        // This is what the last flag is for
        layer->compileShader(true);
    }

    for (auto& u : appState.sharedUniforms->fragment.uniforms)
    {
        u->markForSubmissionToAllClientBuffers();
    }
    for (auto& layer : appState.layers)
    {
        for (auto& u : layer->uniforms)
        {
            u->markForSubmissionToAllClientBuffers();
        }
    }

    // Finally, reset tiled rendering
    unsigned int nTiles = 
        project.readOrDefault<unsigned int>("nRenderingTiles", 1);
    setRenderingTiles(appState, nTiles);

    /* TODO Reimplement
    Exporter::      load   (project, exporter_                            );
    PostProcess::   loadStaticData(project);
    */
}

//----------------------------------------------------------------------------//

void updateLayersDueToUniformTypeOrNameChanged
(
    UPtr<Uniform>& uniform, 
    AppState& appState,
    bool recompileShaders
)
{
    auto updateLayer = [&uniform, &appState, &recompileShaders]
    (
        Layer* layer
    )
    {
        if 
        (
            std::find // I.e., if not already in uncompiledUniforms
            (
                layer->cache.uncompiledUniforms.begin(), 
                layer->cache.uncompiledUniforms.end(), 
                uniform.get()
            ) == layer->cache.uncompiledUniforms.end()
        )
            layer->cache.uncompiledUniforms.emplace_back(uniform.getWeak());
        for (auto& u : layer->cache.uncompiledUniforms)
        {
            if (u->name().size() == 0)
                continue;
            layer->hasUncompiledEdits = true;
            if (recompileShaders)
                layer->compileShader();
            break;
        }
    };
    if (uniform->isSharedByUser)
    {
        for (auto& layer : appState.layers)
        {
            updateLayer(layer.get());
        }
    }
    else
    {
        updateLayer(dynamic_cast<Layer*>(uniform->owner()));
    }
}

void updateLayersDueToUniformDeletion
(
    UPtr<Uniform>& uniform, 
    AppState& appState
)
{
    // TODO Check if setting hasUncompiledEdits within the deferred action does
    // not change anything, and if so, make the code more compact
    if (uniform->isSharedByUser)
    {
        for (auto& layer : appState.layers)
        {
            layer->hasUncompiledEdits = true;
        }
        appState.deferredActionBuffer.add
        (
            [&uniform, &appState]()
            {
                uniform->deleteSelf();
                for (auto& layer : appState.layers)
                {
                    layer->compileShader();
                }
            }
        );
    }
    else
    {
        auto layer = dynamic_cast<Layer*>(uniform->owner());
        layer->hasUncompiledEdits = true;
        appState.deferredActionBuffer.add
        (
            [&uniform, layer]()
            {
                uniform->deleteSelf();
                layer->compileShader();
            }
        );
    }
}

}