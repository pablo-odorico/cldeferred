#ifndef LIGHT_H
#define LIGHT_H

#include "fbocl.h"
#include "camera.h"
// Do not include scene.h

#include <QtGui>
#include <Qt3D/QGLSceneNode>

class Scene;

class Light : protected CLUtilFunctions
{
public:
    Light();
    virtual ~Light() { }

    // Shadow mapping
    // 1. Enable
    void enableShadows(bool value) { _shadowMapping= value; }
    bool hasShadows() { return _shadowMapping; }
    // 2. Setup
    bool setupShadowMap(cl_context context, QSize shadowMapSize= QSize(512, 512),
            GLenum storedDepthFormat= GL_RG32F, // Must be a color format, we use two channels per pixel
            GLenum depthTestingFormat= GL_DEPTH_COMPONENT24); // Must be a depth format
    // 3. Update shadow map
    void updateShadowMap(const Scene& scene);
    // 4. Use FBO image
    FBOCL& shadowMapFBO() { return _depthFbo; }

    QColor ambientColor() const { return _ambientColor; }
    void setAmbientColor(QColor color) { _ambientColor= color; }

    QColor diffuseColor() const { return _diffuseColor; }
    void setDiffuseColor(QColor color) { _diffuseColor= color; }

    QColor specColor() const { return _specColor; }
    void setSpecColor(QColor color) { _specColor= color; }

protected:
    // Returns the depth downsample size for a certain level
    // If level==0, returns the depth fbo size
    //QSize depthDownsampleSize(int level);

    QColor _ambientColor;
    QColor _diffuseColor;
    QColor _specColor;

    // Shadow mapping
    bool _shadowMapping;
    bool _shadowMappingInit;

    // FBO for the depth moments
    FBOCL _depthFbo;
    // OpenCL images for the depth moments downsamples
    //QVector<cl_mem> _depthDownsamples;

    // Light "camera" used for shadow mapping
    Camera _lightCamera;
};

#endif // LIGHT_H
