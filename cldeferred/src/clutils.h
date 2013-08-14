#ifndef CLUTILS_H
#define CLUTILS_H

#include <CL/cl.h>

namespace CLUtils
{

// Creates an OpenCL context and queue with support for OpenGL interop
// The OpenGL context must be created and made current before calling setupOpenCLGL
//
// Currently implemented only for Linux/X11
bool setupOpenCLGL(cl_context& context, cl_command_queue& queue, cl_device_id& device);

// If error != CL_SUCCESS, checkError shows the error string and returns true
// If msg != 0, the message is shown in the error report
bool checkError(cl_int error, const char* msg= 0);

int roundUp(int count, int multiple);

// Return the msecs of elapsed time in event
float eventElapsed(cl_event event);

// Loads a kernel named kernelName from the .cl file in path
// On compilation errors the compiler message is shown and loadKernel returns false
bool loadKernel(cl_context context, cl_kernel* kernel, cl_device_id device, const char* path, const char* kernelName);

}

#endif // CLUTILS_H
