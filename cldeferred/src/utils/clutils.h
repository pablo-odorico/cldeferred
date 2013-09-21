#ifndef CLUTILS_H
#define CLUTILS_H

#include <GL/glew.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <QtCore>

//
// Convenience macros
//

// Checks the value of cl_int error and if != CL_SUCCESS shows msg, the file and
// line number, and returns TRUE
#define clCheckError(error,msg) \
    CLUtils::checkErrorFunc((error), (msg), __FILE__, __LINE__)

// Returns the default work group size in a safe manner to be used in
// clEnqueueNDRangeKernel
#define clWorkGroup()   CLUtils::defaultWorkGroupFunc().data()
// Returns the rounded-up 2D NDRange size in a safe manner to be used in
// clEnqueueNDRangeKernel. Size should be a QSize.
#define clNDRange(size) CLUtils::ndRangeFunc(size).data()

// Sets the idx-nth kernel argument and checks for errors.
// Returns FALSE on error.
#define clKernelArg(kernel,idx,value) \
    (!clCheckError(clSetKernelArg(kernel, idx, sizeof(value), (void*)&(value)), #kernel " arg " #idx))

// Enqueues kernel into queue, using the default work group size and a rounded-up
// 2D NDRange for size. size should be a QSize.
// Returns FALSE on error.
#define clLaunchKernel(kernel,queue,size) \
    (!clCheckError(clEnqueueNDRangeKernel(queue,kernel,2,NULL,clNDRange(size),clWorkGroup(),0,NULL,NULL), #kernel))

class CLUtils
{
public:
    // Creates an OpenCL context and queue with support for OpenGL interop
    // The OpenGL context must be created and made current before calling setupOpenCLGL
    // NOTE: Currently implemented only for Linux/X11
    static bool setupOpenCLGL(cl_context* context, cl_command_queue* queue, cl_device_id* device);

    //
    // Kernel management
    //
    typedef QMap<QString,QString> KernelDefines;

    // Loads a kernel from a path. Returns 0 (a null pointer) on error.
    static cl_kernel loadKernelPath(
        cl_context context, cl_device_id device, QString programPath, QString kernelName,
        CLUtils::KernelDefines defines= KernelDefines(), QStringList includePaths= QStringList(),
        QString compileOptions= "");

    // Loads a kernel from it's source code. Returns 0 (a null pointer) on error.
    static cl_kernel loadKernelText(
        cl_context context, cl_device_id device, QByteArray programText, QString kernelName,
        CLUtils::KernelDefines defines= KernelDefines(), QStringList includePaths= QStringList(),
        QString compileOptions= "");

    //
    // Execution layout (work group size/NDrange)
    //
    static QSize defaultWorkGroupSize() { return QSize(16, 16); }
    // Use the clWorkGroup macro
    static QSharedPointer<size_t> defaultWorkGroupFunc();
    // Use the clNDRange macro
    static QSharedPointer<size_t> ndRangeFunc(QSize dataSize, QSize workGroup=QSize(0,0));
    // Rounds value to the next multiple of multiple
    static int roundUp(int value, int multiple) {
        return (value + (multiple - 1)) & ~(multiple - 1); }
    static QSize roundUp(QSize value, int multiple) {
        return QSize(roundUp(value.width(), multiple), roundUp(value.height(), multiple)); }

    //
    // Misc
    //
    // Return the msecs of elapsed time in event
    static float eventElapsed(cl_event event);
    // If error != CL_SUCCESS, checkError shows the error string and msg and returns true
    // Use the clCheckError macro
    static bool checkErrorFunc(cl_int error, QString msg, const char* file, const int line);

    //
    // Enum conversion
    //
    static cl_image_format gl2clFormat(GLenum glFormat, bool* error=0);
    static QString clErrorString(cl_int error);
};

#endif // CLUTILS_H
