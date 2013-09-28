#include "clutils.h"
#include "debug.h"

#include <QVector2D>

// X11 OpenGL functions
#include <GL/glx.h>

bool CLUtils::setupOpenCLGL(cl_context* context, cl_command_queue* queue, cl_device_id* device)
{
    cl_int error;

    // Get platform info
    cl_platform_id platform;
    error= clGetPlatformIDs(1, &platform, NULL);
    if(clCheckError(error, "clGetPlatformIDs"))
        return false;

    // Create context with OpenGL support
    cl_context_properties props[] =  {
        CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
        CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
        0
    };

    error= clGetGLContextInfoKHR(props, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR,
                                 sizeof(*device), device, NULL);
    if(clCheckError(error, "clGetGLContextInfoKHR"))
        return false;
/*

    // Select default GPU
    cl_device_id devs[2];
    error= clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 2, devs, NULL);
    *device= devs[1];
//    error= clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, device, NULL);
    if(clCheckError(error, "clGetDeviceIDs"))
        return false;
*/
    *context= clCreateContext(props, 1, device, NULL, NULL, &error);
    if(clCheckError(error, "clCreateContext"))
        return false;

    // Create command queue for the selected GPU
    *queue= clCreateCommandQueue(*context, *device, CL_QUEUE_PROFILING_ENABLE, &error);
    if(clCheckError(error, "clCreateCommandQueue"))
        return false;

    clInfo.context= *context;
    clInfo.device= *device;
    clInfo.queue= *queue;

    return true;
}


cl_kernel CLUtils::loadKernelPath(
    cl_context context, cl_device_id device, QString programPath, QString kernelName,
    CLUtils::KernelDefines defines, QStringList includePaths, QString compileOptions)
{
    QFile file(programPath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        debugWarning("Could not open path %s.", qPrintable(programPath));
        return 0;
    }
    const QByteArray programText= file.readAll();
    return loadKernelText(context, device, programText, kernelName, defines,
                          includePaths, compileOptions);
}

cl_kernel CLUtils::loadKernelText(
    cl_context context, cl_device_id device, QByteArray programText, QString kernelName,
    CLUtils::KernelDefines defines, QStringList includePaths, QString compileOptions)
{
    cl_int error;

    // Create program
    char* programData= programText.data();
    size_t programLenght= programText.length();
    cl_program program;
    program= clCreateProgramWithSource(context, 1, (const char **)&programData,
                                       (const size_t *)&programLenght, &error);
    if(clCheckError(error, "Create program for kernel " + kernelName))
        return 0;

    // Build compile options
    compileOptions += " -Werror";
    foreach(QString path, includePaths)
        compileOptions += " -I" + path;
    QHashIterator<QString,QString> i(defines);
    while(i.hasNext()) {
        i.next();
        const QString name= i.key();
        const QString value= i.value();
        compileOptions += " -D " + name + "=" + (value.isEmpty() ? "1" : value);
    }

    // Compile program
    // setenv("CUDA_CACHE_DISABLE", "1", 1);
    error= clBuildProgram(program, 1, &device, compileOptions.toLatin1(), 0, 0);

    cl_build_status build_status;
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &build_status, NULL);

    if(clCheckError(error, "Kernel '" + kernelName + "'")) {
        size_t logSize;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
        logSize++;
        QByteArray logText(logSize, Qt::Uninitialized);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, logText.data(), NULL);

        qDebug("%s********************************************************%s", debugColor(RED), debugColor(DEFAULT));
        qDebug("%s", qPrintable(logText));
        qDebug("%s********************************************************%s", debugColor(RED), debugColor(DEFAULT));

        clReleaseProgram(program);
        return 0;
    }

    // Create a kernel from the program (a program may contain many kernels)
    cl_kernel kernel= clCreateKernel(program, kernelName.toLatin1(), &error);
    if(clCheckError(error, "Creating kernel " + kernelName)) {
        clReleaseProgram(program);
        return 0;
    }

    clReleaseProgram(program);
    return kernel;
}



QSharedPointer<size_t> CLUtils::defaultWorkGroupFunc()
{
    QSharedPointer<size_t> ret(new size_t[2]);
    ret.data()[0]= defaultWorkGroupSize().width();
    ret.data()[1]= defaultWorkGroupSize().width();
    return ret;
}

