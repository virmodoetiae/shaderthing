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

#include "shaderthing/include/shareduniforms.h"

#include "shaderthing/include/macros.h"
#include "shaderthing/include/objectio.h"
#include "shaderthing/include/random.h"
#include "shaderthing/include/resource.h"
#include "shaderthing/include/statusbar.h"
#include "shaderthing/include/uniform.h"

#include "vir/include/vir.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//

SharedUniforms::SharedUniforms()
{
    // Init CPU block data
    static const auto window = vir::Window::instance();
    if (!window->iconified())
       fBlock_.iResolution = {window->width(), window->height()};
    fBlock_.iAspectRatio = fBlock_.iResolution.x/fBlock_.iResolution.y;
    for (int i=0; i<256; i++)
        fBlock_.iKeyboard[i] = glm::ivec3({0,0,0});

    // Init cameras
    if (screenCamera_ == nullptr)
        screenCamera_ = vir::Camera::create<vir::Camera>();
    if (shaderCamera_ == nullptr)
        shaderCamera_ = vir::Camera::create<vir::InputCamera>();
    screenCamera_->setProjectionType
    (
        vir::Camera::ProjectionType::Orthographic
    );
    screenCamera_->setViewportHeight
    (
        std::min(1.0f, 1.0f/fBlock_.iAspectRatio)
    );
    screenCamera_->setPosition({0, 0, 1});
    screenCamera_->setPlanes(.01f, 100.f);
    shaderCamera_->setZPlusIsLookDirection(true);
    shaderCamera_->setDirection(fBlock_.iLook.packed());
    shaderCamera_->setPosition(fBlock_.iWASD.packed());
    screenCamera_->update();
    shaderCamera_->update();
    vBlock_.iMVP = 
        screenCamera_->projectionViewMatrix();

    // Init random number generator and set initial random number
    if (random_ == nullptr)
        random_ = new Random();
    fBlock_.iRandom = random_->generateFloat();

    // Init uniform buffers, bind to designated binding points and set
    // initial data
    if (vBuffer_ == nullptr)
        vBuffer_ = 
            vir::UniformBuffer::create(VertexBlock::size());
    vBuffer_->bind();
    vBuffer_->setBindingPoint(vBindingPoint_);
    vBuffer_->setData
    (
        &(vBlock_),
        VertexBlock::size(),
        0
    );
    if (fBuffer_ == nullptr)
        fBuffer_ = 
            vir::UniformBuffer::create(FragmentBlock::size());
    fBuffer_->bind();
    fBuffer_->setBindingPoint(fBindingPoint_);
    fBuffer_->setData
    (
        &(fBlock_),
        FragmentBlock::size(),
        0
    );

    // Init bounds
    bounds_.insert({Uniform::SpecialType::Time, {0, 1}});
    bounds_.insert({Uniform::SpecialType::CameraPosition, {0, 1}});

    exportData_.resolution = fBlock_.iResolution;

    // Set VSync
    vir::Window::instance()->setVSync(flags_.isVSyncEnabled);

    // Register the class iteself with the vir event broadcaster with a 
    // higher priority (lower value is higher priority) than all other
    // ShaderThing event receivers
    this->tuneIntoEventBroadcaster(VIR_DEFAULT_PRIORITY-1);

    // Make SharedUniforms receive MouseMotion event before the vir::InputCamera
    // itself. This is necessary to enable the functionality controlled by the
    // cameraMouseInputRequiresLMBHold flag
    this->setEventReceiverPriority
    (
        vir::Event::Type::MouseMotion, 
        VIR_CAMERA_PRIORITY-1
    );

    this->setEventReceiverPriority
    (
        vir::Event::Type::MouseButtonPress, 
        VIR_IMGUI_PRIORITY-1
    );
    this->setEventReceiverPriority
    (
        vir::Event::Type::MouseButtonRelease, 
        VIR_IMGUI_PRIORITY-1
    );
    this->setEventReceiverPriority
    (
        vir::Event::Type::KeyPress, 
        VIR_IMGUI_PRIORITY-1
    );
    this->setEventReceiverPriority
    (
        vir::Event::Type::KeyRelease, 
        VIR_IMGUI_PRIORITY-1
    );
}

//----------------------------------------------------------------------------//

SharedUniforms::~SharedUniforms()
{
    DELETE_IF_NOT_NULLPTR(fBuffer_)
    DELETE_IF_NOT_NULLPTR(vBuffer_)
    DELETE_IF_NOT_NULLPTR(screenCamera_)
    DELETE_IF_NOT_NULLPTR(shaderCamera_)
    DELETE_IF_NOT_NULLPTR(random_)
}

