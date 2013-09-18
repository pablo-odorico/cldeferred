#ifndef SPOTLIGHT_H
#define SPOTLIGHT_H

#include "light.h"
#include "cl_spotlight.h"

class SpotLight : public Light
{
public:
    SpotLight();

    // Spot parameters
    void setParams(float cutOff, float fadeDegrees, float atenuation, float near= 0.1f, float far=100.0f);
    float cutOff() const { return _cutOff; }
    float fadeDegrees() const { return _fadeDegrees; }
    float atenuation() const { return _atenuation; }

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
    float _cutOff;           // Field of view of the light camera, in degrees
    float _fadeDegrees;      // Degrees from the cutoff angle before the spot starts to fade
    float _atenuation;       // Max light distance is 1 / (_linearAtenuation**2)
};

#endif // SPOTLIGHT_H