QSharedPointer<size_t> CLUtils::ndRangeFunc(QSize dataSize, QSize workGroup)
{
    if(!workGroup.width() or !workGroup.height())
        workGroup= defaultWorkGroupSize();
    QSharedPointer<size_t> ret(new size_t[2]);
    ret.data()[0]= roundUp(dataSize.width() , workGroup.width());
    ret.data()[1]= roundUp(dataSize.height() , workGroup.height());
    return ret;
}

bool CLUtils::checkErrorFunc(cl_int error, QString msg, const char* file, const int line)
{
    if(error == CL_SUCCESS)
        return false;

    qDebug("%s!! %s:%d:%s OpenCL error '%s': %s.", debugColor(RED), file, line,
           debugColor(DEFAULT), qPrintable(clErrorString(error)), qPrintable(msg));
    return true;
}

inline uint qHash(const QSize& size, uint)
{
    return qHash((qint64)size.width() + ((qint64)(size.height()) << 32));
}

cl_mem CLUtils::gaussianKernel(cl_context context, cl_command_queue queue, QSize size)
{
    static QHash<QSize,cl_mem> kernels;

    if(kernels.contains(size))
        return kernels[size];

    cl_int error;

    const size_t count= size.width() * size.height();
    const size_t bytes= count * sizeof(float);

    debugMsg("Creating gaussian kernel of %d x %d.", size.width(), size.height());
    cl_mem kernel= clCreateBuffer(context, CL_MEM_READ_ONLY, bytes, NULL, &error);
    if(clCheckError(error, "clCreateBuffer"))
        return 0;

    float sum= 0;
    float weights[count];
    for(int y=0; y<size.height(); y++) {
        for(int x=0; x<size.width(); x++) {
            const QVector2D vec((float)x/size.width()-0.5f,(float)y/size.height()-0.5f);
            const float weight= exp(-10.0f * vec.lengthSquared());
            weights[x + y * size.width()]= weight;
            sum += weight;
        }
    }
    // Normalize
    for(uint i=0; i<count; i++)
        weights[i] /= sum;
/*
    for(int y=0; y<size.height(); y++)
        for(int x=0; x<size.width(); x++)
            qDebug("%d %d %f", x,y, weights[x + y * size.width()]);
*/

    error= clEnqueueWriteBuffer(queue, kernel, CL_TRUE, 0, bytes, weights, 0, NULL, NULL);
    if(clCheckError(error, "clEnqueueWriteBuffer"))
        return 0;

    kernels[size]= kernel;
    return kernel;
}

QImage CLUtils::image(cl_context context, cl_device_id device, cl_command_queue queue,
                        cl_mem image, bool saveAlpha)
{
    cl_int error;

    // Get image parameteres
    cl_image_format format;
    size_t width;
    size_t height;
    size_t pitch;

    error  = clGetImageInfo(image, CL_IMAGE_FORMAT, sizeof(cl_image_format), &format, 0);
    error |= clGetImageInfo(image, CL_IMAGE_WIDTH, sizeof(size_t), &width, 0);
    error |= clGetImageInfo(image, CL_IMAGE_HEIGHT, sizeof(size_t), &height, 0);
    error |= clGetImageInfo(image, CL_IMAGE_ROW_PITCH, sizeof(size_t), &pitch, 0);
    if(clCheckError(error, "clGetImageInfo"))
        return QImage();

    // If a "conversion image" of that size is not allocated, create one.
    QSize size(width, height);
    static QHash<QSize, cl_mem> convImages;

    if(!convImages.contains(size)) {
        debugMsg("Creating a new conversion buffer for size %d x %d.", (int)width, (int)height);
        convImages[size]= clCreateImage2D(context, CL_MEM_WRITE_ONLY, clFormatGL(GL_BGRA),
                                          width, height, 0, 0, &error);
        if(clCheckError(error, "Creating conversion image")) {
            convImages.remove(size);
            return QImage();
        }

    }
    cl_mem& convImage= convImages[size];
    size_t origin[3]= { 0, 0, 0 };
    size_t region[3]= { width, height, 1 };

    // If the conversion kernel is not allocated, create it.
    static cl_kernel convKernel= 0;
    if(!convKernel) {
        debugMsg("Creating conversion kernel.");
        const char* convKernelText= \
        "kernel void conv(read_only image2d_t src, write_only image2d_t dst) {  "
        "    int2 p= (int2)(get_global_id(0), get_global_id(1));                  "
        "    int2 size= get_image_dim(dst);                                       "
        "    if(p.x >= size.x || p.y >= size.y) return;                           "
        "    sampler_t sampler= CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;"
        "    float4 color= read_imagef(src, sampler, (int2)(p.x,size.y-1-p.y));   "
        "    if(get_image_channel_order(src)==CLK_R) color.yzw= (float3)(color.x);"
        "    write_imagef(dst, p, color);                                         "
        "}                                                                      ";

        convKernel= loadKernelText(context, device, convKernelText, "conv");
        if(!convKernel)
            return QImage();
    }

    // Run conversion kernel
    clKernelArg(convKernel, 0, image);
    clKernelArg(convKernel, 1, convImage);
    if(!clLaunchKernel(convKernel, queue, size))
        return QImage();

    // Download the conversion image into a new QImage. The read operation is blocking.
    QImage ret(width, height, saveAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);

    error= clEnqueueReadImage(queue, convImage, CL_TRUE, origin, region, width * 4,
                              0, (void*)ret.bits(), 0,0,0);
    if(clCheckError(error, "Image download"))
        return QImage();

    debugMsg("Done converting to QImage.");
    return ret;
}

