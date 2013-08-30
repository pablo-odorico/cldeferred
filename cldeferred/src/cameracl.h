#ifndef CAMERACL_H
#define CAMERACL_H

#include "clutilfunctions.h"
#include "camera.h"
#include "cl_camera.h"

class CameraCL : public Camera, protected CLUtilFunctions
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
};

#endif // CAMERACL_H
