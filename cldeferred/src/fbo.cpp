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
        //initializeOpenGLFunctions();
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

    // Store which color attachments the fragment shader will write to
    colorAttachments= QVector<GLenum>() << GL_COLOR_ATTACHMENT0 << GL_COLOR_ATTACHMENT1;

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
    glDrawBuffers(colorAttachments.count(), colorAttachments.data());
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

    // Return the image vertically-mirrored to correct the scanline order
    return image.mirrored();
}

QImage FBO::normalsToImage()
{
    float xBuffer[_width * _height];
    float yBuffer[_width * _height];

    bind(GL_READ_FRAMEBUFFER);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glReadPixels(0,0, _width,_height, GL_RED, GL_FLOAT, static_cast<GLvoid*>(xBuffer));
    glReadPixels(0,0, _width,_height, GL_GREEN, GL_FLOAT, static_cast<GLvoid*>(yBuffer));
    unbind(GL_READ_FRAMEBUFFER);

    QImage image(_width, _height, QImage::Format_RGB32);

    // Convert float values to colors
    for(int y=0; y<_height; y++) {
        for(int x=0; x<_width; x++) {
            const int index= x + y * _width;
            // Normal coords converted to fp32
            const float nx= xBuffer[index];
            const float ny= yBuffer[index];
            const float nz= (nx or ny) ? sqrtf(1.0f - nx*nx - ny*ny) : 0.0f;

            // Colors to store the normal
            const uchar nr= qBound(0, (int)(nx * 255.0f), 255);
            const uchar ng= qBound(0, (int)(ny * 255.0f), 255);
            const uchar nb= qBound(0, (int)(nz * 255.0f), 255);

            ((QRgb*)image.bits())[index]= qRgb(nr, ng, nb);
        }
    }

    // Return the image vertically-mirrored to correct the scanline order
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

