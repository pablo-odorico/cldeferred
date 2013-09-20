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
    cl_mem& brightImage() { assert(_initialized); return _brightImages[0]; }
    // 4. Filter bright image and blend both images into outputImage
    // outputImage is written GAMMA-CORRECTED with luma precomputed in the alpha
    // channel
    bool update(cl_command_queue queue, cl_mem outputImage);

    float brightBlend() const { return _brightBlend; }
    void setBrightBlend(float value) { _brightBlend= value; }

    float brightThreshold() const { return _brightThres; }
    void setBrightThreshold(float value) { _brightThres= value; }

private:
    bool _initialized;

    float _brightBlend;   // finalColor= visibleColor + brightBlend * brightColor
    float _brightThres;   // Values over this threshold are consdered "bright"

    cl_context _context;
    cl_kernel _blendKernel;
    cl_kernel _downKernel;

    // Visible image (linear color values below _brightThres)
    cl_mem _visibleImage;
    // Bright image (linea color values above _brightThres), and it's downsamples
    QVector<cl_mem> _brightImages;

    QSize _size;
    // Returns the size of a bright image downsample, and _size for level==0
    QSize brightSize(int level);

    // Number of bright images (_brightLevels-1 downsamples)
    static const int _brightLevels= 4;
};

#endif // BLOOM_H
