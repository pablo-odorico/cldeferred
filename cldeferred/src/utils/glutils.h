#ifndef GLUTILFUNCTIONS_H
#define GLUTILFUNCTIONS_H

#include <GL/glew.h>

#define checkGLError(msg) GLUtils::checkGLErrorFunc((msg), __FILE__, __LINE__)

class GLUtils
{
public:
    static
    bool checkGLErrorFunc(const char* msg, const char* file, const int line);
};

#endif // GLUTILFUNCTIONS_H
