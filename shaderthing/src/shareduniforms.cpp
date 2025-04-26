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

#include "shaderthing/include/shareduniforms.h"

#include "shaderthing/include/layer.h"
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
       iResolution_ = {window->width(), window->height()};
    iAspectRatio_ = iResolution_.x/iResolution_.y;
    for (int i=0; i<256; i++)
        iKeyboard_[i] = glm::ivec3({0,0,0});

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
        std::min(1.0f, 1.0f/iAspectRatio_)
    );
    screenCamera_->setPosition({0, 0, 1});
    screenCamera_->setPlanes(.01f, 100.f);
    shaderCamera_->setZPlusIsLookDirection(true);
    shaderCamera_->setDirection(iLook_);
    shaderCamera_->setPosition(iWASD_);
    screenCamera_->update();
    shaderCamera_->update();
    iMVP_ = screenCamera_->projectionViewMatrix();

    // Init random number generator and set initial random number
    if (random_ == nullptr)
        random_ = new Random();
    iRandom_ = random_->generateFloat();

    

    // Init uniform buffers, bind to designated binding points and set
    // initial data
    if (vBuffer_ == nullptr)
        vBuffer_ = 
            vir::DynamicUniformBuffer::create(64, "vertexSharedUniformBlock");
    vBuffer_->bind();
    vBuffer_->setBindingPoint(vBindingPoint_);

    iMVPUniform_.name = "iMVP";
    iMVPUniform_.setValuePtr(&iMVP_, Uniform::Type::Mat4);
    iMVPUniform_.gui.showBounds = false;
    vBuffer_->addUniform(&iMVPUniform_);

    if (fBuffer_ == nullptr)
        fBuffer_ = 
            vir::DynamicUniformBuffer::create(8196, "sharedUniformBlock");
    fBuffer_->bind();
    fBuffer_->setBindingPoint(fBindingPoint_);
    /*fBuffer_->setData
    (
        &(fBlock_),
        FragmentBlock::size(),
        0
    );*/

        // Init uniform wrappers
    iFrameUniform_.name = "iFrame";
    iFrameUniform_.setValuePtr(&iFrame_, Uniform::Type::Int);
    iFrameUniform_.gui.showBounds = false;
    iFrameUniform_.specialType = Uniform::SpecialType::Frame;
    fBuffer_->addUniform(&iFrameUniform_);

    iRenderPassUniform_.name = "iRenderPass";
    iRenderPassUniform_.setValuePtr(&iRenderPass_, Uniform::Type::Int);
    iRenderPassUniform_.gui.showBounds = false;
    iRenderPassUniform_.specialType = Uniform::SpecialType::RenderPass;
    fBuffer_->addUniform(&iRenderPassUniform_);
    
    iTimeUniform_.name = "iTime";
    iTimeUniform_.setValuePtr(&iTime_, Uniform::Type::Float);
    iTimeUniform_.specialType = Uniform::SpecialType::Time;
    fBuffer_->addUniform(&iTimeUniform_);
    
    iTimeDeltaUniform_.name = "iTimeDelta";
    iTimeDeltaUniform_.setValuePtr(&iTimeDelta_, Uniform::Type::Float);
    iTimeDeltaUniform_.gui.showBounds = false;
    fBuffer_->addUniform(&iTimeDeltaUniform_);

    iRandomUniform_.name = "iRandom";
    iRandomUniform_.setValuePtr(&iRandom_, Uniform::Type::Float);
    iRandomUniform_.gui.showBounds = false;
    fBuffer_->addUniform(&iRandomUniform_);

    iUserActionUniform_.name = "iUserAction";
    iUserActionUniform_.setValuePtr(&iUserAction_, Uniform::Type::Bool);
    iUserActionUniform_.gui.showBounds = false;
    iUserActionUniform_.specialType = Uniform::SpecialType::UserAction;
    fBuffer_->addUniform(&iUserActionUniform_);

    iExportUniform_.name = "iExport";
    iExportUniform_.setValuePtr(&iExport_, Uniform::Type::Bool);
    iExportUniform_.gui.showBounds = false;
    fBuffer_->addUniform(&iExportUniform_);

    iWASDUniform_.name = "iWASD";
    iWASDUniform_.setValuePtr(&iWASD_, Uniform::Type::Float3);
    iWASDUniform_.specialType = Uniform::SpecialType::CameraPosition;
    fBuffer_->addUniform(&iWASDUniform_);

    iLookUniform_.name = "iLook";
    iLookUniform_.setValuePtr(&iLook_, Uniform::Type::Float3);
    iLookUniform_.gui.showBounds = false;
    iLookUniform_.specialType = Uniform::SpecialType::CameraDirection;
    fBuffer_->addUniform(&iLookUniform_);

    iMouseUniform_.name = "iMouse";
    iMouseUniform_.setValuePtr(&iMouse_, Uniform::Type::Float4);
    iMouseUniform_.gui.showBounds = false;
    iMouseUniform_.specialType = Uniform::SpecialType::Mouse;
    fBuffer_->addUniform(&iMouseUniform_);

    iAspectRatioUniform_.name = "iWindowAspectRatio";
    iAspectRatioUniform_.setValuePtr(&iAspectRatio_, Uniform::Type::Float);
    iAspectRatioUniform_.gui.showBounds = false;
    iAspectRatioUniform_.specialType = Uniform::SpecialType::WindowAspectRatio;
    fBuffer_->addUniform(&iAspectRatioUniform_);

    iResolutionUniform_.name = "iWindowResolution";
    iResolutionUniform_.setValuePtr(&iResolution_, Uniform::Type::Float2);
    iResolutionUniform_.gui.showBounds = false;
    iResolutionUniform_.specialType = Uniform::SpecialType::WindowResolution;
    fBuffer_->addUniform(&iResolutionUniform_);

    iKeyboardUniform_.name = "iKeyboard";
    iKeyboardUniform_.setValuePtr(&iKeyboard_, Uniform::Type::Int3, 256);
    iKeyboardUniform_.gui.showBounds = false;
    iKeyboardUniform_.specialType = Uniform::SpecialType::Keyboard;
    fBuffer_->addUniform(&iKeyboardUniform_);

    // Init bounds
    //bounds_.insert({Uniform::SpecialType::Time, {0, 1}});
    //bounds_.insert({Uniform::SpecialType::CameraPosition, {0, 1}});

    exportData_.resolution = iResolution_;

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
    iResolution_ = resolution;
    iAspectRatio_ = ((float)resolution.x)/resolution.y;

    // If not preparing for export, reset export resolution and its scale if
    // the window is resized in any way (either manullay or via the GUI). Not
    // necessary but I like this behavior better
    if (!prepareForExport)
    {
        exportData_.resolution = iResolution_;
        exportData_.resolutionScale = 1.f;
    }

    // Update screen camera
    screenCamera_->setViewportHeight
    (
        std::min(1.0f, 1.0f/iAspectRatio_)
    );
    screenCamera_->update();
    iMVP_ = screenCamera_->projectionViewMatrix();
    vBuffer_->markUniformForSubmission(&iMVPUniform_);
    fBuffer_->markUniformForSubmission(&iAspectRatioUniform_);
    fBuffer_->markUniformForSubmission(&iResolutionUniform_);
    
    // Let's try updating these instantly
    //fBuffer_->setData(&fBlock_, FragmentBlock::dataRangeIIISize(), 0);
    //vBuffer_->bind();
    //vBuffer_->setData(&vBlock_, VertexBlock::size(), 0);
    //fBuffer_->bind();

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
        iMouse_.x = 
            std::max(std::min(iMouse_.x, iResolution_.x), 0.f);
        iMouse_.y = 
            std::max(std::min(iMouse_.y, iResolution_.y), 0.f);
    }
    flags_.updateDataRangeII = true;
}

