#include "fbo.h"

GLuint FBO::attachBuffer(GLenum format, GLenum target)
{
    GLuint id;

    glGenRenderbuffers(1, &id);
    glBindRenderbuffer(GL_RENDERBUFFER, id);
    glRenderbufferStorage(GL_RENDERBUFFER, format, _width, _height);
    // Attach DEPTH buffer
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, target, GL_RENDERBUFFER, id);

    return id;
}

bool FBO::init(QSize size)
{
    _width= size.width();
    _height= size.height();

    initializeOpenGLFunctions();

    // Generate and bind FBO
    glGenFramebuffers(1, &_id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _id);

    // Create DEPTH buffer
    _depthBufferId= attachBuffer(_depthFormat, GL_DEPTH_ATTACHMENT);
    // Create COLOR0 buffer
    _diffuseSpecBufferId= attachBuffer(_diffuseSpecFormat, GL_COLOR_ATTACHMENT0);
    // Create COLOR1 buffer
    _normalsBufferId= attachBuffer(_normalsFormat, GL_COLOR_ATTACHMENT1);

    // Only return true if the framebuffer is fully supported
    return glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

bool FBO::cleanup()
{
    glDeleteRenderbuffers(1, &_depthBufferId);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &_id);

    return true;
}

void FBO::bind(GLenum target)
{
    glBindFramebuffer(target, _id);
}

void FBO::unbind(GLenum target)
{
    glBindFramebuffer(target, 0);
}

void FBO::clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void FBO::drawToScreen(const int& viewportWidth, const int& viewportHeight)
{
    bind();

    // Bind myself for reading
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _id);
    // Bind window FBO for writing
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glViewport(0, 0, viewportWidth, viewportHeight);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Copy

   /* glBlitFramebuffer(
        0, 0, _width, _height,
        0, 0, viewportWidth, viewportHeight,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);*/

    // Remember to swap buffers
}

QImage FBO::colorToImage()
{
    bind(GL_READ_FRAMEBUFFER);


    QImage dst(_width, _height, QImage::Format_RGB32);

    float buf[_width * _height];
    glReadBuffer(GL_DEPTH_ATTACHMENT);
    glReadPixels(0,0, _width,_height, GL_DEPTH_COMPONENT, GL_FLOAT,
                 (GLvoid*)buf);

    for(int y=0; y<_height; y++) {
        for(int x=0; x<_width; x++) {
            const uchar d= buf[x + y * _width] * 255.0f;
            dst.setPixel(x, y, qRgb(d,d,d));
        }
    }



    /*
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0,0, _width,_height, GL_BGRA, GL_UNSIGNED_BYTE,
                 (GLvoid*)dst.bits());
    QTransform transform;
    transform.rotate(180);
    dst= dst.transformed(transform);

    */

    unbind(GL_READ_FRAMEBUFFER);

    return dst;
}
