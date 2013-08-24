#ifndef CAMERA_H
#define CAMERA_H

#include <QtGui>

class Camera
{
public:
    enum MovementFlag {
        North = 1 << 0,
        East  = 1 << 1,
        South = 1 << 2,
        West  = 1 << 3
    };

    Camera();

    float pitch() { return _pitch; }
    void setPitch(float degrees) { _pitch= qBound(-180.0f, degrees, 180.0f); updateViewMatrix(); }
    void setDeltaPitch(float deltaDegrees) { setPitch(_pitch + deltaDegrees); }

    float yaw() { return _yaw; }
    void setYaw(float degrees) { _yaw= fmod(degrees, 360.0f); updateViewMatrix(); }
    void setDeltaYaw(float deltaDegrees) { setYaw(_yaw + deltaDegrees); }

    QVector3D position() { return _position; }
    void setPosition(const QVector3D position) { _position= position; updateViewMatrix(); }

    void setProjection(float verticalFOV, float aspectRatio, float near, float far);

    float moveSpeed() { return _moveSpeed; }
    void setMoveSpeed(float speed) { _moveSpeed= speed; }

private:
    void updateViewMatrix();

    float _pitch;
    float _yaw;
    QVector3D _position;

    float _moveSpeed;

    QMatrix4x4 _viewMatrix;
    QMatrix4x4 _projMatrix;
};

#endif // CAMERA_H
