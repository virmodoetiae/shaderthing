#include "vpch.h"
#include "vcamera/vinputcamera.h"

namespace vir
{

InputCamera::InputCamera()
{
    this->tuneIn();
    updated_ = false;
    update();
}

void InputCamera::onReceive(Event::KeyPressEvent& event)
{
    if (!modifiable_)
        return;
    if (event.keyCode() == VIR_KEY_P && event.repeatCount() == 0)
        switchProjectionType();
}

void InputCamera::onReceive(Event::MouseMotionEvent& event)
{
    if (!modifiable_)
        return;
    auto inputState = GlobalPtr<InputState>::instance();
    if (inputState == nullptr)
        return;
    if (inputState->pressedMouseButtons()[VIR_MOUSE_BUTTON_1])
    {
        dTheta_ += -event.dy()*TPI_BY_360;
        dPhi_ += -event.dx()*TPI_BY_360;
        updated_ = false;
    }
}

void InputCamera::onReceive(Event::MouseScrollEvent& event)
{
    if (!modifiable_)
        return;
    if (event.dy() > 0)
    {
        fov_ *= mouseWheelSensitivity_;
        viewportHeight_ *= mouseWheelSensitivity_;
        mouseSensitivity_ *= mouseWheelSensitivity_;
    }
    else 
    { 
        fov_ /= mouseWheelSensitivity_;
        viewportHeight_ /= mouseWheelSensitivity_;
        mouseSensitivity_ /= mouseWheelSensitivity_;
    }
    constexpr float maxFov(glm::radians(150.0f));
    constexpr float minFov(glm::radians(.5f));
    fov_ = std::max(std::min(fov_, maxFov), minFov);
    updated_ = false;
}

void InputCamera::onReceive(Event::WindowResizeEvent& event)
{
    (void)event;
    updated_ = false;
}

void InputCamera::update()
{
    // Check for pressed keys
    auto inputState = GlobalPtr<InputState>::instance();
    if (inputState == nullptr)
        return;
    auto pressedKeys = inputState->pressedKeys();
    bool W = pressedKeys[VIR_KEY_W];
    bool A = pressedKeys[VIR_KEY_A];
    bool S = pressedKeys[VIR_KEY_S];
    bool D = pressedKeys[VIR_KEY_D];
    bool Q = pressedKeys[VIR_KEY_Q];
    bool E = pressedKeys[VIR_KEY_E];
    bool RCTRL = pressedKeys[VIR_KEY_LEFT_CONTROL];
    bool SPACE = pressedKeys[VIR_KEY_SPACE];
    bool keyPressEnabled = canCurrentlyReceive(vir::Event::Type::KeyPress);
    if 
    (
        (W || A || S || D || Q || E || RCTRL || SPACE) && 
        keyPressEnabled
    )
        updated_ = false;

    //
    if (updated_)
        return;

    float dt = GlobalPtr<Time>::instance()->smoothOuterTimestep();
    dTheta_ *= mouseSensitivity_;
    dPhi_ *= mouseSensitivity_;

    float sign = zPlusIsLookDirection_ ? 1.0 : -1.0;

    // Update yaw, pitch (mouse controlled) and roll (keyboard controlled)
    xRotateBy(dTheta_);
    yRotateBy(dPhi_);
    float ksdt = keySensitivity_*dt;
    if (yAxisType_ == YAxisType::Free)
    {
        float dPsi = ksdt/5.0f;
        if (Q)
            zRotateBy(-dPsi);
        else if (E)
            zRotateBy(dPsi);
    }

    // Update position and pivot
    if (modifiable_ && keyPressEnabled)
    {
        glm::vec3 dPos(0.0f);
        if (W)
            dPos += sign*ksdt*z_;
        else if (S)
            dPos -= sign*ksdt*z_;
        if (A)
            dPos += sign*ksdt*x_;
        else if (D)
            dPos -= sign*ksdt*x_;
        if (SPACE)
            dPos += ksdt*y_;
        else if (RCTRL)
            dPos -= ksdt*y_;
        position_ += dPos;
        pivot_ += dPos;
    }
    
    updatePositionAroundPiovot();
    
    updateProjectionMatrix();

    projectionViewMatrix_ = projectionMatrix_ * glm::lookAt
    (
        position_,
        position_ + sign*z_,
        y_
    );

    dTheta_ = 0.0f;
    dPhi_ = 0.0f;

    updated_ = true;
}



}