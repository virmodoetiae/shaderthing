#include "shaderthing-p/include/shareduniforms.h"
#include "vir/include/vir.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//

#define DELETE_IF_NULLPTR(ptr) if (ptr != nullptr) delete ptr;

//----------------------------------------------------------------------------//

SharedUniforms::SharedUniforms(const unsigned int bindingPoint) :
    gpuBindingPoint_(bindingPoint)
{
    // Init CPU block dataW
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    cpuBlock_.iResolution = 
        glm::ivec2{window->width(), window->height()};
    cpuBlock_.iAspectRatio = 
        float(window->width())/float(window->height());
    for (int i=0; i<256; i++)
    {
        cpuBlock_.iKeyboard[i] = glm::ivec3({0,0,0});
    }

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
    shaderCamera_->setZPlusIsLookDirection(true);
    shaderCamera_->setDirection
    (
        cpuBlock_.iLook.packed()
    );
    shaderCamera_->setPosition
    (
        cpuBlock_.iWASD.packed()
    );
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

    // Finally, register the class iteself with the vir event broadcaster
    this->tuneIn();
    this->receiverPriority() = 1;
}

//----------------------------------------------------------------------------//

SharedUniforms::~SharedUniforms()
{
    DELETE_IF_NULLPTR(gpuBlock_)
    DELETE_IF_NULLPTR(screenCamera_)
    DELETE_IF_NULLPTR(shaderCamera_)
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::WindowResizeEvent& event)
{
    // Limit resolution
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    auto monitorScale = window->contentScale();
    glm::ivec2 minResolution = {120*monitorScale.x, 1};
    glm::ivec2 maxResolution = window->primaryMonitorResolution();
    event.width = 
        std::max(std::min(event.width, maxResolution.x), minResolution.x);
    event.height = 
        std::max(std::min(event.height, maxResolution.y), minResolution.y);
    if (window->width() != event.width || window->height() != event.height)
        window->setSize
        (
            event.width,
            event.height,
            false // Do not re-trigger the whole WindowResizeEvent broadcasting
                  // else you get an infinite loop
        );

    // Store in iResolution & update aspectRatio
    glm::ivec2 resolution = {event.width, event.height};
    if (cpuBlock_.iResolution == resolution)
        return;
    cpuBlock_.iResolution = resolution;
    cpuBlock_.iAspectRatio = ((float)resolution.x)/resolution.y;

    // Finally, update screen camera
    screenCamera_->setViewportHeight
    (
        std::min(1.0f, 1.0f/cpuBlock_.iAspectRatio)
    );
    screenCamera_->update();
    cpuBlock_.iMVP = screenCamera_->projectionViewMatrix();
    flags_.updateDataRangeIII = true;
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
    static const unsigned int size(sizeof(Block::ivec3A16));
    Block::ivec3A16& data(cpuBlock_.iKeyboard[event.keyCode]);
    static auto* inputState = vir::GlobalPtr<vir::InputState>::instance();
    int shaderToyKeyCode = vir::inputKeyCodeVirToShaderToy(event.keyCode);
    auto& status = inputState->keyState(event.keyCode);
    data.x = (int)status.isPressed();
    data.y = (int)status.isHeld();
    data.z = (int)status.isToggled();
    // Only exception where I set the data immediately in the event callback in
    // order to avoid having to update the whole 4kB of key memory all at once
    // at every SharedUniforms::update call
    gpuBlock_->setData((void*)&data, size, size*shaderToyKeyCode);
}

//----------------------------------------------------------------------------//

void SharedUniforms::onReceive(vir::Event::KeyReleaseEvent& event)
{
    static const unsigned int size(sizeof(Block::ivec3A16));
    Block::ivec3A16& data(cpuBlock_.iKeyboard[event.keyCode]);
    static auto* inputState = vir::GlobalPtr<vir::InputState>::instance();
    int shaderToyKeyCode = vir::inputKeyCodeVirToShaderToy(event.keyCode);
    data.x = 0;
    data.y = 0;
    data.z = (int)inputState->keyState(event.keyCode).isToggled();
    // Only exception where I set the data immediately in the event callback in
    // order to avoid having to update the whole 4kB of key memory all at once
    // at every SharedUniforms::update call
    gpuBlock_->setData((void*)&data, size, size*shaderToyKeyCode);
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

void SharedUniforms::update()
{
    //
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    cpuBlock_.iTime += window->time()->outerTimestep();
    cpuBlock_.iFrame += 1;

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

    flags_.updateDataRangeII = false;
    flags_.updateDataRangeIII = false;
}

//----------------------------------------------------------------------------//

void SharedUniforms::renderWindowResolutionMenuGUI()
{
    if (ImGui::BeginMenu("Window"))
    {
        ImGui::Text("Resolution");
        ImGui::SameLine();
        ImGui::PushItemWidth(8.0*ImGui::GetFontSize());
        if 
        (
            ImGui::InputInt2
            (
                "##windowResolution", 
                glm::value_ptr(cpuBlock_.iResolution)
            )
        )
        {
            static const auto broadcaster =
                vir::GlobalPtr<vir::Event::Broadcaster>::instance();
            broadcaster->broadcast
            (
                vir::Event::WindowResizeEvent
                (
                    cpuBlock_.iResolution.x,
                    cpuBlock_.iResolution.y
                )
            );
        }
        ImGui::PopItemWidth();
        ImGui::EndMenu();
    }
}

void SharedUniforms::renderRowsGUI()
{

}

}