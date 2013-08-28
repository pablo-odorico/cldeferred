#include "camera.h"

Camera::Camera()
    : _pitch(0), _yaw(0), _position(0,0,0),
    _moveSpeed(1.0f), _movementFlags(0)
{
    updateViewMatrix();
    setPerspective(60.0f, 16.0f/9.0f, 0.01f, 100.0f);
}

void Camera::updateViewMatrix()
{
    _viewMatrix.setToIdentity();
    _viewMatrix.lookAt(_position, _position + lookVector(), QVector3D(0,1,0));
}

void Camera::setPerspective(float verticalFOV, float aspectRatio, float near, float far)
{
    _projMatrix.setToIdentity();
    _projMatrix.perspective(verticalFOV, aspectRatio, near, far);
}

void Camera::setOrthoProjection(float left, float right, float bottom, float top, float near, float far)
{
    _projMatrix.setToIdentity();
    _projMatrix.ortho(left, right, bottom, top, near, far);
}

QVector3D Camera::lookVector() const
{
    // The output vector will have a length of 1
    // http://en.wikipedia.org/wiki/Spherical_coordinate_system

    const float theta= (90.0f - _pitch) * M_PI / 180.0f;
    const float phi  = (90.0f - _yaw  ) * M_PI / 180.0f;

    const float lookX= sinf(theta) * cosf(phi);
    const float lookY= cosf(theta);
    const float lookZ= sinf(theta) * sinf(phi);
    return QVector3D(lookX, lookY, lookZ);
}

void Camera::lookAt(QVector3D lookPos)
{
    // Reconstruct pitch/yaw from the look direction
    const QVector3D lookDir= (lookPos - _position).normalized();

    const float theta= acosf(lookDir.z());
    const float phi= atan2f(lookDir.y(), lookDir.x());

    _pitch= 90.0f - (theta * 180.0f / M_PI);
    _yaw  = 90.0f - (phi   * 180.0f / M_PI);

    updateViewMatrix();
}

void Camera::toggleMovingDir(MovementDirection direction, bool moving)
{
    // Set bit to 0
    _movementFlags &= ~(1 << direction);
    // Set bit to moving
    _movementFlags |= moving << direction;
}

void Camera::move(float elapsedMsecs)
{
    // Quit if the camera is not moving in any direction
    if(!_movementFlags)
        return;

    const bool front= _movementFlags & (1 << Front);
    const bool right= _movementFlags & (1 << Right);
    const bool back = _movementFlags & (1 << Back );
    const bool left = _movementFlags & (1 << Left );

    const QVector3D lookDir= lookVector();

    // For left/right sidestep, rotate the XZ projection of lookDir
    QVector3D sideStep(0, 0, 0);
    if(right xor left) {
        QTransform transform;
        transform.rotate(90.0f * (right-left));
        const QPointF rotated= transform.map(QPointF(lookDir.x(), lookDir.z()));
        sideStep= QVector3D(rotated.x(), 0, rotated.y()).normalized();
    }
    const QVector3D moveDir= ((front-back) * lookDir + sideStep).normalized();

    _position += elapsedMsecs/1e6d * _moveSpeed * moveDir;

    updateViewMatrix();
}
