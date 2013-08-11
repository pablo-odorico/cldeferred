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

    if(!_initialized) {
        initializeOpenGLFunctions();
        _initialized= true;
    } else {
        unbind();
        cleanup();
    }

    // Generate and bind FBO
    glGenFramebuffers(1, &_id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _id);

    // Create COLOR0 buffer
    _diffuseSpecBufferId= attachBuffer(_diffuseSpecFormat, GL_COLOR_ATTACHMENT0);
    // Create COLOR1 buffer
    _normalsBufferId= attachBuffer(_normalsFormat, GL_COLOR_ATTACHMENT1);
    // Create DEPTH buffer
    _depthBufferId= attachBuffer(_depthFormat, GL_DEPTH_ATTACHMENT);

    // Only return true if the framebuffer is fully supported
    return glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void FBO::cleanup()
{
    glDeleteRenderbuffers(1, &_diffuseSpecBufferId);
    glDeleteRenderbuffers(1, &_normalsBufferId);
    glDeleteRenderbuffers(1, &_depthBufferId);

    unbind();
    glDeleteFramebuffers(1, &_id);
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

QImage FBO::diffuseToImage()
{
    QImage image(_width, _height, QImage::Format_RGB32);

    bind(GL_READ_FRAMEBUFFER);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    // Channels are reversed to correct endianness
    glReadPixels(0,0, _width,_height, GL_BGRA, GL_UNSIGNED_BYTE,
                 static_cast<GLvoid*>(image.bits()));
    unbind(GL_READ_FRAMEBUFFER);

    // Return the image vertically-mirrored to correct scanline order
    return image.mirrored();
}

QImage FBO::depthToImage()
{
    QImage image(_width, _height, QImage::Format_RGB32);

    bind(GL_READ_FRAMEBUFFER);
    glReadBuffer(GL_DEPTH_ATTACHMENT);
    // Read float data to the image buffer (sizeof float == sizeof RGB32)
    glReadPixels(0,0, _width,_height, GL_DEPTH_COMPONENT, GL_FLOAT,
                 static_cast<GLvoid*>(image.bits()));
    unbind(GL_READ_FRAMEBUFFER);

    // Convert float values to colors
    for(int y=0; y<_height; y++) {
        for(int x=0; x<_width; x++) {
            const int index= x + y * _width;
            const float df= ((float*)image.bits())[index];
            const uchar d= df * 255.0f;
            ((QRgb*)image.bits())[index]= (df == 1.0f) ? qRgb(128,128,255) : qRgb(d,d,d);
        }
    }
    // Return the image vertically-mirrored to correct scanline order
    return image.mirrored();
}
