#ifndef FBO_H
#define FBO_H

#include <GL/glew.h>
#include <QtCore>
#include <QImage>

class FBO
{
public:
    FBO() : _initialized(false) { }

    bool init(QSize size);
    void cleanup();

    void bind(GLenum target= GL_DRAW_FRAMEBUFFER);
    void unbind(GLenum target= GL_DRAW_FRAMEBUFFER);
    void clear();

    QImage diffuseToImage();
    QImage normalsToImage();
    QImage depthToImage();

private:
    // Create and attach a buffer object to the FBO, returns the buffer id
    GLuint attachBuffer(GLenum format, GLenum target);

    int _width;
    int _height;
    bool _initialized;

    GLuint _id;

    QVector<GLenum> colorAttachments;

    // NOTICE: if any buffer format changes, update diffuseToImage and depthToImage
    // COLOR0: Diffuse texture sample + Specular power
    GLuint _diffuseSpecBufferId;
    static const GLenum _diffuseSpecFormat= GL_RGBA;
    // COLOR1: Normals in world coords
    GLuint _normalsBufferId;
    static const GLenum _normalsFormat= GL_RG16F;
    // DEPTH buffer
    GLuint _depthBufferId;
    static const GLenum _depthFormat= GL_DEPTH_COMPONENT32F;
};


#endif // FBO_H
