#ifndef GLUTILS_H
#define GLUTILS_H

#include <GL/glew.h>
#include <QString>

//
// Convenience macros
//
#define checkGLError(msg) GLUtils::checkGLErrorFunc((msg), __FILE__, __LINE__)

class GLUtils
{
public:
    static
    bool checkGLErrorFunc(QString msg, const char* file, const int line);
};

#endif // GLUTILS_H
