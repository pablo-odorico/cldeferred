#ifndef FBO_H
#define FBO_H

#include <GL/glew.h>
#include <QtCore>

#include <QImage>
#include "glutilfunctions.h"

class FBO
{
public:
    struct Attachment {
        GLenum format;   // eg GL_RGBA
        GLenum target;   // eg GL_COLOR_ATTACHMENT0
        GLenum bufferId; // Returned by glGenRenderbuffers
    };

    FBO() : _width(0), _height(0), _initialized(false),
        _bindedTarget(GL_NONE), _id(0) { }
    virtual ~FBO() { cleanup(); }

    // resize MUST be called before using the fbo
    virtual bool resize(QSize size,
        QList<GLenum> colorFormats= QList<GLenum>() << GL_RGBA,
        GLenum depthFormat= GL_DEPTH_COMPONENT24);

    void bind(GLenum target= GL_DRAW_FRAMEBUFFER);
    virtual void unbind();
    void clear();

    int width() const { return _width; }
    int height() const { return _height; }
    QSize size() const { return QSize(_width, _height); }

    QImage diffuseToImage();
    QImage depthToImage();

protected:
    virtual void cleanup();
    // Create and attach a buffer object to the FBO
    Attachment createAttach(GLenum format, GLenum target);

    bool _initialized;

    int _width;
    int _height;

    GLenum _bindedTarget;

    GLuint _id;

    QVector<Attachment> _colorAttachs;
    Attachment _depthAttach;
};


#endif // FBO_H