//----------------------------------------------------------------------------//

void SharedUniforms::setResolution
(
    glm::ivec2& resolution, 
    bool windowFrameManuallyDragged,
    bool prepareForExport
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

    // Store in iResolution & update aspectRatio
    fBlock_.iResolution = resolution;
    fBlock_.iAspectRatio = ((float)resolution.x)/resolution.y;

    // If not preparing for export, reset export resolution and its scale if
    // the window is resized in any way (either manullay or via the GUI). Not
    // necessary but I like this behavior better
    if (!prepareForExport)
    {
        exportData_.resolution = fBlock_.iResolution;
        exportData_.resolutionScale = 1.f;
    }

    // Update screen camera
    screenCamera_->setViewportHeight
    (
        std::min(1.0f, 1.0f/fBlock_.iAspectRatio)
    );
    screenCamera_->update();
    vBlock_.iMVP = screenCamera_->projectionViewMatrix();
    
    // Let's try updating these instantly
    fBuffer_->setData(&fBlock_, FragmentBlock::dataRangeIIISize(), 0);
    vBuffer_->bind();
    vBuffer_->setData(&vBlock_, VertexBlock::size(), 0);
    fBuffer_->bind();

    // Set the actual window resolution and propagate event if not preparing
    // for export
    if (!prepareForExport && !windowFrameManuallyDragged)
        window->setSize
        (
            resolution.x,
            resolution.y
        );
}

void SharedUniforms::setMouseInputsClamped(bool flag)
{
    flags_.isMouseInputClampedToWindow = flag;
    if (flag)
    {
        fBlock_.iMouse.x = 
            std::max(std::min(fBlock_.iMouse.x, fBlock_.iResolution.x), 0.f);
        fBlock_.iMouse.y = 
            std::max(std::min(fBlock_.iMouse.y, fBlock_.iResolution.y), 0.f);
    }
    flags_.updateDataRangeII = true;
}

//----------------------------------------------------------------------------//

void SharedUniforms::setUserAction(bool flag)
{
    fBlock_.iUserAction = int(flag);
    flags_.updateDataRangeII = true;
}

//----------------------------------------------------------------------------//

void SharedUniforms::toggleMouseInputs()
{
    flags_.isMouseInputEnabled = !flags_.isMouseInputEnabled;
    // Mouse-related event-reception not 'really' paused as it does some 
    // important pre-processing required to possibly block input propagation 
    // to the input camera
}

//----------------------------------------------------------------------------//

void SharedUniforms::toggleKeyboardInputs()
{
    flags_.isKeyboardInputEnabled = !flags_.isKeyboardInputEnabled;
    if (flags_.isKeyboardInputEnabled)
    {
        resumeEventReception(vir::Event::Type::KeyPress);
        resumeEventReception(vir::Event::Type::KeyRelease);
    }
    else
    {
        pauseEventReception(vir::Event::Type::KeyPress);
        pauseEventReception(vir::Event::Type::KeyRelease);
    }
}

//----------------------------------------------------------------------------//

void SharedUniforms::toggleCameraKeyboardInputs()
{
    auto camera = (vir::InputCamera*)shaderCamera_;
    flags_.isCameraKeyboardInputEnabled = !flags_.isCameraKeyboardInputEnabled;
    if (flags_.isCameraKeyboardInputEnabled)
        camera->resumeEventReception(vir::Event::Type::KeyPress);
    else
        camera->pauseEventReception(vir::Event::Type::KeyPress);
}

//----------------------------------------------------------------------------//

