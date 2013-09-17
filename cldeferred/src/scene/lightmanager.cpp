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

    cl_int error;

    const size_t spotStructsSize= maxLights * sizeof(cl_spotlight);    
    _spotStructs= clCreateBuffer(context, CL_MEM_READ_ONLY, spotStructsSize, NULL, &error);
    if(checkCLError(error, "clCreateBuffer spotLights"))
        return false;

    const size_t dirStructsSize= maxLights * sizeof(cl_dirlight);
    _dirStructs= clCreateBuffer(context, CL_MEM_READ_ONLY, dirStructsSize, NULL, &error);
    if(checkCLError(error, "clCreateBuffer dirLights"))
        return false;

    _initialized= true;
    return true;
}

void LightManager::addSpotLight(SpotLight* light)
{
    assert(lights() < maxLights);
    _spotLights << light;
}
SpotLight* LightManager::spotLight(int i)
{
    assert(i < _spotLights.count());
    return _spotLights[i];
}

void LightManager::addDirLight(DirLight* light)
{
    assert(lights() < maxLights);
    _dirLights << light;
}
DirLight* LightManager::dirLight(int i)
{
    assert(i < _dirLights.count());
    return _dirLights[i];
}


void LightManager::updateStructs(cl_command_queue queue)
{
    assert(_initialized);

    // Update spot lights   
    for(int i=0; i<_spotLights.count(); i++)
        _spotLights[i]->updateStructCL(queue, _spotStructs, i);

    // Update dir lights
    for(int i=0; i<_dirLights.count(); i++)
        _dirLights[i]->updateStructCL(queue, _dirStructs, i);
}

QVector<cl_mem> LightManager::spotDepths()
{
    assert(_initialized);

    QVector<cl_mem> buffers;
    foreach(SpotLight* light, _spotLights)
        buffers << light->shadowMapImage();

    return buffers;
}

QVector<cl_mem> LightManager::dirDepths()
{
    assert(_initialized);

    QVector<cl_mem> buffers;
    foreach(DirLight* light, _dirLights)
        buffers << light->shadowMapImage();

    return buffers;
}


cl_mem LightManager::spotStructs()
{
    assert(_initialized);
    return _spotStructs;
}

cl_mem LightManager::dirStructs()
{
    assert(_initialized);
    return _dirStructs;
}


void LightManager::updateShadowMaps(const Scene& scene, cl_command_queue queue)
{
    foreach(SpotLight* light, _spotLights) {
        if(!light->hasShadows()) continue;
        if(!light->updateShadowMap(scene, queue))
            debugWarning("Error updating spot shadow map.");
    }

    foreach(DirLight* light, _dirLights) {
        if(!light->hasShadows()) continue;
        if(!light->updateShadowMap(scene, queue))
            debugWarning("Error updating dir shadow map.");
    }
}

int LightManager::lightsWithShadows() const
{
    int count= 0;
    foreach(SpotLight* light, _spotLights)
        count += light->hasShadows();
    foreach(DirLight* light, _dirLights)
        count += light->hasShadows();
    return count;
}
