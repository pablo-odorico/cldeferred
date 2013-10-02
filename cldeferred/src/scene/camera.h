#ifndef CAMERA_H
#define CAMERA_H

#include <QtGui>

class Camera
{
public:
    enum MovementDirection { Front=0, Right, Back, Left };

    Camera();
    virtual ~Camera() { }

    QVector3D position() const { return _position; }
    void setPosition(const QVector3D position) { _position= position; updateViewMatrix(); }
    void setPosition(float x, float y, float z) { setPosition(QVector3D(x,y,z)); }
    void setDeltaPosition(const QVector3D delta) { setPosition(_position + delta); }

    float pitch() const { return _pitch; }
    void setPitch(float degrees) { _pitch= qBound(-89.99f, degrees, 89.99f); updateViewMatrix(); }
    void setDeltaPitch(float deltaDegrees) { setPitch(_pitch + deltaDegrees); }

    float yaw() const { return _yaw; }
    void setYaw(float degrees) { _yaw= fmod(degrees, 360.0f); updateViewMatrix(); }
    void setDeltaYaw(float deltaDegrees) { setYaw(_yaw + deltaDegrees); }

    QVector3D lookVector() const;
    void lookAt(QVector3D lookPos);
    void lookAt(float x, float y, float z) { lookAt(QVector3D(x,y,z)); }
    void lookAt(QVector3D lookFrom, QVector3D lookTo) { setPosition(lookFrom); lookAt(lookTo); }

    void setPerspective(float verticalFOV, float aspectRatio, float near, float far);
    void setOrthoProjection(float left, float right, float bottom, float top, float near, float far);

    QMatrix4x4 viewMatrix() const { return _viewMatrix; }
    QMatrix4x4 projMatrix() const { return _projMatrix; }

    // Camera movement
    float moveSpeed() const { return _moveSpeed; }
    void setMoveSpeed(float unitsPerSec) { _moveSpeed= unitsPerSec; }

    void resetMovingDir() { _movementFlags= 0; }
    void toggleMovingDir(MovementDirection direction, bool moving);

    void move(float elapsedMsecs);

    // Depth of Field
    void setDoFParams(float focusDistance, float depthOfField,
                      float minClampDist, float maxClampDist,
                      float nearCoC, float farCoC);

protected:
    void updateViewMatrix();

    // Clipping planes
    float _near;
    float _far;

    float _pitch;
    float _yaw;
    QVector3D _position;

    float _moveSpeed;
    uchar _movementFlags;

    QMatrix4x4 _viewMatrix;
    QMatrix4x4 _projMatrix;

    // Depth of Field parameters (in world units)
    float _focusDistance;
    float _depthOfField; // DoF centered in focusDistance
    float _minClampDist;
    float _maxClampDist;
    float _nearCoC; // In normalized screen height units
    float _farCoC;  // In normalized screen height units
};

#endif // CAMERA_H
