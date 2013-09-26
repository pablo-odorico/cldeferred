#ifndef MOTIONBLUR_H
#define MOTIONBLUR_H

#include "clutils.h"
#include <Qt3D/QGLTexture2D>

class MotionBlur
{
public:
    MotionBlur();

private:
    QGLTexture2D _output;
};

#endif // MOTIONBLUR_H
