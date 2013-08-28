#include "lightmanager.h"

LightManager::LightManager()
    : _initialized(false)
{
}

bool LightManager::init(cl_context context)
{
    if(_initialized) {
        qDebug() << "LightManager::init: Already initialized";
        return false;
    }

    cl_int error;
    _spotStructs= clCreateBuffer(context, CL_MEM_READ_ONLY, maxLights * sizeof(cl_spotlight),
                                 NULL, &error);
    if(checkCLError(error, "LightManager::init: clCreateBuffer spotLights"))
        return false;

    _initialized= true;
    return true;
}

void LightManager::updateSpotLightStruct(cl_command_queue queue, int index)
{
    if(index > _spotLights.count()) {
        qDebug() << "LightManager::updateSpotLightStruct: Index out of range.";
        return;
    }

    _spotLights[index]->clUpdateStruct(queue, _spotStructs, index);
}

void LightManager::updateStructs(cl_command_queue queue)
{
    // Update spot lights
    for(int i=0; i<qMin(maxLights, _spotLights.count()); i++)
        _spotLights[i]->clUpdateStruct(queue, _spotStructs, i);
}
