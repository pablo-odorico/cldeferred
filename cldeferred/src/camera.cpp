#include "camera.h"

Camera::Camera() :
    _pitch(0), _yaw(0), _position(0,0,0), _moveSpeed(0.01f)
{
    updateViewMatrix();
    setProjection(60.0f, 16.0f/9.0f, 0.1f, 100.0f);
}

void Camera::updateViewMatrix()
{
    const float theta= _pitch * M_PI / 180.0f;
    const float phi  = _yaw   * M_PI / 180.0f;

    const float lookX= sin(theta) * cos(phi);
    const float lookY= cos(theta);
    const float lookZ= sin(theta) * sin(phi);

    _viewMatrix.setToIdentity();
    _viewMatrix.lookAt(_position, _position + QVector3D(lookX, lookY, lookZ),
                       QVector3D(0,1,0));
}

void Camera::setProjection(float verticalFOV, float aspectRatio, float near, float far)
{
    _projMatrix.setToIdentity();
    _projMatrix.perspective(verticalFOV, aspectRatio, near, far);
}
