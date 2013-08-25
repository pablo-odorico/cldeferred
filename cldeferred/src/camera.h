#ifndef CAMERA_H
#define CAMERA_H

#include <QtGui>

class Camera
{
public:
    enum MovementDirection { Front=0, Right, Back, Left };

    Camera();

    QVector3D position() { return _position; }
    void setPosition(const QVector3D position) { _position= position; updateViewMatrix(); }
    void setPosition(float x, float y, float z) { setPosition(QVector3D(x,y,z)); }
    void setDeltaPosition(const QVector3D delta) { setPosition(_position + delta); }

    float pitch() { return _pitch; }
    void setPitch(float degrees) { _pitch= qBound(-89.9f, degrees, 89.9f); updateViewMatrix(); }
    void setDeltaPitch(float deltaDegrees) { setPitch(_pitch + deltaDegrees); }

    float yaw() { return _yaw; }
    void setYaw(float degrees) { _yaw= fmod(degrees, 360.0f); updateViewMatrix(); }
    void setDeltaYaw(float deltaDegrees) { setYaw(_yaw + deltaDegrees); }

    QVector3D lookVector();
    void lookAt(QVector3D lookPos);
    void lookAt(float x, float y, float z) { lookAt(QVector3D(x,y,z)); }

    void setPerspective(float verticalFOV, float aspectRatio, float near, float far);

    QMatrix4x4 viewMatrix() { return _viewMatrix; }
    QMatrix4x4 projMatrix() { return _projMatrix; }

    // Camera movement
    float moveSpeed() { return _moveSpeed; }
    void setMoveSpeed(float unitsPerSec) { _moveSpeed= unitsPerSec; }

    void resetMovingDir() { _movementFlags= 0; }
    void toggleMovingDir(MovementDirection direction, bool moving);

    void move(float elapsedMsecs);

private:
    void updateViewMatrix();

    float _pitch;
    float _yaw;
    QVector3D _position;

    float _moveSpeed;
    uchar _movementFlags;

    QMatrix4x4 _viewMatrix;
    QMatrix4x4 _projMatrix;
};

#endif // CAMERA_H
