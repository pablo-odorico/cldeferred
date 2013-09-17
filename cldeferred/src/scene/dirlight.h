#ifndef DIRLIGHT_H
#define DIRLIGHT_H

#include "light.h"
#include "cl_dirlight.h"

// Directional Light

class DirLight : public Light
{
public:
    DirLight();

    // Spot parameters
    void setParams(float areaWidth, float areaHeight, float near= 0.1f, float far=100.0f);
    float areaWidth() const { return _areaWidth; }
    float areaHeight() const { return _areaHeight; }
    float near() const { return _near; }
    float far() const { return _far; }

    // Spot direction parameters
    QVector3D position() const { return _lightCamera.position(); }
    void setPosition(const QVector3D position) { _lightCamera.setPosition(position); }

    QVector3D lookVector() const { return _lightCamera.lookVector(); }
    void lookAt(QVector3D lookPos) { _lightCamera.lookAt(lookPos); }
    void lookAt(float x, float y, float z) { lookAt(QVector3D(x,y,z)); }
    void lookAt(QVector3D lookFrom, QVector3D lookTo) { _lightCamera.lookAt(lookFrom, lookTo); }

    float pitch() const { return _lightCamera.pitch(); }
    void setPitch(float degrees) { _lightCamera.setPitch(degrees); }
    void setDeltaPitch(float deltaDegrees) { _lightCamera.setDeltaPitch(deltaDegrees); }

    float yaw() const { return _lightCamera.yaw(); }
    void setYaw(float degrees) { _lightCamera.setYaw(degrees); }
    void setDeltaYaw(float deltaDegrees) { _lightCamera.setDeltaYaw(deltaDegrees); }

    void updateStructCL(cl_command_queue queue, cl_mem buffer, size_t index);

private:
    // Area light in world units
    float _areaWidth;
    float _areaHeight;
    // Clipping params
    float _near;
    float _far;
};

#endif // DIRLIGHT_H
