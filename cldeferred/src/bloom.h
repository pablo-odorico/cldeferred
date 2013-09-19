#ifndef BLOOM_H
#define BLOOM_H

#include <cassert>
#include "utils/clutils.h"

class Bloom
{
public:
    Bloom();

    // 1. Initialize
    bool init(cl_context context, cl_device_id device);
    // 2. Create/resize bloom images
    bool resize(QSize size);
    // 3. Write LINEARLY into the visible and bright images
    cl_mem& visibleImage() { assert(_initialized); return _visibleImage; }
    cl_mem& brightImage() { assert(_initialized); return _brightImage; }
    // 4. Filter bright image and blend both images into outputImage
    // outputImage is written GAMMA-CORRECTED with luma precomputed in the alpha
    // channel
    bool update(cl_command_queue queue, cl_mem outputImage);

    float brightBlend() const { return _brightBlend; }
    void setBrightBlend(float value) { _brightBlend= value; }

private:
    bool _initialized;

    cl_context _context;

    float _brightBlend;
    cl_kernel _blendKernel;
    cl_mem _visibleImage; // OpenCL FP16 image
    cl_mem _brightImage;  // OpenCL FP16 image

    QSize _size;
};

#endif // BLOOM_H