void SharedUniforms::toggleCameraMouseInputs()
{
    auto camera = (vir::InputCamera*)shaderCamera_;
    flags_.isCameraMouseInputEnabled = !flags_.isCameraMouseInputEnabled;
    if (flags_.isCameraMouseInputEnabled)
        camera->resumeEventReception(vir::Event::Type::MouseMotion);
    else
        camera->pauseEventReception(vir::Event::Type::MouseMotion);
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::WindowResizeEvent& event)
{
    if (event.width == 0 || event.height == 0)
    {
        event.handled = true;
        return;
    }
    glm::ivec2 resolution{event.width, event.height};
    setResolution(resolution, true);
    event.width = resolution.x;
    event.height = resolution.y;
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::MouseButtonPressEvent& event)
{
    if (!flags_.isCameraMouseInputEnabled)
        event.handled = true; // Prevent propagation to vir::InputCamera
    if (!flags_.isMouseInputEnabled)
        return;
    glm::vec4 mouse = 
    {
        event.x,
        fBlock_.iResolution.y-event.y,
        fBlock_.iMouse.x,
        -fBlock_.iMouse.y
    };
    if (flags_.isMouseInputClampedToWindow)
    {
        mouse.x = std::max(std::min(mouse.x, fBlock_.iResolution.x), 0.f);
        mouse.y = std::max(std::min(mouse.y, fBlock_.iResolution.y), 0.f);
    }
    if (fBlock_.iMouse == mouse)
        return;
    fBlock_.iUserAction = true;
    fBlock_.iMouse = mouse;
    flags_.updateDataRangeII = true;
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::MouseMotionEvent& event)
{
    bool LMBClicked = 
        vir::InputState::instance()->mouseButtonState(VIR_MOUSE_BUTTON_1)
        .isClicked();
    if 
    (
        !flags_.isCameraMouseInputEnabled || 
        (flags_.cameraMouseInputRequiresLMBHold && !LMBClicked)
    )
        event.handled = true; // Prevent propagation to vir::InputCamera
    if 
    (
        !flags_.isMouseInputEnabled || 
        (flags_.mouseInputRequiresLMBHold && !LMBClicked)
    )
        return;
    glm::vec4 mouse = 
    {
        event.x,
        fBlock_.iResolution.y-event.y,
        fBlock_.iMouse.z,
        fBlock_.iMouse.w
    };
    if (flags_.isMouseInputClampedToWindow)
    {
        mouse.x = std::max(std::min(mouse.x, fBlock_.iResolution.x), 0.f);
        mouse.y = std::max(std::min(mouse.y, fBlock_.iResolution.y), 0.f);
    }
    if (fBlock_.iMouse == mouse)
        return;
    fBlock_.iUserAction = true;
    fBlock_.iMouse = mouse;
    flags_.updateDataRangeII = true;
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::MouseButtonReleaseEvent& event)
{
    if (!flags_.isCameraMouseInputEnabled)
        event.handled = true; // Prevent propagation to vir::InputCamera
    if (!flags_.isMouseInputEnabled)
        return;
    glm::vec4 mouse = 
    {
        fBlock_.iMouse.x,
        fBlock_.iMouse.y,
        fBlock_.iMouse.z*-1,
        fBlock_.iMouse.w
    };
    if (fBlock_.iMouse == mouse)
        return;
    fBlock_.iUserAction = true;
    fBlock_.iMouse.z = mouse.z;
    flags_.updateDataRangeII = true;
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::KeyPressEvent& event)
{
    if 
    (
        event.keyCode == VIR_KEY_ESCAPE &&
        vir::Window::instance()->cursorStatus() == 
            vir::Window::CursorStatus::Captured
    ) // Un-capture mouse on ESC
    {
        setMouseCaptured(false);
    }
    FragmentBlock::ivec3A16& data(fBlock_.iKeyboard[event.keyCode]);
    static auto* inputState = vir::InputState::instance();
    auto& status = inputState->keyState(event.keyCode);
    data.x = (int)status.isPressed();
    data.y = (int)status.isHeld();
    data.z = (int)status.isToggled();
    // Only exception where I set the data immediately in the event callback in
    // order to avoid having to update the whole 4kB of key memory all at once
    // at every SharedUniforms::update call
    int offset = FragmentBlock::iKeyboardKeyOffset
    (
        vir::inputKeyCodeVirToShaderToy(event.keyCode)
    );
    fBuffer_->setData
    (
        (void*)&data, 
        FragmentBlock::iKeyboardKeySize(), 
        offset
    );
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::KeyReleaseEvent& event)
{
    FragmentBlock::ivec3A16& data(fBlock_.iKeyboard[event.keyCode]);
    static auto* inputState = vir::InputState::instance();
    data.x = 0;
    data.y = 0;
    data.z = (int)inputState->keyState(event.keyCode).isToggled();
    // Only exception where I set the data immediately in the event callback in
    // order to avoid having to update the whole 4kB of key memory all at once
    // at every SharedUniforms::update call
    fBuffer_->setData
    (
        (void*)&data, 
        FragmentBlock::iKeyboardKeySize(), 
        FragmentBlock::iKeyboardKeyOffset
        (
            vir::inputKeyCodeVirToShaderToy(event.keyCode)
        )
    );
}

