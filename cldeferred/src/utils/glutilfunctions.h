#ifndef GLUTILFUNCTIONS_H
#define GLUTILFUNCTIONS_H

#include <GL/glew.h>

#define checkGLError(msg) GLUtilFunctions::checkGLErrorFunc((msg), __FILE__, __LINE__)

class GLUtilFunctions
{
public:
    static
    bool checkGLErrorFunc(const char* msg, const char* file, const int line);
};

#endif // GLUTILFUNCTIONS_H
