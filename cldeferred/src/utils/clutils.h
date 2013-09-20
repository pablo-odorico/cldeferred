#ifndef CLUTILFUNCTIONS_H
#define CLUTILFUNCTIONS_H

#include <GL/glew.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <iostream>
#include <QtCore>

// Checks the value of cl_int error and if != CL_SUCCESS shows msg, the file and
// line number, and returns TRUE
#define checkCLError(error,msg) \
    CLUtils::checkCLErrorFunc((error), (msg), __FILE__, __LINE__)

// Returns the default work group size in a safe manner to be used in
// clEnqueueNDRangeKernel
#define clWorkGroup()   CLUtils::workGroup().data()
// Returns the rounded-up 2D NDRange size in a safe manner to be used in
// clEnqueueNDRangeKernel. Size should be a QSize.
#define clNDRange(size) CLUtils::ndRange(size).data()

// Sets the idx-nth kernel argument and checks for errors.
// Returns FALSE on error.
#define clKernelArg(kernel,idx,value) \
    (!checkCLError(clSetKernelArg(kernel, idx, sizeof(value), (void*)&(value)), #kernel " arg " #idx))

// Enqueues kernel into queue, using the default work group size and a rounded-up
// 2D NDRange for size. size should be a QSize.
// Returns FALSE on error.
#define clLaunchKernel(kernel,queue,size) \
    (!checkCLError(clEnqueueNDRangeKernel(queue,kernel,2,NULL,clNDRange(size),clWorkGroup(),0,NULL,NULL), #kernel))

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

    // Rounds value to the next multiple of multiple
    static
    int roundUp(int value, int multiple) { return (value + (multiple - 1)) & ~(multiple - 1); }

    static
    QSize roundUp(QSize value, int multiple) {
        return QSize(roundUp(value.width(), multiple), roundUp(value.height(), multiple)); }

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

    static
    QSize workGroupSize() {
        return QSize(16, 16);
    }

    static
    QSharedPointer<size_t> workGroup() {
        QSharedPointer<size_t> ret(new size_t[2]);
        ret.data()[0]= workGroupSize().width();
        ret.data()[1]= workGroupSize().width();
        return ret;
    }

    static
    QSharedPointer<size_t> ndRange(QSize dataSize, QSize workGroup=QSize(0,0)) {
        if(!workGroup.width() or !workGroup.height())
            workGroup= workGroupSize();
        QSharedPointer<size_t> ret(new size_t[2]);
        ret.data()[0]= roundUp(dataSize.width() , workGroup.width());
        ret.data()[1]= roundUp(dataSize.height() , workGroup.height());
        return ret;
    }

private:
    static
    void checkProgramBuild(cl_program program, cl_device_id device);

    static
    bool loadProgramText(const char* path, QByteArray& source);
};

#endif // CLUTILFUNCTIONS_H
