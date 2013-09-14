#ifndef FBO_H
#define FBO_H

#include <GL/glew.h>
#include <QtCore>

#include <QImage>
#include "glutils.h"

class FBO
{
public:
    FBO() : _initialized(false),
        _bindedTarget(GL_NONE), _id(0) { }
    virtual ~FBO() { cleanup(); }

    // resize MUST be called before using the fbo
    virtual bool resize(QSize size,
        QList<GLenum> colorFormats= QList<GLenum>() << GL_RGBA,
        GLenum depthFormat= GL_DEPTH_COMPONENT24);

    void bind(GLenum target= GL_DRAW_FRAMEBUFFER);
    virtual void unbind();
    void clear();

    int width() const { return _size.width(); }
    int height() const { return _size.height(); }
    QSize size() const { return _size; }

    QImage colorAttachImage(int index= 0);
    QImage depthAttachImage();

protected:
    struct Attachment {
        GLenum format;   // eg GL_RGBA
        GLenum target;   // eg GL_COLOR_ATTACHMENT0
        GLenum bufferId; // Returned by glGenRenderbuffers
    };

    virtual void cleanup();
    // Create and attach a buffer object to the FBO
    Attachment createAttach(GLenum format, GLenum target);

    bool _initialized;
    GLenum _bindedTarget;
    GLuint _id;

    QSize _size;

    QVector<Attachment> _colorAttachs;
    Attachment _depthAttach;
};


#endif // FBO_H
