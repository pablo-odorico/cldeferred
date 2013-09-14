#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H

#include "clutilfunctions.h"
#include "spotlight.h"
#include "cl_spotlight.h"

#include <QtCore>

class LightManager : protected CLUtilFunctions
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
    void updateShadowMaps(const Scene& scene);

    // Returns the number of lights that have shadows
    int lightsWithShadows();
    int spotLightCount() { return _spotLights.count(); }

    cl_mem spotStructs();
    QVector<cl_mem> aquireSpotDepths(cl_command_queue queue);
    bool releaseSpotDephts(cl_command_queue queue);


private:
    bool _initialized;

    QList<SpotLight*> _spotLights;
    cl_mem _spotStructs; // Array of MaxLights cl_spotlight structs
};

#endif // LIGHTMANAGER_H