//----------------------------------------------------------------------------//

void SharedUniforms::setUserAction(bool flag)
{
    iUserAction_ = int(flag);
    fBuffer_->markUniformForSubmission(&iUserActionUniform_);
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
        iResolution_.y-event.y,
        iMouse_.x,
        -iMouse_.y
    };
    if (flags_.isMouseInputClampedToWindow)
    {
        mouse.x = std::max(std::min(mouse.x, iResolution_.x), 0.f);
        mouse.y = std::max(std::min(mouse.y, iResolution_.y), 0.f);
    }
    if (iMouse_ == mouse)
        return;
    iUserAction_ = true;
    iMouse_ = mouse;
    fBuffer_->markUniformForSubmission(&iUserActionUniform_);
    fBuffer_->markUniformForSubmission(&iMouseUniform_);
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
        iResolution_.y-event.y,
        iMouse_.z,
        iMouse_.w
    };
    if (flags_.isMouseInputClampedToWindow)
    {
        mouse.x = std::max(std::min(mouse.x, iResolution_.x), 0.f);
        mouse.y = std::max(std::min(mouse.y, iResolution_.y), 0.f);
    }
    if (iMouse_ == mouse)
        return;
    iUserAction_ = true;
    iMouse_ = mouse;
    fBuffer_->markUniformForSubmission(&iUserActionUniform_);
    fBuffer_->markUniformForSubmission(&iMouseUniform_);
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
        iMouse_.x,
        iMouse_.y,
        iMouse_.z*-1,
        iMouse_.w
    };
    if (iMouse_ == mouse)
        return;
    iUserAction_ = true;
    iMouse_.z = mouse.z;
    fBuffer_->markUniformForSubmission(&iUserActionUniform_);
    fBuffer_->markUniformForSubmission(&iMouseUniform_);
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
    auto stKeyCode = vir::inputKeyCodeVirToShaderToy(event.keyCode);
    if (stKeyCode > 256)
        return;
    auto& data(iKeyboard_[stKeyCode]);
    static auto* inputState = vir::InputState::instance();
    auto& status = inputState->keyState(event.keyCode);
    data.x = (int)status.isPressed();
    data.y = (int)status.isHeld();
    data.z = (int)status.isToggled();
    fBuffer_->markArrayUniformRangeForSubmission(&iKeyboardUniform_, stKeyCode);
    /*// Only exception where I set the data immediately in the event callback in
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
    );*/
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::KeyReleaseEvent& event)
{
    auto stKeyCode = vir::inputKeyCodeVirToShaderToy(event.keyCode);
    if (stKeyCode > 256)
        return;
    auto& data(iKeyboard_[stKeyCode]);
    static auto* inputState = vir::InputState::instance();
    data.x = 0;
    data.y = 0;
    data.z = (int)inputState->keyState(event.keyCode).isToggled();
    fBuffer_->markArrayUniformRangeForSubmission(&iKeyboardUniform_, stKeyCode);
    /*// Only exception where I set the data immediately in the event callback in
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
    );*/
}

