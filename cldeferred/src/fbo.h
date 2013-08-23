#ifndef FBO_H
#define FBO_H

#include <GL/glew.h>
#include <QtCore>

#include <QImage> // TODO scar

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

    virtual bool init(QSize size,
        QList<GLenum> colorFormats= QList<GLenum>() << GL_RGBA,
        GLenum depthFormat= GL_DEPTH_COMPONENT24);

    void bind(GLenum target= GL_DRAW_FRAMEBUFFER);
    virtual void unbind();
    void clear();

    int width() { return _width; }
    int height() { return _height; }

    QImage depthToImage();
    QImage diffuseToImage();

protected:
    virtual void cleanup();
    // Create and attach a buffer object to the FBO
    Attachment createAttach(GLenum format, GLenum target);

    int _width;
    int _height;

    bool _initialized;
    GLenum _bindedTarget;

    GLuint _id;

    QVector<Attachment> _colorAttachs;
    Attachment _depthAttach;
};


#endif // FBO_H
