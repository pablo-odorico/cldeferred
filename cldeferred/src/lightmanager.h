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

    // Update all OpenCL light structs
    void updateStructs(cl_command_queue queue);
    // Updates the OpenCL struct of a single spot light
    void updateSpotLightStruct(cl_command_queue queue, int index);


private:
    bool _initialized;

    QList<SpotLight*> _spotLights;
    cl_mem _spotStructs; // Array of MaxLights cl_spotlight structs
};

#endif // LIGHTMANAGER_H