//----------------------------------------------------------------------------//

void SharedUniforms::bindShader(vir::Shader* shader) const
{
    shader->bindUniformBlock
    (
        FragmentBlock::glslName,
        fBindingPoint_
    );
    shader->bindUniformBlock
    (
        VertexBlock::glslName,
        vBindingPoint_
    );
}

//----------------------------------------------------------------------------//

void SharedUniforms::update(const UpdateArgs& args)
{
    if (!flags_.isTimePaused)
    {
        fBlock_.iTime += args.timeStep;
        fBlock_.iTimeDelta = args.timeStep;
    }
    else if (flags_.stepToNextFrame || flags_.stepToNextTimeStep)
        fBlock_.iTime += fBlock_.iTimeDelta;

    const glm::vec2& timeLoopBounds(bounds_[Uniform::SpecialType::Time]);
    if (flags_.isTimeLooped && fBlock_.iTime >= timeLoopBounds.y)
    {
        auto duration = timeLoopBounds.y-timeLoopBounds.x;
        auto fraction = 
            (fBlock_.iTime-timeLoopBounds.y)/std::max(duration, 1e-6f);
        fraction -= (int)fraction;
        fBlock_.iTime = timeLoopBounds.x + duration*fraction;
    }
    
    if (args.advanceFrame && !(flags_.isRenderingPaused && !flags_.stepToNextFrame))
        ++fBlock_.iFrame;
    // fBlock_.iRenderPass = 0;

    flags_.stepToNextFrame = false;
    flags_.stepToNextTimeStep = false;
    if (flags_.resetFrameCounterPreOrPostExport)
    {
        fBlock_.iFrame = 0;
        flags_.resetFrameCounterPreOrPostExport = false;
    }
    if (flags_.resetFrameCounter)
    {
        fBlock_.iFrame = 0;
        if (flags_.isTimeResetOnFrameCounterReset)
            fBlock_.iTime = 0;
        flags_.resetFrameCounter = false;
    }

    // The shaderCamera has its own event listeners, but all of its updates are
    // deferred (just like here nothing is processed/sent to the GPU in the
    // event callback), so we update it here and check whether the GPU data
    // should be updated as well
    shaderCamera_->update();

    // Re-gen random number
    if (!flags_.isRandomNumberGeneratorPaused)
        fBlock_.iRandom = random_->generateFloat();

    if 
    (
        fBlock_.iWASD != shaderCamera_->position() ||
        fBlock_.iLook != shaderCamera_->z()
    )
    {
        fBlock_.iWASD = shaderCamera_->position();
        fBlock_.iLook = shaderCamera_->z();
        fBlock_.iUserAction = true;
        flags_.updateDataRangeII = true;
    }
    
    // Data range I is always updated, data range III is updated on the spot
    // in setResolution, the keyboard data range is updated on the spot in
    // onReceive(KeyPressEvent/KeyReleaseEvent)
    if (!flags_.updateDataRangeII)
        fBuffer_->setData(&fBlock_, FragmentBlock::dataRangeISize(), 0);
    else
    {
        fBuffer_->setData(&fBlock_, FragmentBlock::dataRangeIISize(), 0);
        flags_.updateDataRangeII = false;
    }

    if (fBlock_.iUserAction) // Always reset
    {
        flags_.updateDataRangeII = true;
        fBlock_.iUserAction = false;
    }
}

//----------------------------------------------------------------------------//

void SharedUniforms::nextRenderPass(unsigned int nMaxRenderPasses)
{   
    if (fBlock_.iRenderPass < (int)nMaxRenderPasses-1)
        ++fBlock_.iRenderPass;
    else
        fBlock_.iRenderPass = 0;
    fBuffer_->setData(&fBlock_, FragmentBlock::dataRangeISize(), 0);
}

//----------------------------------------------------------------------------//

void SharedUniforms::resetTimeAndFrame(float time)
{
    flags_.resetFrameCounterPreOrPostExport = true;
    fBlock_.iTime = time;
}

//----------------------------------------------------------------------------//

void SharedUniforms::prepareForExport(bool setTime, float exportStartTime)
{
    if (setTime && flags_.isTimePaused)
        flags_.isTimePaused = false;
    exportData_.originalTime = fBlock_.iTime;
    flags_.resetFrameCounterPreOrPostExport = true;
    if (setTime)
        fBlock_.iTime = exportStartTime;
    exportData_.originalResolution = fBlock_.iResolution;
    setResolution(exportData_.resolution, false, true);

    fBlock_.iExport = true;
    flags_.updateDataRangeII = true;

    if (flags_.isVSyncEnabled)
        vir::Window::instance()->setVSync(false);
}

