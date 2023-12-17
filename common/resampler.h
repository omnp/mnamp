#include <array>
#include <cstdint>
#include "filter.h"

template <typename type, class filter_type, const uint32_t max_factor>
class resampler
{
public:
    filter_type upsampler;
    filter_type downsampler;
    uint32_t upfactor;
    uint32_t downfactor;
    std::array<type, max_factor> buffer;

    explicit resampler() : upfactor{1u}, downfactor{1u} {
        if (upfactor > max_factor || downfactor > max_factor)
            throw;
    }

    void upsample(type const x) {
        upsampler.process(x);
        buffer[0] = upsampler.pass();
        for (uint32_t i = 1u; i < upfactor; i++) {
            upsampler.process(0.0);
            buffer[i] = upsampler.pass();
        }
        for (uint32_t i = 0u; i < upfactor; i++) {
            upsampler.process(buffer[upfactor-1u-i]);
            buffer[upfactor-1u-i] = upsampler.pass();
        }
    }

    type const downsample() {
        for (uint32_t i = 0u; i < downfactor; i++) {
            if (i < upfactor) {
                downsampler.process(buffer[i]);
            }
            else {
                downsampler.process(0.0);
            }
            buffer[i] = downsampler.pass();
        }
        for (uint32_t i = 0u; i < downfactor; i++) {
            if (i < upfactor) {
                downsampler.process(buffer[downfactor-1u-i]);
            }
            else {
                downsampler.process(0.0);
            }
            buffer[downfactor-1u-i] = downsampler.pass();
        }
        return downsampler.pass();
    }
};
