#include "glutils.h"
#include "debug.h"

bool GLUtils::checkGLErrorFunc(QString msg, const char* file, const int line)
{
    GLenum error= glGetError();
    if(error == GL_NO_ERROR)
        return false;

    const GLubyte* errorString= gluErrorString(error);

    qDebug("%s!! %s:%d:%s OpenGL error '%s': %s.", debugColor(RED), file,
           line, debugColor(DEFAULT), errorString, qPrintable(msg));
    return true;
}
