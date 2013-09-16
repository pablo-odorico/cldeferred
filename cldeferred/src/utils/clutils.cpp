#include "clutils.h"

#include <iostream>
#include <fstream>
#include "debug.h"

// X11 OpenGL functions
#include <GL/glx.h>

using namespace std;

bool CLUtils::setupOpenCLGL(cl_context& context, cl_command_queue& queue, cl_device_id &device)
{
    cl_int clError;

    // Get platform info
    cl_platform_id platform;
    clError= clGetPlatformIDs(1, &platform, NULL);
    if(checkCLError(clError, "clGetPlatformIDs"))
        return false;

    // Select default GPU
    cl_device_id devs[2];
    clError= clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 2, devs, NULL);
    device= devs[1];
    //clError= clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if(checkCLError(clError, "clGetDeviceIDs"))
        return false;

    // Create context with OpenGL support
    cl_context_properties props[] =  {
        CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
        CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
        0
    };
    context= clCreateContext(props, 1, &device, NULL, NULL, &clError);

    if(checkCLError(clError, "clCreateContext"))
        return false;

    // Create command queue for the selected GPU
    queue= clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &clError);
    if(checkCLError(clError, "clCreateCommandQueue"))
        return false;

    return true;
}

const char* CLUtils::clErrorToString(cl_int err)
{
    // OpenCL 1.2 enums
    switch (err) {
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

bool CLUtils::checkCLErrorFunc(cl_int error, const char* msg, const char* file, const int line)
{
    if(error == CL_SUCCESS)
        return false;

    qDebug("%s!! %s:%d:%s OpenCL error '%s' %s.", debugColor(RED), file,
           line, debugColor(DEFAULT), clErrorToString(error), msg);
    return true;
}

int CLUtils::roundUp(int count, int multiple)
{
    int r = count % multiple;
    if(!r)
        return count;
    else
        return count + multiple - r;
}

float CLUtils::eventElapsed(cl_event event)
{
    cl_int error;

    cl_ulong start_time, end_time;
    error = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start_time, NULL);
    error |= clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end_time, NULL);
    checkCLError(error, "eventElapsed: clGetEventProfilingInfo");

    return float(end_time - start_time) * 1.0e-6f; // in ms.
}

bool CLUtils::loadProgramText(const char* path, QByteArray& source)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    source= file.readAll();
    return true;
}

void CLUtils::checkProgramBuild(cl_program program, cl_device_id device)
{
    cl_int clError;
    cl_build_status build_status;
    clError = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                    sizeof(cl_build_status), &build_status, NULL);
    checkCLError(clError, "checkProgramBuild: clGetProgramBuildInfo");


    char *build_log;
    size_t ret_val_size;
    clError = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                    0, NULL, &ret_val_size);
    checkCLError(clError, "checkProgramBuild: clGetProgramBuildInfo");

    build_log = new char[ret_val_size+1];
    clError = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                    ret_val_size, build_log, NULL);
    checkCLError(clError, "checkProgramBuild: clGetProgramBuildInfo");

    // end of string character
    build_log[ret_val_size] = '\0';

    if (build_status != CL_BUILD_SUCCESS) {
        cerr << "Error building program:" << endl;
        cerr << build_log << endl;
    }

}

bool CLUtils::loadKernel(cl_context context, cl_kernel* kernel,
                         cl_device_id device, QString programText, const char* kernelName,
                         const char* compileOptions)
{
    // Create program
    cl_int error;
    cl_program program;
    QByteArray programTextData= programText.toLatin1();
    char* programData= programTextData.data();
    size_t programLenght= programTextData.length();
    program= clCreateProgramWithSource(context, 1, (const char **)&programData,
                                       (const size_t *)&programLenght, &error);
    if(checkCLError(error, "loadKernel: clCreateProgramWithSource"))
        return false;
    // Compile program for all the GPUs in the context
    error= clBuildProgram(program, 0, NULL, compileOptions, NULL, NULL);
    if(checkCLError(error, "loadKernel: clBuildProgram")) {
        checkProgramBuild(program, device);
        return false;
    }

    // Create a kernel from the program (a program may contain many kernels)
    *kernel= clCreateKernel(program, kernelName, &error);
    if(checkCLError(error, "loadKernel: clCreateKernel"))
        return false;

    clReleaseProgram(program);

    return true;
}

bool CLUtils::loadKernel(cl_context context, cl_kernel* kernel,
                                 cl_device_id device, const char* path, const char* kernelName,
                                 const char* compileOptions)
{
    // Load program text into a string
    QByteArray programText;
    if(!loadProgramText(path, programText)) {
        cerr << "Error loading program text." << endl;
        return false;
    }
    return loadKernel(context, kernel, device, QString(programText), kernelName, compileOptions);
}


bool CLUtils::gl2clFormat(GLenum glFormat, cl_image_format& clFormat)
{
    switch(glFormat) {
    // Some of the format mappings listed in the standard
    case GL_RGBA:
    case GL_RGBA8:
        clFormat.image_channel_order= CL_RGBA;
        clFormat.image_channel_data_type= CL_UNORM_INT8;
        break;
    case GL_BGRA:
        clFormat.image_channel_order= CL_BGRA;
        clFormat.image_channel_data_type= CL_UNORM_INT8;
        break;
    case GL_RGBA16:
        clFormat.image_channel_order= CL_RGBA;
        clFormat.image_channel_data_type= CL_UNORM_INT16;
        break;
    case GL_RGBA16F:
        clFormat.image_channel_order= CL_RGBA;
        clFormat.image_channel_data_type= CL_HALF_FLOAT;
        break;
    case GL_DEPTH_COMPONENT16:
        clFormat.image_channel_order= CL_DEPTH;
        clFormat.image_channel_data_type= CL_UNORM_INT16;
        break;
    case GL_DEPTH_COMPONENT32F:
        clFormat.image_channel_order= CL_DEPTH;
        clFormat.image_channel_data_type= CL_FLOAT;
        break;

    // Format mappings not listed in the standard
    case GL_RG16F:
        clFormat.image_channel_order= CL_RG;
        clFormat.image_channel_data_type= CL_HALF_FLOAT;
        break;
    case GL_RG32F:
        clFormat.image_channel_order= CL_RG;
        clFormat.image_channel_data_type= CL_FLOAT;
        break;

    // Unknown format
    default:
        cerr << "CLUtilFunctions::gl2clFormat: Could not convert format " << glFormat << endl;
        return false;
    }
    return true;
}