//----------------------------------------------------------------------------//

void SharedUniforms::bindShader(vir::Shader* shader) const
{
    shader->bindUniformBlock
    (
        fBuffer_->name(),
        fBindingPoint_
    );
    shader->bindUniformBlock
    (
        vBuffer_->name(),
        vBindingPoint_
    );
}

//----------------------------------------------------------------------------//

void SharedUniforms::update(const UpdateArgs& args)
{
    if (!flags_.isTimePaused)
    {
        iTime_ += args.timeStep;
        if (args.advanceFrame)
            iTimeDelta_ = args.timeStep;
    }
    else if 
    (
        args.advanceFrame && 
        (flags_.stepToNextFrame || flags_.stepToNextTimeStep)
    )
        iTime_ += iTimeDelta_;

    const glm::vec2& timeLoopBounds(iTimeUniform_.gui.bounds);
    if (flags_.isTimeLooped && iTime_ >= timeLoopBounds.y)
    {
        auto duration = timeLoopBounds.y-timeLoopBounds.x;
        auto fraction = 
            (iTime_-timeLoopBounds.y)/std::max(duration, 1e-6f);
        fraction -= (int)fraction;
        iTime_ = timeLoopBounds.x + duration*fraction;
    }
    
    if 
    (
        args.advanceFrame && 
        !(flags_.isRenderingPaused && !flags_.stepToNextFrame)
    )
        ++iFrame_;

    if (flags_.resetFrameCounterPreOrPostExport)
    {
        iFrame_ = 0;
        flags_.resetFrameCounterPreOrPostExport = false;
    }
    if (flags_.resetFrameCounter)
    {
        iFrame_ = 0;
        if (flags_.isTimeResetOnFrameCounterReset)
            iTime_ = 0;
        flags_.resetFrameCounter = false;
    }

    // The shaderCamera has its own event listeners, but all of its updates are
    // deferred (just like here nothing is processed/sent to the GPU in the
    // event callback), so we update it here and check whether the GPU data
    // should be updated as well
    shaderCamera_->update();

    // Re-gen random number
    if (!flags_.isRandomNumberGeneratorPaused)
        iRandom_ = random_->generateFloat();

    if 
    (
        iWASD_ != shaderCamera_->position() ||
        iLook_ != shaderCamera_->z()
    )
    {
        iWASD_ = shaderCamera_->position();
        iLook_ = shaderCamera_->z();
        iUserAction_ = true;
        flags_.updateDataRangeII = true;
    }
    
    // Data range I is always updated, data range III is updated on the spot
    // in setResolution, the keyboard data range is updated on the spot in
    // onReceive(KeyPressEvent/KeyReleaseEvent)
    fBuffer_->markContiguousUniformsForSubmission(&iFrameUniform_, &iRandomUniform_);
    /*
    if (!flags_.updateDataRangeII)
        fBuffer_->setData(&fBlock_, FragmentBlock::dataRangeISize(), 0);
    else
    {
        fBuffer_->setData(&fBlock_, FragmentBlock::dataRangeIISize(), 0);
        flags_.updateDataRangeII = false;
    }*/
    if (flags_.updateDataRangeII)
    {
        fBuffer_->markContiguousUniformsForSubmission(&iUserActionUniform_, &iMouseUniform_);
        flags_.updateDataRangeII = false;
    }

    vBuffer_->submitUniforms();
    fBuffer_->submitUniforms();

    if (iUserAction_) // Always reset
    {
        flags_.updateDataRangeII = true;
        iUserAction_ = false;
    }
}

