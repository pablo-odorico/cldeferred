#ifndef CLUTILS_H
#define CLUTILS_H

#include <GL/glew.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <iostream>

class CLUtilFunctions
{
protected:
    // Creates an OpenCL context and queue with support for OpenGL interop
    // The OpenGL context must be created and made current before calling setupOpenCLGL
    //
    // Currently implemented only for Linux/X11
    static
    bool setupOpenCLGL(cl_context& context, cl_command_queue& queue,
                       cl_device_id& device);

    // If error != CL_SUCCESS, checkError shows the error string and returns true
    // If msg != 0, the message is shown in the error report
    static
    bool checkCLError(cl_int error, const char* msg= 0);

    static
    int roundUp(int count, int multiple);

    // Return the msecs of elapsed time in event
    static
    float eventElapsed(cl_event event);

    // Loads a kernel named kernelName from the .cl file in path
    // On compilation errors the compiler message is shown and loadKernel returns false
    static
    bool loadKernel(cl_context context, cl_kernel* kernel, cl_device_id device,
                    const char* path, const char* kernelName);

    static
    std::string clErrorToString(cl_int err);

    static
    void checkProgramBuild(cl_program program, cl_device_id device);

    static
    bool loadProgramText(const char* path, char** text, size_t* length);

    // Converts from an OpenGL format (eg GL_RGBA8) to an OpenCL
    // channel order/channel type pair (eg CL_RGBA, CL_UNORM_INT8)
    // returns false on error
    static
    bool gl2clFormat(GLenum glFormat, cl_channel_order& clOrder, cl_channel_type& clType);
};

#endif // CLUTILS_H