//----------------------------------------------------------------------------//

void SharedUniforms::resetAfterExport(bool resetFrameCounter)
{
    flags_.resetFrameCounterPreOrPostExport = resetFrameCounter;
    fBlock_.iTime = exportData_.originalTime;
    ExportData cache = exportData_;
    setResolution(exportData_.originalResolution, false, false);
    exportData_ = cache;

    fBlock_.iExport = false;
    flags_.updateDataRangeII = true;

    if (flags_.isVSyncEnabled)
        vir::Window::instance()->setVSync(true);
}

//----------------------------------------------------------------------------//

void SharedUniforms::save(ObjectIO& io) const
{
    io.writeObjectStart("sharedUniforms");
    io.write("windowResolution", fBlock_.iResolution);
    io.write("exportWindowResolutionScale", exportData_.resolutionScale);
    io.write("time", fBlock_.iTime);
    io.write("timePaused", flags_.isTimePaused);
    io.write("timeLooped", flags_.isTimeLooped);
    io.write("timeBounds", bounds_.at(Uniform::SpecialType::Time));
    io.write("randomGeneratorPaused", flags_.isRandomNumberGeneratorPaused);
    io.write("iWASD", shaderCamera_->position());
    io.write("iWASDSensitivity", shaderCamera_->keySensitivityRef());
    io.write("iWASDInputEnabled", flags_.isCameraKeyboardInputEnabled);
    io.write("iLook", shaderCamera_->z());
    io.write("iLookSensitivity",shaderCamera_->mouseSensitivityRef());
    io.write("iLookInputEnabled",flags_.isCameraMouseInputEnabled);
    io.write("iLookInputRequiresLMBHold", 
        flags_.cameraMouseInputRequiresLMBHold);
    io.write("iMouseInputEnabled", flags_.isMouseInputEnabled);
    io.write("iMouseInputClampedToWindow", flags_.isMouseInputClampedToWindow);
    io.write("mouseInputRequiresLMBHold", 
        flags_.mouseInputRequiresLMBHold);
    io.write("iKeyboardInputEnabled", flags_.isKeyboardInputEnabled);
    io.write("smoothTimeDelta", flags_.isTimeDeltaSmooth);
    io.write("resetTimeOnFrameCounterReset", 
        flags_.isTimeResetOnFrameCounterReset);
    io.write("cursorStatus", 
        vir::Window::instance()->cursorStatus() == 
        vir::Window::CursorStatus::Captured);
    io.write("vSyncEnabled", flags_.isVSyncEnabled);
    Uniform::saveAll(io, userUniforms_);
    io.writeObjectEnd();
}

