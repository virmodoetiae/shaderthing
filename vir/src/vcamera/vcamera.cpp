#include "vpch.h"
#include "vcamera/vcamera.h"

namespace vir
{

Camera::Camera() :
yAxisType_(YAxisType::Locked),
rotationType_(RotationType::AroundPosition),
projectionType_(ProjectionType::Perspective),
position_(0, 0, 0),
pivot_(0, 0, 0),
x_(1, 0, 0),
y_(0, 1, 0),
z_(0, 0, 1),
fov_(glm::radians(45.0f)),
viewportHeight_(1.0f),
nearPlane_(.1),
farPlane_(100.f),
zPlusIsLookDirection_(false)
{
    update();
}

void Camera::setProjectionType(Camera::ProjectionType type)
{
    projectionType_ = type;
    updated_ = false;
}

void Camera::switchProjectionType()
{
    if (projectionType_ == Camera::ProjectionType::Perspective)
        setProjectionType(Camera::ProjectionType::Orthographic);
    else
        setProjectionType(Camera::ProjectionType::Perspective);
}

void Camera::setPosition(glm::vec3& position)
{
    if (!modifiable_)
        return;
    position_ = position;
    updated_ = false;
}

void Camera::setPivot(glm::vec3& pivot)
{
    if (!modifiable_)
        return;
    pivot_ = pivot;
    updated_ = false;
}

void Camera::setDirection(glm::vec3& direction)
{
    if (!modifiable_)
        return;
    if (zPlusIsLookDirection_)
        z_ = direction;
    else
        z_ = -direction;
    x_ = glm::normalize(glm::cross(y_, z_));
    updated_ = false;
}

void Camera::setUp(glm::vec3& up)
{
    if (!modifiable_)
        return;
    y_ = up;
    x_ = glm::normalize(glm::cross(y_, z_));
    updated_ = false;
}

void Camera::setFov(float fov)
{
    if (!modifiable_)
        return;
    fov_ = fov;
    if (projectionType_ == Camera::ProjectionType::Perspective)
        updated_ = false;
}

void Camera::setViewportHeight(float vph)
{
    if (!modifiable_)
        return;
    viewportHeight_ = vph;
    if (projectionType_ == Camera::ProjectionType::Orthographic)
        updated_ = false;
}

void Camera::setPlanes(float near, float far)
{
    if (!modifiable_)
        return;
    nearPlane_ = near;
    farPlane_ = far;
    updated_ = false;
}

void Camera::setZPlusIsLookDirection(bool flag)
{
    if (flag == zPlusIsLookDirection_)
        return;
    x_ = -x_;
    z_ = -z_;
    zPlusIsLookDirection_ = flag;
    updated_ = false;
}

float Camera::aspectRatio()
{
    Window* window;
    if (GlobalPtr<Window>::valid(window))
        return window->aspectRatio();
    return 1.0f;
}

void Camera::xRotateBy(float angle)
{
    if (!modifiable_)
        return;
    if (zPlusIsLookDirection_)
        angle *= -1.0f;
    switch(yAxisType_)
    {
        case YAxisType::Locked :
        {
            float theta = glm::acos(glm::dot(z_, y_)) + angle;
            constexpr float piMinus = PI-0.005f;
            constexpr float zeroPlus = 0.005f;
            if (theta > piMinus || theta < zeroPlus)
                return;
            z_ = glm::rotate(z_, angle, x_);
            break;
        }
        case YAxisType::Free :
        {
            z_ = glm::rotate(z_, angle, x_);
            y_ = glm::normalize(glm::cross(z_, x_));
            break;
        }
    }
    updated_ = false;
}

void Camera::yRotateBy(float angle)
{
    if (!modifiable_)
        return;
    z_ = glm::rotate(z_, angle, y_);
    x_ = glm::rotate(x_, angle, y_);
    updated_ = false;
}

void Camera::zRotateBy(float angle)
{
    if (!modifiable_)
        return;
    if (zPlusIsLookDirection_)
        angle *= -1;
    x_ = glm::rotate(x_, angle, z_);
    y_ = glm::rotate(y_, angle, z_);
    updated_ = false;
}

void Camera::updatePositionAroundPiovot()
{
    if (!modifiable_)
        return;
    if (rotationType_ != RotationType::AroundPivot)
        return;
    float radius = (position_-pivot_).length();

    position_ = pivot_ + (zPlusIsLookDirection_ ? -z_ : z_)*radius;
}

void Camera::updateProjectionMatrix()
{
    float ar = this->aspectRatio();
    switch (projectionType_)
    {
        case (ProjectionType::Perspective) :
        {
            projectionMatrix_ = glm::perspective
            (
                fov_,
                ar,
                nearPlane_,
                farPlane_
            );
            break;
        }

        case (ProjectionType::Orthographic) :
        {
            float hvph(viewportHeight_/2.0);
            projectionMatrix_ = glm::ortho
            (
                -hvph*ar,
                hvph*ar,
                -hvph,
                hvph,
                nearPlane_,
                farPlane_
            );
            break;
        }
    }
}

void Camera::update()
{
    if (updated_)
        return;
    
    updatePositionAroundPiovot();
    
    updateProjectionMatrix();

    projectionViewMatrix_ = projectionMatrix_ * glm::lookAt
    (
        position_,
        position_-(zPlusIsLookDirection_ ? -z_ : z_),
        y_
    );

    updated_ = true;
}

}