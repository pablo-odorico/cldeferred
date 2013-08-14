#ifndef FBO_H
#define FBO_H

#include <GL/glew.h>
#include <QtCore>
#include <QImage>

class FBO
{
public:
    struct Attachment {
        GLenum format;   // eg GL_RGBA
        GLenum target;   // eg GL_COLOR_ATTACHMENT0
        GLenum bufferId; // returned by glGenRenderbuffers
    };

    FBO() : _initialized(false), _width(0), _height(0),
        _bindedTarget(GL_NONE) { }

    bool init(QSize size,
              QList<GLenum> _colorFormats= QList<GLenum>() << GL_RGBA,
              GLenum _depthFormat= GL_DEPTH_COMPONENT16);
    void cleanup();

    void bind(GLenum target= GL_DRAW_FRAMEBUFFER);
    void unbind();
    void clear();

    QImage diffuseToImage();
    QImage normalsToImage();
    QImage depthToImage();

private:
    // Create and attach a buffer object to the FBO
    Attachment createAttach(GLenum format, GLenum target);

    int _width;
    int _height;
    bool _initialized;
    GLenum _bindedTarget;

    GLuint _id;

    Attachment _depthAttach;
    QVector<Attachment> _colorAttachs;

/*
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
*/
};


#endif // FBO_H
