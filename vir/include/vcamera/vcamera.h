#ifndef V_CAMERA_H
#define V_CAMERA_H

#include <thirdparty/glm/glm.hpp>

namespace vir
{

class Camera
{
public:

    enum class YAxisType
    {
        Locked,
        Free
    };

    enum class RotationType
    {
        AroundPosition,
        AroundPivot
    };

    enum class ProjectionType
    {
        Perspective,
        Orthographic
    };

protected:

    // Camera up directiontype (either Locked or Free)
    YAxisType yAxisType_;

    //
    RotationType rotationType_;

    //
    ProjectionType projectionType_;

    //
    bool modifiable_ = true;

    // Camera position
    glm::vec3 position_;

    // Camera pivot for rotation
    glm::vec3 pivot_;

    // What is generally know as "right", though it is really left wrt to the
    // negative "direction"
    glm::vec3 x_;

    // What is generally known as "up"
    glm::vec3 y_;

    // What is generally known as "direction", though it is really the negative
    // of the actual direction in which the camera is looking
    glm::vec3 z_;

    // Field of vision in radians
    float fov_;

    // y size of an orthographic camera viewport
    float viewportHeight_;

    // Near plane (for clipping)
    float nearPlane_;

    // Far plane (for clipping)
    float farPlane_;

    // Projection matrix
    glm::mat4 projectionMatrix_;

    // View-projection matrix
    glm::mat4 projectionViewMatrix_;

    //
    bool zPlusIsLookDirection_;

    //
    bool updated_ = false;

    // Sensitivities (only really relevant for an input camera, but here we go)
    float keySensitivity_ = 10.0; // It really is the camera speed in m / real s
    float mouseSensitivity_ = 0.2f; // 
    float mouseWheelSensitivity_ = 0.95f; //

    // Protected functions
    void updatePositionAroundPiovot();
    void updateProjectionMatrix();

public:

    Camera();
    virtual ~Camera(){}

    template<class CameraType = Camera>
    static Camera* create()
    {
        return new CameraType();
    }
    
    void lockYAxis(){yAxisType_ = YAxisType::Locked;}
    void unlockYAxis(){yAxisType_ = YAxisType::Free;}
    void setRotationType(const RotationType type){rotationType_ = type;}
    void setProjectionType(const ProjectionType);
    void switchProjectionType();
    void setModifiable(const bool flag){modifiable_=flag;}
    void setPosition(const glm::vec3&);
    void setPivot(const glm::vec3&);
    void setDirection(const glm::vec3&);
    void setUp(const glm::vec3&);
    void setFov(const float);
    void setViewportHeight(const float);
    void setPlanes(const float, const float);
    void setKeySensitivity(const float s){keySensitivity_ = s;}
    void setMouseSensitivity(const float s){mouseSensitivity_ = s;}
    void setMouseWheelSensitivity(const float s){mouseWheelSensitivity_ = s;}
    void setZPlusIsLookDirection(bool);
    
    void xRotateBy(const float);
    void yRotateBy(const float);
    void zRotateBy(float);

    float aspectRatio();

    virtual void update();

    YAxisType yAxisType() const {return yAxisType_;}
    RotationType rotationType() const {return rotationType_;}
    ProjectionType projectionType() const {return projectionType_;}
    const glm::vec3& position(){return position_;}
    const glm::vec3& pivot(){return pivot_;}
    const glm::vec3& x(){return x_;}
    const glm::vec3& y(){return y_;}
    const glm::vec3& z(){return z_;}
    const float& fov() const {return fov_;}
    const float& viewportHeight() const {return viewportHeight_; }
    const float& nearPlane() const {return nearPlane_;}
    const float& farPlane() const {return farPlane_;}
    const glm::mat4& projectionViewMatrix(){return projectionViewMatrix_;}
    float& keySensitivityRef(){return keySensitivity_;}
    float& mouseSensitivityRef(){return mouseSensitivity_;}
};

}

#endif