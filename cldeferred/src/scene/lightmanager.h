#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H

#include "clutils.h"
#include "spotlight.h"
#include "dirlight.h"

#include <QtCore>

class LightManager
{
public:
    static const int maxLights = 16;

    LightManager();
    ~LightManager() { qDeleteAll(_spotLights); qDeleteAll(_dirLights); }

    // Init must be called before calling any other methods
    bool init(cl_context context);

    // LightManager will delete all light objects on it's dtor
    void addSpotLight(SpotLight* light);
    SpotLight* spotLight(int i);

    void addDirLight(DirLight* light);
    DirLight* dirLight(int i);

    // Update all OpenCL light structs
    void updateStructs(cl_command_queue queue);
    // Recalculate all of the lights' shadow maps
    void updateShadowMaps(const Scene& scene, cl_command_queue queue);

    // Returns the total number of lights
    int lights() const { return _spotLights.count() + _dirLights.count(); }
    // Returns the number of lights that have shadows
    int lightsWithShadows() const;
    int spotLightCount() const { return _spotLights.count(); }
    int dirLightCount() const { return _dirLights.count(); }

    // OpenCL structs
    cl_mem spotStructs();
    cl_mem dirStructs();

    // OpenCL images of all light depths
    // These buffers SHOULD NOT be acquired and released
    QVector<cl_mem> spotDepths();
    QVector<cl_mem> dirDepths();

private:
    bool _initialized;

    QList<SpotLight*> _spotLights;
    QList<DirLight*>  _dirLights;
    cl_mem _spotStructs; // Array of MaxLights cl_spotlight structs
    cl_mem _dirStructs; // Array of MaxLights cl_dirlight structs
};

#endif // LIGHTMANAGER_H
