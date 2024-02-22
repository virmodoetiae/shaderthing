#include "shaderthing-p/include/shareduniforms.h"
#include "shaderthing-p/include/uniform.h"
#include "shaderthing-p/include/macros.h"
#include "vir/include/vir.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//

SharedUniforms::SharedUniforms(const unsigned int bindingPoint) :
    gpuBindingPoint_(bindingPoint)
{
    // Init CPU block data
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    cpuBlock_.iResolution = 
        glm::ivec2{window->width(), window->height()};
    cpuBlock_.iAspectRatio = 
        float(window->width())/float(window->height());
    for (int i=0; i<256; i++)
        cpuBlock_.iKeyboard[i] = glm::ivec3({0,0,0});

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
        std::min(1.0f, 1.0f/cpuBlock_.iAspectRatio)
    );
    screenCamera_->setPosition({0, 0, 1});
    screenCamera_->setPlanes(.01f, 100.f);
    shaderCamera_->setZPlusIsLookDirection(true);
    shaderCamera_->setDirection(cpuBlock_.iLook.packed());
    shaderCamera_->setPosition(cpuBlock_.iWASD.packed());
    screenCamera_->update();
    shaderCamera_->update();
    cpuBlock_.iMVP = 
        screenCamera_->projectionViewMatrix();

    // Init GPU-side uniform block, bind and copy data from CPU
    if (gpuBlock_ == nullptr);
        gpuBlock_ = 
            vir::UniformBuffer::create(SharedUniforms::Block::size());
    gpuBlock_->bind(gpuBindingPoint_);
    gpuBlock_->setData
    (
        &(cpuBlock_),
        SharedUniforms::Block::size(),
        0
    );

    // Init bounds
    bounds_.insert({Uniform::SpecialType::Time, {0, 1}});
    bounds_.insert({Uniform::SpecialType::CameraPosition, {0, 1}});

    // Register the class iteself with the vir event broadcaster
    this->tuneIntoEventBroadcaster(VIR_DEFAULT_PRIORITY-1);
}

//----------------------------------------------------------------------------//

SharedUniforms::~SharedUniforms()
{
    DELETE_IF_NOT_NULLPTR(gpuBlock_)
    DELETE_IF_NOT_NULLPTR(screenCamera_)
    DELETE_IF_NOT_NULLPTR(shaderCamera_)
}

//----------------------------------------------------------------------------//

void SharedUniforms::setResolution
(
    glm::ivec2& resolution, 
    bool windowFrameManuallyDragged
)
{
    // Limit resolution
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    auto monitorScale = window->contentScale();
    glm::ivec2 minResolution = {120*monitorScale.x, 1};
    glm::ivec2 maxResolution = window->primaryMonitorResolution();
    resolution.x = 
        std::max(std::min(resolution.x, maxResolution.x), minResolution.x);
    resolution.y = 
        std::max(std::min(resolution.y, maxResolution.y), minResolution.y);
    
    // Store in iResolution & update aspectRatio
    cpuBlock_.iResolution = resolution;
    cpuBlock_.iAspectRatio = ((float)resolution.x)/resolution.y;

    // Update screen camera
    screenCamera_->setViewportHeight
    (
        std::min(1.0f, 1.0f/cpuBlock_.iAspectRatio)
    );
    screenCamera_->update();
    cpuBlock_.iMVP = screenCamera_->projectionViewMatrix();
    flags_.updateDataRangeIII = true;

    // Set the actual window resolution and propagate event
    if (!windowFrameManuallyDragged)
        window->setSize
        (
            resolution.x,
            resolution.y
        );
}

//----------------------------------------------------------------------------//

void SharedUniforms::setUserAction(bool flag)
{
    cpuBlock_.iUserAction = int(flag);
    flags_.updateDataRangeII;
}

//----------------------------------------------------------------------------//