float CLUtils::eventElapsed(cl_event event)
{
    cl_int error;

    cl_ulong startTime;
    cl_ulong endTime;
    error  = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &startTime, NULL);
    error |= clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &endTime, NULL);
    clCheckError(error, "clGetEventProfilingInfo");

    return (endTime - startTime) * 1.0e-6f; // in ms.
}


QSharedPointer<cl_image_format> CLUtils::clFormatGLFunc(GLenum glFormat)
{
    bool error;
    cl_image_format format= gl2clFormat(glFormat, &error);
    if(error)
        return QSharedPointer<cl_image_format>(0);
    QSharedPointer<cl_image_format> ret(new cl_image_format);
    *ret.data()= format;
    return ret;
}

cl_image_format CLUtils::gl2clFormat(GLenum glFormat, bool* error)
{
    cl_image_format clFormat;

    if(error) *error= false;

    switch(glFormat) {
    // Some of the format mappings listed in the standard
    case GL_RGBA:
    case GL_RGBA8:
        clFormat.image_channel_order    = CL_RGBA;
        clFormat.image_channel_data_type= CL_UNORM_INT8;
        break;
    case GL_BGRA:
        clFormat.image_channel_order    = CL_BGRA;
        clFormat.image_channel_data_type= CL_UNORM_INT8;
        break;
    case GL_RGBA16:
        clFormat.image_channel_order    = CL_RGBA;
        clFormat.image_channel_data_type= CL_UNORM_INT16;
        break;
    case GL_RGBA16F:
        clFormat.image_channel_order    = CL_RGBA;
        clFormat.image_channel_data_type= CL_HALF_FLOAT;
        break;

    // OpenCL 1.2 formats
    case GL_DEPTH_COMPONENT16:
        clFormat.image_channel_order    = CL_DEPTH;
        clFormat.image_channel_data_type= CL_UNORM_INT16;
        break;
    case GL_DEPTH_COMPONENT32F:
        clFormat.image_channel_order    = CL_DEPTH;
        clFormat.image_channel_data_type= CL_FLOAT;
        break;

    // Format mappings not listed in the standard
    case GL_R:
        clFormat.image_channel_order    = CL_R;
        clFormat.image_channel_data_type= CL_UNORM_INT8;
        break;
    case GL_R16F:
        clFormat.image_channel_order    = CL_R;
        clFormat.image_channel_data_type= CL_HALF_FLOAT;
        break;
    case GL_R32F:
        clFormat.image_channel_order    = CL_R;
        clFormat.image_channel_data_type= CL_FLOAT;
        break;
    case GL_RG16F:
        clFormat.image_channel_order    = CL_RG;
        clFormat.image_channel_data_type= CL_HALF_FLOAT;
        break;
    case GL_RG32F:
        clFormat.image_channel_order    = CL_RG;
        clFormat.image_channel_data_type= CL_FLOAT;
        break;

    // Unknown format
    default:
        debugWarning("Could not convert format %X.", glFormat);
        if(error) *error= true;
    }

    return clFormat;
}

