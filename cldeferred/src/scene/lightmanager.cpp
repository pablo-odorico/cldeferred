#include "lightmanager.h"
#include "scene.h"
#include <cassert>
#include "debug.h"

const int LightManager::maxLights;

LightManager::LightManager()
    : _initialized(false)
{
}

bool LightManager::init(cl_context context)
{
    assert(!_initialized);

    const size_t spotStructsSize= maxLights * sizeof(cl_spotlight);
    cl_int error;
    _spotStructs= clCreateBuffer(context, CL_MEM_READ_ONLY, spotStructsSize,
                                 NULL, &error);
    if(checkCLError(error, "clCreateBuffer spotLights"))
        return false;

    /*
    char data[spotStructsSize];
    memset(data, 0, spotStructsSize);
    error= clEnqueueWriteBuffer(queue, _clMem, CL_FALSE, 0, spotStructsSize,
                             data, 0, NULL, NULL);
    checkCLError(error, ">>> Enqueue write struct");
    */

    _initialized= true;
    return true;
}

void LightManager::updateStructs(cl_command_queue queue)
{
    assert(_initialized);

    // Update spot lights   
    for(int i=0; i<qMin(maxLights, _spotLights.count()); i++)
        _spotLights[i]->updateStructCL(queue, _spotStructs, i);
}

QVector<cl_mem> LightManager::spotDepths()
{
    assert(_initialized);

    QVector<cl_mem> buffers;

    foreach(SpotLight* spot, _spotLights)
        buffers << spot->shadowMapImage();

    return buffers;
}

cl_mem LightManager::spotStructs()
{
    assert(_initialized);
    return _spotStructs;
}

void LightManager::updateShadowMaps(const Scene& scene, cl_command_queue queue)
{
    foreach(SpotLight* spot, _spotLights) {
        if(!spot->hasShadows()) continue;

        if(!spot->updateShadowMap(scene, queue))
            debugWarning("Error updating shadow map.");
    }
}

int LightManager::lightsWithShadows() const
{
    int count= 0;
    foreach(SpotLight* spot, _spotLights)
        count += spot->hasShadows();
    return count;
}
