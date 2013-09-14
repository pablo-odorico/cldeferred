#include "fbo.h"

#include "debug.h"
#include <cassert>


FBO::Attachment FBO::createAttach(GLenum format, GLenum target)
{
    Attachment attach;
    attach.format= format;
    attach.target= target;

    glGenRenderbuffers(1, &attach.bufferId);
    glBindRenderbuffer(GL_RENDERBUFFER, attach.bufferId);
    checkGLError("glGenBuffers");

    glRenderbufferStorage(GL_RENDERBUFFER, attach.format, width(), height());
    checkGLError("Buffer storage.");

    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, attach.target,
                              GL_RENDERBUFFER, attach.bufferId);
    checkGLError("Renderbuffer");

    return attach;
}

bool FBO::resize(QSize size, QList<GLenum> colorFormats, GLenum depthFormat)
{
    assert(colorFormats.count());

    _size= size;
    if(_initialized) {
        unbind();
        cleanup();
        _initialized= false;
    }

    // Generate and bind FBO
    glGenFramebuffers(1, &_id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _id);
    checkGLError("glGenFramebuffers");

    // Create depth attachment
    _depthAttach= createAttach(depthFormat, GL_DEPTH_ATTACHMENT);

    // Create color attachments
    _colorAttachs.resize(colorFormats.count());
    for(int i=0; i<colorFormats.count(); i++) {
        const GLenum format= colorFormats[i];
        const GLenum target= GL_COLOR_ATTACHMENT0 + i;
        _colorAttachs[i]= createAttach(format, target);
    }

    // Only return true if the framebuffer is fully supported
    const GLenum complete= glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    const bool error= complete != GL_FRAMEBUFFER_COMPLETE;
    if(error) {
        debugFatal("Framebuffer not complete: %s.", gluGetString(complete));
    }
    _initialized= !error;

    // Unbind
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    return !error;
}

void FBO::cleanup()
{
    assert(_initialized);

    unbind();

    glDeleteRenderbuffers(1, &_depthAttach.bufferId);

    for(int i=0; i<_colorAttachs.count(); i++)
        glDeleteRenderbuffers(1, &_colorAttachs[i].bufferId);
    _colorAttachs.clear();

    glDeleteFramebuffers(1, &_id);

    checkGLError("Error cleaning-up");
}

void FBO::bind(GLenum target)
{
    assert(_initialized);
    // Clear OpenGL error
    glGetError();

    if(_bindedTarget != GL_NONE and _bindedTarget != target)
        debugWarning("Binded to another target.");

    _bindedTarget= target;
    glBindFramebuffer(target, _id);

    QVector<GLenum> colorTargets(_colorAttachs.count());
    for(int i=0; i<_colorAttachs.count(); i++)
        colorTargets[i]= _colorAttachs[i].target;

    glDrawBuffers(colorTargets.count(), colorTargets.data());

    // Set FBO viewport
    glViewport(0, 0, width(), height());

    checkGLError("Error binding");
}

void FBO::unbind()
{
    assert(_initialized);

    // Clear OpenGL error
    glGetError();

    if(_bindedTarget == GL_NONE) {
        debugWarning("Not binded!");
        return;
    }
    glBindFramebuffer(_bindedTarget, 0);
    _bindedTarget= GL_NONE;
}

void FBO::clear()
{
    assert(_initialized);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/*


QImage FBO::normalsToImage()
{
    float xBuffer[width() * height()];
    float yBuffer[width() * height()];

    bind(GL_READ_FRAMEBUFFER);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glReadPixels(0,0, width(),height(), GL_RED, GL_FLOAT, static_cast<GLvoid*>(xBuffer));
    glReadPixels(0,0, width(),height(), GL_GREEN, GL_FLOAT, static_cast<GLvoid*>(yBuffer));
    unbind(GL_READ_FRAMEBUFFER);

    QImage image(width(), height(), QImage::Format_RGB32);

    // Convert float values to colors
    for(int y=0; y<height(); y++) {
        for(int x=0; x<width(); x++) {
            const int index= x + y * width();
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
*/

QImage FBO::colorAttachImage(int index)
{
    QImage image(width(), height(), QImage::Format_RGB32);

    bind(GL_READ_FRAMEBUFFER);
    glReadBuffer(GL_COLOR_ATTACHMENT0 + index);
    // Channels are reversed to correct endianness
    glReadPixels(0,0, width(),height(), GL_BGRA, GL_UNSIGNED_BYTE,
                 static_cast<GLvoid*>(image.bits()));
    unbind();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Return the image vertically-mirrored to correct the scanline order
    return image.mirrored();

}

QImage FBO::depthAttachImage()
{
    QImage image(width(), height(), QImage::Format_RGB32);

    bind(GL_READ_FRAMEBUFFER);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    // Read float data to the image buffer (sizeof float == sizeof RGB32)
    glReadPixels(0,0, width(),height(), GL_DEPTH_COMPONENT, GL_FLOAT,
                 static_cast<GLvoid*>(image.bits()));
    unbind();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Convert float values to colors
    for(int y=0; y<height(); y++) {
        for(int x=0; x<width(); x++) {
            const int index= x + y * width();
            const float df= ((float*)image.bits())[index];
            const uchar d= df * 255.0f;
            ((QRgb*)image.bits())[index]= (df == 1.0f) ? qRgb(128,128,255) : qRgb(d,d,d);
        }
    }

    // Return the image vertically-mirrored to correct scanline order
    return image.mirrored();
}

