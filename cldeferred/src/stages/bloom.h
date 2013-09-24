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
    bool resize(QSize inputSize);
    // 3. Write LINEAR color values to the input buffer.
    //    Values above 1.0f are considered "bright".
    cl_mem& input() { assert(_initialized); return _images[0]; }
    // 4. Filter bright image and blend both images into outputImage
    // outputImage is written GAMMA-CORRECTED with luma precomputed in the alpha
    // channel
    bool update(cl_command_queue queue, cl_mem outputImage);

    bool enabled() const { return _enabled; }
    void setEnabled(bool value) { _enabled= value; }
    void toggleEnabled() { _enabled= !_enabled; }

    float bloomBlend() const { return _bloomBlend; }
    void setBloomBlend(float value) { _bloomBlend= qMax(value, 0.0f); }

    float brightThreshold() const { return _bloomThres; }
    void setBrightThreshold(float value) { _bloomThres= qMax(value, 0.0f); }


private:
    Q_DISABLE_COPY(Bloom)

    bool _initialized;
    bool _enabled;

    float _bloomBlend; // finalColor= visibleColor + bloomBlend * brightColor
    float _bloomThres; // Values over this threshold are consdered "bright"

    // Input image and it's downsamples. Values over 1.0f are "bright".
    QVector<cl_mem> _images;
    // Number of _images (_levels-1 downsamples), depends on _inputSize
    int _levels;

    // Size of input image (_images[0])
    QSize _inputSize;
    // Size of a downsample level (_inputSize if level==0)
    QSize imageSize(int level);

    cl_context _context;
    cl_kernel _downKernel;
    cl_kernel _blendKernel;
    cl_kernel _bypassKernel;
};

#endif // BLOOM_H