void SharedUniforms::toggleMouseInputs()
{
    flags_.isMouseInputEnabled = !flags_.isMouseInputEnabled;
    if (flags_.isMouseInputEnabled)
    {
        resumeEventReception(vir::Event::Type::MouseButtonPress);
        resumeEventReception(vir::Event::Type::MouseMotion);
        resumeEventReception(vir::Event::Type::MouseButtonRelease);
    }
    else
    {
        pauseEventReception(vir::Event::Type::MouseButtonPress);
        pauseEventReception(vir::Event::Type::MouseMotion);
        pauseEventReception(vir::Event::Type::MouseButtonRelease);
    }
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
    glm::ivec2 resolution{event.width, event.height};
    setResolution(resolution, true);
    event.width = resolution.x;
    event.height = resolution.y;
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::MouseButtonPressEvent& event)
{
    glm::vec4 mouse = 
    {
        event.x,
        cpuBlock_.iResolution.y-event.y,
        cpuBlock_.iMouse.x,
        -cpuBlock_.iMouse.y
    };
    if (cpuBlock_.iMouse == mouse)
        return;
    cpuBlock_.iUserAction = true;
    cpuBlock_.iMouse = mouse;
    flags_.updateDataRangeII = true;
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::MouseMotionEvent& event)
{
    if 
    (
        !vir::GlobalPtr<vir::InputState>::instance()->
        mouseButtonState(VIR_MOUSE_BUTTON_1).isClicked()
    )
        return;
    glm::vec4 mouse = 
    {
        event.x,
        cpuBlock_.iResolution.y-event.y,
        cpuBlock_.iMouse.z,
        cpuBlock_.iMouse.w
    };
    if (cpuBlock_.iMouse == mouse)
        return;
    cpuBlock_.iUserAction = true;
    cpuBlock_.iMouse = mouse;
    flags_.updateDataRangeII = true;
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::MouseButtonReleaseEvent& e)
{
    glm::vec4 mouse = 
    {
        cpuBlock_.iMouse.x,
        cpuBlock_.iMouse.y,
        cpuBlock_.iMouse.z*-1,
        cpuBlock_.iMouse.w
    };
    if (cpuBlock_.iMouse == mouse)
        return;
    cpuBlock_.iUserAction = true;
    cpuBlock_.iMouse.z = mouse.z;
    flags_.updateDataRangeII = true;
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::KeyPressEvent& event)
{
    Block::ivec3A16& data(cpuBlock_.iKeyboard[event.keyCode]);
    static auto* inputState = vir::GlobalPtr<vir::InputState>::instance();
    auto& status = inputState->keyState(event.keyCode);
    data.x = (int)status.isPressed();
    data.y = (int)status.isHeld();
    data.z = (int)status.isToggled();
    // Only exception where I set the data immediately in the event callback in
    // order to avoid having to update the whole 4kB of key memory all at once
    // at every SharedUniforms::update call
    int offset = Block::iKeyboardKeyOffset
    (
        vir::inputKeyCodeVirToShaderToy(event.keyCode)
    );
    gpuBlock_->setData
    (
        (void*)&data, 
        Block::iKeyboardKeySize(), 
        offset
    );
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::KeyReleaseEvent& event)
{
    Block::ivec3A16& data(cpuBlock_.iKeyboard[event.keyCode]);
    static auto* inputState = vir::GlobalPtr<vir::InputState>::instance();
    int shaderToyKeyCode = vir::inputKeyCodeVirToShaderToy(event.keyCode);
    data.x = 0;
    data.y = 0;
    data.z = (int)inputState->keyState(event.keyCode).isToggled();
    // Only exception where I set the data immediately in the event callback in
    // order to avoid having to update the whole 4kB of key memory all at once
    // at every SharedUniforms::update call
    gpuBlock_->setData
    (
        (void*)&data, 
        Block::iKeyboardKeySize(), 
        Block::iKeyboardKeyOffset
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
        Block::glslName,
        *gpuBlock_,
        gpuBindingPoint_
    );
}

//----------------------------------------------------------------------------//

void SharedUniforms::update(const UpdateArgs& args)
{
    //
    //static const auto window = vir::GlobalPtr<vir::Window>::instance();
    if (!flags_.isTimePaused)
        cpuBlock_.iTime += args.timeStep;//window->time()->outerTimestep();

    const glm::vec2& timeLoopBounds(bounds_[Uniform::SpecialType::Time]);
    if (flags_.isTimeLooped && cpuBlock_.iTime >= timeLoopBounds.y)
    {
        auto duration = timeLoopBounds.y-timeLoopBounds.x;
        auto fraction = 
            (cpuBlock_.iTime-timeLoopBounds.y)/std::max(duration, 1e-6f);
        fraction -= (int)fraction;
        cpuBlock_.iTime = timeLoopBounds.x + duration*fraction;
    }
    
    if (!flags_.isRenderingPaused)
        ++cpuBlock_.iFrame;
    
    if (flags_.restartRendering)
    {
        cpuBlock_.iFrame = 0;
        if (flags_.isTimeResetOnRenderingRestart)
            cpuBlock_.iTime = 0;
        flags_.restartRendering = false;
    }

    // The shaderCamera has its own event listeners, but all of its updates are
    // deferred (just like here nothing is processed/sent to the GPU in the
    // event callback), so we update it here and check whether the GPU data
    // should be updated as well
    shaderCamera_->update();
    if 
    (
        cpuBlock_.iWASD != shaderCamera_->position() ||
        cpuBlock_.iLook != shaderCamera_->z()
    )
    {
        cpuBlock_.iWASD = shaderCamera_->position();
        cpuBlock_.iLook = shaderCamera_->z();
        cpuBlock_.iUserAction = true;
        flags_.updateDataRangeII = true;
    }
    
    // The goal of the following section is to reduce the number of gpuBlock
    // setData calls, hence the weird branching
    if (!flags_.updateDataRangeIII)
    {
        if (!flags_.updateDataRangeII)
            gpuBlock_->setData(&cpuBlock_, Block::dataRangeICumulativeSize(), 0);
        else
        {
            gpuBlock_->setData(&cpuBlock_, Block::dataRangeIICumulativeSize(), 0);
            flags_.updateDataRangeII = false;
        }
    }
    else
    {
        gpuBlock_->setData(&cpuBlock_, Block::dataRangeIIICumulativeSize(), 0);
        flags_.updateDataRangeIII = false;
    }
    if (cpuBlock_.iUserAction) // Always reset
    {
        flags_.updateDataRangeII = true;
        cpuBlock_.iUserAction = false;
    }
}

//----------------------------------------------------------------------------//

void SharedUniforms::renderWindowResolutionMenuGUI()
{
    if (ImGui::BeginMenu("Window"))
    {
        ImGui::Text("Resolution ");
        ImGui::SameLine();
        ImGui::PushItemWidth(8.0*ImGui::GetFontSize());
        glm::ivec2 resolution(cpuBlock_.iResolution);
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
        ImGui::EndMenu();
    }
}

}