#ifndef LIGHT_H
#define LIGHT_H

#include "fbocl.h"
#include "camera.h"
// Do not include scene.h

#include <QtGui>
#include <Qt3D/QGLSceneNode>

class Scene;

class Light
{
public:
    Light();
    virtual ~Light() { }

    // Shadow mapping
    // 1. Enable
    void enableShadows(bool value);
    bool hasShadows() { return _shadowMapping; }
    // 2. Setup
    bool setupShadowMap(
            cl_context context, QSize shadowMapSize= QSize(512, 512),
            GLenum storedDepthFormat= GL_R32F, // Must be a color format
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
    QColor _ambientColor;
    QColor _diffuseColor;
    QColor _specColor;

    // Shadow mapping
    bool _shadowMapping;
    bool _shadowMappingInit;

    FBOCL _depthFbo;

    // Light "camera" used for shadow mapping
    Camera _lightCamera;
};

#endif // LIGHT_H