//----------------------------------------------------------------------------//

void SharedUniforms::nextRenderPass(unsigned int nMaxRenderPasses)
{   
    if (iRenderPass_ < (int)nMaxRenderPasses-1)
        ++iRenderPass_;
    else
        iRenderPass_ = 0;
    fBuffer_->markUniformForSubmission(&iRenderPassUniform_);
    //fBuffer_->setData(&fBlock_, FragmentBlock::dataRangeISize(), 0);
}

//----------------------------------------------------------------------------//

void SharedUniforms::resetTimeAndFrame(float time)
{
    flags_.resetFrameCounterPreOrPostExport = true;
    iTime_ = time;
}

//----------------------------------------------------------------------------//

void SharedUniforms::toggleRenderingPaused(bool dueToLowFps)
{
    flags_.isRenderingPaused = 
        !flags_.isRenderingPaused;
    
    if (flags_.isRenderingPaused)
    {
        flags_.isTimePausedBecauseRenderingPaused = 
            !flags_.isTimePaused;
        flags_.isTimePaused = true;
    }
    else if (flags_.isTimePausedBecauseRenderingPaused)
        flags_.isTimePaused = false;

    // It would be better to update the StatusBar messages elsewhere, but
    // whatever
    if (flags_.isRenderingPaused)
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

void SharedUniforms::prepareForExport(bool setTime, float exportStartTime)
{
    if (setTime && flags_.isTimePaused)
        flags_.isTimePaused = false;
    exportData_.originalTime = iTime_;
    flags_.resetFrameCounterPreOrPostExport = true;
    if (setTime)
        iTime_ = exportStartTime;
    exportData_.originalResolution = iResolution_;
    setResolution(exportData_.resolution, false, true);

    iExport_ = true;
    flags_.updateDataRangeII = true;
}

//----------------------------------------------------------------------------//

void SharedUniforms::resetAfterExport(bool resetFrameCounter)
{
    flags_.resetFrameCounterPreOrPostExport = resetFrameCounter;
    iTime_ = exportData_.originalTime;
    ExportData cache = exportData_;
    setResolution(exportData_.originalResolution, false, false);
    exportData_ = cache;

    iExport_ = false;
    flags_.updateDataRangeII = true;
}

//----------------------------------------------------------------------------//

void SharedUniforms::save(ObjectIO& io) const
{
    io.writeObjectStart("sharedUniforms");
    io.write("windowResolution", iResolution_);
    io.write("exportWindowResolutionScale", exportData_.resolutionScale);
    io.write("time", iTime_);
    io.write("timePaused", 
        flags_.isTimePaused && flags_.isTimePausedBecauseRenderingPaused ? 
        false : flags_.isTimePaused);
    io.write("timeLooped", flags_.isTimeLooped);
    io.write("timeBounds", iTimeUniform_.gui.bounds);
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
    su->iTime_ = ioSu.read<float>("time");
    su->flags_.resetFrameCounter = false;
    su->iTimeUniform_.gui.bounds = ioSu.read<glm::vec2>("timeBounds");
    su->iWASD_ = ioSu.read<glm::vec3>("iWASD");
    su->iLook_ = ioSu.read<glm::vec3>("iLook");
    su->flags_.isTimePaused = ioSu.read<bool>("timePaused");
    su->flags_.isTimeLooped = ioSu.read<bool>("timeLooped");
    su->flags_.isTimeDeltaSmooth = 
        ioSu.readOrDefault<bool>("smoothTimeDelta", false);
    su->flags_.isTimeResetOnFrameCounterReset = 
        ioSu.read<bool>("resetTimeOnFrameCounterReset");
    su->flags_.isRandomNumberGeneratorPaused = 
        ioSu.readOrDefault<bool>("randomGeneratorPaused", false);
    su->shaderCamera_->setDirection(su->iLook_);
    su->shaderCamera_->setPosition(su->iWASD_);
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
        (glm::vec2)su->iResolution_*
        su->exportData_.resolutionScale + .5f;
    if (ioSu.readOrDefault<bool>("cursorStatus", false))
        su->setMouseCaptured(true);
    Uniform::loadAll
    (
        ioSu, 
        su->userUniforms_,
        nullptr,
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
            // TODO: Once the sharedUniform buffer is set up, pass it
            uniform->setResourcePtr(resource); 
        }
    }
    cache_.uninitializedResourceLayers.clear();
}

//----------------------------------------------------------------------------//

void SharedUniforms::setMouseCaptured(bool flag)
{
    auto window = vir::Window::instance();
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
        StatusBar::queueMessage(mouseCapturedMessage);
    }
}

}