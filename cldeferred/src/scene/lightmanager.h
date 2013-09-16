#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H

#include "clutils.h"
#include "spotlight.h"
#include "cl_spotlight.h"

#include <QtCore>

class LightManager
{
public:
    static const int maxLights = 32;

    LightManager();
    ~LightManager() { qDeleteAll(_spotLights); }

    // Init must be called before calling any other methods
    bool init(cl_context context);

    // LightManager will delete all light objects on it's dtor
    void addSpotLight(SpotLight* spotLight) { _spotLights << spotLight; }
    SpotLight* spotLight(int i) { return _spotLights[i]; }

    // Update all OpenCL light structs
    void updateStructs(cl_command_queue queue);
    // Recalculate all of the lights' shadow maps
    void updateShadowMaps(const Scene& scene, cl_command_queue queue);

    // Returns the number of lights that have shadows
    int lightsWithShadows() const;
    int spotLightCount() const { return _spotLights.count(); }

    cl_mem spotStructs();
    // OpenCL images of all the spot depths
    // These buffers SHOULD NOT be acquired and released
    QVector<cl_mem> spotDepths();

private:
    bool _initialized;

    QList<SpotLight*> _spotLights;
    cl_mem _spotStructs; // Array of MaxLights cl_spotlight structs
};

#endif // LIGHTMANAGER_H
