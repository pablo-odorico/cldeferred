#include "lightmanager.h"
#include "scene.h"
#include <cassert>

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
    if(checkCLError(error, "LightManager::init: clCreateBuffer spotLights"))
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

QVector<cl_mem> LightManager::aquireSpotDepths(cl_command_queue queue)
{
    assert(_initialized);

    QVector<cl_mem> buffers;
    buffers.reserve(_spotLights.count());

    foreach(SpotLight* spot, _spotLights) {
        FBOCL& depthFBO= spot->shadowMapFBO();
        QVector<cl_mem> colorBuffers= depthFBO.aquireColorBuffers(queue);
        if(colorBuffers.empty())
            return QVector<cl_mem>();
        else
            buffers << colorBuffers[0];
    }

    return buffers;
}

bool LightManager::releaseSpotDephts(cl_command_queue queue)
{
    assert(_initialized);

    foreach(SpotLight* spot, _spotLights) {
        const bool ok= spot->shadowMapFBO().releaseColorBuffers(queue);
        if(!ok)
            return false;
    }

    return true;
}

cl_mem LightManager::spotStructs()
{
    assert(_initialized);
    return _spotStructs;
}

void LightManager::updateShadowMaps(const Scene& scene)
{
    foreach(SpotLight* spot, _spotLights)
        spot->updateShadowMap(scene);
}
