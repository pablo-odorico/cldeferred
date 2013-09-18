#ifndef CLUTILFUNCTIONS_H
#define CLUTILFUNCTIONS_H

#include <GL/glew.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <iostream>
#include <QtCore>

#define checkCLError(error,msg) \
    CLUtils::checkCLErrorFunc((error), (msg), __FILE__, __LINE__)

class CLUtils
{
public:
    // Creates an OpenCL context and queue with support for OpenGL interop
    // The OpenGL context must be created and made current before calling setupOpenCLGL
    //
    // Currently implemented only for Linux/X11
    static
    bool setupOpenCLGL(cl_context& context, cl_command_queue& queue,
                       cl_device_id& device);

    // If error != CL_SUCCESS, checkError shows the error string and msg and returns true
    // Call this function using the checkCLError and checkCLError macros defined above
    static
    bool checkCLErrorFunc(cl_int error, const char* msg, const char* file, const int line);

    static
    int roundUp(int count, int multiple);

    // Return the msecs of elapsed time in event
    static
    float eventElapsed(cl_event event);

    // Loads a kernel named kernelName from the .cl file in path
    // On compilation errors the compiler message is shown and loadKernel returns false
    static
    bool loadKernel(cl_context context, cl_kernel* kernel, cl_device_id device,
                    const char* path, const char* kernelName, const char* compileOptions= 0);
    static
    bool loadKernel(cl_context context, cl_kernel* kernel, cl_device_id device,
                    QString programText, const char* kernelName, const char* compileOptions= 0);

    static
    const char* clErrorToString(cl_int err);

    // Converts from an OpenGL format (eg GL_RGBA8) to an OpenCL
    // format (eg CL_RGBA, CL_UNORM_INT8). Returns false on error
    static
    bool gl2clFormat(GLenum glFormat, cl_image_format &clFormat);

private:
    static
    void checkProgramBuild(cl_program program, cl_device_id device);

    static
    bool loadProgramText(const char* path, QByteArray& source);
};

#endif // CLUTILFUNCTIONS_H