void SharedUniforms::load
(
    const ObjectIO& io, 
    SharedUniforms*& su,
    const std::vector<Resource*>& resources
)
{
    if (su != nullptr)
        delete su;
    su = new SharedUniforms();
    auto ioSu = io.readObject("sharedUniforms");
    auto resolution = (glm::ivec2)ioSu.read<glm::vec2>("windowResolution");
    su->setResolution(resolution, false);
    su->fBlock_.iTime = ioSu.read<float>("time");
    su->flags_.resetFrameCounter = false;
    su->bounds_[Uniform::SpecialType::Time] = ioSu.read<glm::vec2>("timeBounds");
    su->fBlock_.iWASD = ioSu.read<glm::vec3>("iWASD");
    su->fBlock_.iLook = ioSu.read<glm::vec3>("iLook");
    su->flags_.isTimePaused = ioSu.read<bool>("timePaused");
    su->flags_.isTimeLooped = ioSu.read<bool>("timeLooped");
    su->flags_.isTimeDeltaSmooth = 
        ioSu.readOrDefault<bool>("smoothTimeDelta", false);
    su->flags_.isTimeResetOnFrameCounterReset = 
        ioSu.read<bool>("resetTimeOnFrameCounterReset");
    su->flags_.isRandomNumberGeneratorPaused = 
        ioSu.readOrDefault<bool>("randomGeneratorPaused", false);
    su->shaderCamera_->setDirection(su->fBlock_.iLook.packed());
    su->shaderCamera_->setPosition(su->fBlock_.iWASD.packed());
    su->shaderCamera_->setKeySensitivity(ioSu.read<float>("iWASDSensitivity"));
    su->shaderCamera_->setMouseSensitivity(ioSu.read<float>("iLookSensitivity"));
    su->shaderCamera_->update();
    if (!ioSu.read<bool>("iWASDInputEnabled"))
        su->toggleCameraKeyboardInputs();
    if (!ioSu.read<bool>("iLookInputEnabled"))
        su->toggleCameraMouseInputs();
    su->flags_.cameraMouseInputRequiresLMBHold = 
        ioSu.readOrDefault<bool>("iLookInputRequiresLMBHold", true);
    if (!ioSu.read<bool>("iMouseInputEnabled"))
        su->toggleMouseInputs();
    su->setMouseInputsClamped(
        ioSu.readOrDefault<bool>("iMouseInputClampedToWindow", false));
    su->flags_.mouseInputRequiresLMBHold = 
        ioSu.readOrDefault<bool>("mouseInputRequiresLMBHold", true);
    if (!ioSu.read<bool>("iKeyboardInputEnabled"))
        su->toggleKeyboardInputs();
    su->exportData_.resolutionScale = 
        ioSu.read<float>("exportWindowResolutionScale");
    su->exportData_.resolution = 
        (glm::vec2)su->fBlock_.iResolution*
        su->exportData_.resolutionScale + .5f;
    su->flags_.isVSyncEnabled = ioSu.readOrDefault<bool>("vSyncEnabled", true);
    if (ioSu.readOrDefault<bool>("cursorStatus", false))
        su->setMouseCaptured(true);
    vir::Window::instance()->setVSync(su->flags_.isVSyncEnabled);
    Uniform::loadAll
    (
        ioSu, 
        su->userUniforms_,
        resources,
        su->cache_.uninitializedResourceLayers
    );
    su->flags_.updateDataRangeII = true;
}

void SharedUniforms::postLoadProcessCachedResourceLayers
(
    const std::vector<Resource*>& resources
)
{
    for (auto& entry : cache_.uninitializedResourceLayers)
    {
        auto* uniform = entry.first;
        auto& layerName = entry.second;
        for (auto resource : resources)
        {
            if (resource->name() != layerName)
                continue;
            uniform->setValuePtr<Resource>(resource);
        }
    }
    cache_.uninitializedResourceLayers.clear();
}

//----------------------------------------------------------------------------//

void SharedUniforms::renderWindowMenuGui()
{
    if (ImGui::BeginMenu("Window", !vir::Window::instance()->iconified()))
    {
        ImGui::Text("Resolution    ");
        ImGui::SameLine();
        ImGui::PushItemWidth(8.0*ImGui::GetFontSize());
        glm::ivec2 resolution(fBlock_.iResolution);
        if 
        (
            ImGui::InputInt2
            (
                "##windowResolution", 
                glm::value_ptr(resolution)
            )
        )
            setResolution(resolution, false);
        ImGui::PopItemWidth();

        auto window = vir::Window::instance();
        ImGui::Text("VSync         ");
        ImGui::SameLine();
        if (ImGui::Checkbox("##windowVSync", &flags_.isVSyncEnabled))
            window->setVSync(flags_.isVSyncEnabled);

        if (ImGui::Button("Capture mouse cursor", ImVec2(-1, 0)))
            setMouseCaptured(true);

        ImGui::EndMenu();
    }
}

void SharedUniforms::setMouseCaptured(bool flag)
{
    auto window = vir::Window::instance();
    if (!flag)
    {
        window->setCursorStatus(vir::Window::CursorStatus::Normal);
        StatusBar::clearMessage();
        StatusBar::queueTemporaryMessage("Mouse cursor freed", 3);
    }
    if (flag)
    {
        auto position = 
            vir::Window::instance()->position
            (
                vir::Window::PositionOf::Center
            );
        auto pauseForOneBroadcast = [this]
        (
            vir::Event::Receiver* receiver, 
            vir::Event::Type type
        )
        {
            if (!receiver->isEventReceptionPaused(type))
                receiver->pauseEventReception(type, 1);
        };
        pauseForOneBroadcast(this, vir::Event::Type::MouseMotion);
        pauseForOneBroadcast(this, vir::Event::Type::MouseButtonPress);
        pauseForOneBroadcast(this, vir::Event::Type::MouseButtonRelease);
        pauseForOneBroadcast
        (
            (vir::InputCamera*)this->shaderCamera_, 
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
        StatusBar::setMessage
        (
            "Mouse cursor captured by window (press ESC to free)"
        );
    }
}

}