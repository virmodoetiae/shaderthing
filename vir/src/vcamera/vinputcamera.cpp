#include "vpch.h"
#include "vcamera/vinputcamera.h"

namespace vir
{

InputCamera::InputCamera()
{
    tuneIntoEventBroadcaster(VIR_CAMERA_PRIORITY);
    updated_ = false;
    update();
}

void InputCamera::onReceive(Event::KeyPressEvent& event)
{
    (void)event;
}

void InputCamera::onReceive(Event::MouseMotionEvent& event)
{
    if (!modifiable_)
        return;
    auto inputState = GlobalPtr<InputState>::instance();
    if (inputState == nullptr)
        return;
    if (inputState->mouseButtonState(VIR_MOUSE_BUTTON_1).isClicked())
    {
        dTheta_ += -event.dy*TPI_BY_360;
        dPhi_ += -event.dx*TPI_BY_360;
        updated_ = false;
    }
}

void InputCamera::onReceive(Event::MouseScrollEvent& event)
{
    if (!modifiable_)
        return;
    if (event.dy > 0)
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
    bool W = inputState->keyState(VIR_KEY_W).isPressedOrHeld();
    bool A = inputState->keyState(VIR_KEY_A).isPressedOrHeld();
    bool S = inputState->keyState(VIR_KEY_S).isPressedOrHeld();
    bool D = inputState->keyState(VIR_KEY_D).isPressedOrHeld();
    bool Q = inputState->keyState(VIR_KEY_Q).isPressedOrHeld();
    bool E = inputState->keyState(VIR_KEY_E).isPressedOrHeld();
    bool RSHFT = inputState->keyState(VIR_KEY_LEFT_SHIFT).isPressedOrHeld();
    bool SPACE = inputState->keyState(VIR_KEY_SPACE).isPressedOrHeld();
    bool keyPressEnabled = !isEventReceptionPaused(vir::Event::Type::KeyPress);
    if 
    (
        (W || A || S || D || Q || E || RSHFT || SPACE) && 
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
        else if (RSHFT)
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