#ifndef CAMERACL_H
#define CAMERACL_H

#include "clutils.h"
#include "camera.h"
#include "cl_camera.h"

class CameraCL : public Camera
{
public:
    CameraCL();
    ~CameraCL() { }

    // Init must be called before calling any other methods
    bool init(cl_context context);

    void updateStructCL(cl_command_queue queue);
    cl_mem structCL();

private:
    bool _initialized;

    cl_mem _clMem;
    cl_camera _clStruct;
};

#endif // CAMERACL_H