QString CLUtils::clErrorString(cl_int error)
{
    // OpenCL 1.2 enums
    switch (error) {
    case CL_SUCCESS:                            return "Success!";
    case CL_DEVICE_NOT_FOUND:                   return "Device not found.";
    case CL_DEVICE_NOT_AVAILABLE:               return "Device not available";
    case CL_COMPILER_NOT_AVAILABLE:             return "Compiler not available";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "Memory object allocation failure";
    case CL_OUT_OF_RESOURCES:                   return "Out of resources";
    case CL_OUT_OF_HOST_MEMORY:                 return "Out of host memory";
    case CL_PROFILING_INFO_NOT_AVAILABLE:       return "Profiling information not available";
    case CL_MEM_COPY_OVERLAP:                   return "Memory copy overlap";
    case CL_IMAGE_FORMAT_MISMATCH:              return "Image format mismatch";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
    case CL_BUILD_PROGRAM_FAILURE:              return "Program build failure";
    case CL_MAP_FAILURE:                        return "Map failure";
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:       return "Misaligned sub buffer offset";
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return "Exec status error for events in wait list";
    case CL_COMPILE_PROGRAM_FAILURE:             return "Compile program failure";
    case CL_LINKER_NOT_AVAILABLE:                return "Linker not available";
    case CL_LINK_PROGRAM_FAILURE:                return "Link program failure";
    case CL_DEVICE_PARTITION_FAILED:             return "Device partition failed";
    case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:       return "Kernel arg info not available";
    case CL_INVALID_VALUE:                      return "Invalid value";
    case CL_INVALID_DEVICE_TYPE:                return "Invalid device type";
    case CL_INVALID_PLATFORM:                   return "Invalid platform";
    case CL_INVALID_DEVICE:                     return "Invalid device";
    case CL_INVALID_CONTEXT:                    return "Invalid context";
    case CL_INVALID_QUEUE_PROPERTIES:           return "Invalid queue properties";
    case CL_INVALID_COMMAND_QUEUE:              return "Invalid command queue";
    case CL_INVALID_HOST_PTR:                   return "Invalid host pointer";
    case CL_INVALID_MEM_OBJECT:                 return "Invalid memory object";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "Invalid image format descriptor";
    case CL_INVALID_IMAGE_SIZE:                 return "Invalid image size";
    case CL_INVALID_SAMPLER:                    return "Invalid sampler";
    case CL_INVALID_BINARY:                     return "Invalid binary";
    case CL_INVALID_BUILD_OPTIONS:              return "Invalid build options";
    case CL_INVALID_PROGRAM:                    return "Invalid program";
    case CL_INVALID_PROGRAM_EXECUTABLE:         return "Invalid program executable";
    case CL_INVALID_KERNEL_NAME:                return "Invalid kernel name";
    case CL_INVALID_KERNEL_DEFINITION:          return "Invalid kernel definition";
    case CL_INVALID_KERNEL:                     return "Invalid kernel";
    case CL_INVALID_ARG_INDEX:                  return "Invalid argument index";
    case CL_INVALID_ARG_VALUE:                  return "Invalid argument value";
    case CL_INVALID_ARG_SIZE:                   return "Invalid argument size";
    case CL_INVALID_KERNEL_ARGS:                return "Invalid kernel arguments";
    case CL_INVALID_WORK_DIMENSION:             return "Invalid work dimension";
    case CL_INVALID_WORK_GROUP_SIZE:            return "Invalid work group size";
    case CL_INVALID_WORK_ITEM_SIZE:             return "Invalid work item size";
    case CL_INVALID_GLOBAL_OFFSET:              return "Invalid global offset";
    case CL_INVALID_EVENT_WAIT_LIST:            return "Invalid event wait list";
    case CL_INVALID_EVENT:                      return "Invalid event";
    case CL_INVALID_OPERATION:                  return "Invalid operation";
    case CL_INVALID_GL_OBJECT:                  return "Invalid OpenGL object";
    case CL_INVALID_BUFFER_SIZE:                return "Invalid buffer size";
    case CL_INVALID_MIP_LEVEL:                  return "Invalid mip-map level";
    case CL_INVALID_GLOBAL_WORK_SIZE:			return "Invalid global work size";
    case CL_INVALID_PROPERTY:                   return "Invalid property";
    case CL_INVALID_IMAGE_DESCRIPTOR:           return "Invalid image descriptor";
    case CL_INVALID_COMPILER_OPTIONS:           return "Invalid compiler options";
    case CL_INVALID_LINKER_OPTIONS:             return "Invalid linker options";
    case CL_INVALID_DEVICE_PARTITION_COUNT:     return "Invalid device partition count";
    default: return "Unknown";
    }
}

