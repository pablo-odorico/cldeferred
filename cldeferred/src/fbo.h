#ifndef FBO_H
#define FBO_H

#include <QtCore>
#include <QtGui/QOpenGLFunctions>
#include <QImage>

class FBO : protected QOpenGLFunctions
{
public:
    FBO() : QOpenGLFunctions() { }
    bool init(QSize size);
    bool cleanup();

    void bind(GLenum target= GL_DRAW_FRAMEBUFFER);
    void unbind(GLenum target= GL_DRAW_FRAMEBUFFER);
    void clear();
    void drawToScreen(const int& viewportWidth, const int& viewportHeight);

    QImage colorToImage();

private:
    // Create and attach a buffer object to the FBO, returns the buffer id
    GLuint attachBuffer(GLenum format, GLenum target);

    int _width;
    int _height;

    GLuint _id;

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
