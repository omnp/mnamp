#pragma once

#include "../common/math.h"
#include "../common/functions.h"

namespace lookup {
    template <typename type, template <typename T> typename base_lookup> class lookup_table 
    {
    private:
        static const uint32_t g_steps {base_lookup<type>::g_steps};
        static const type constexpr g_max {base_lookup<type>::g_max};
        static const type constexpr (&lut)[4u*base_lookup<type>::g_steps] {base_lookup<type>::lut};
        static const type constexpr g_scale {g_steps / g_max};
    public:
        static type inline lookup(type g, type (*shaper)(type, type)) {
            uint32_t i = 0;
            if (shaper == functions::NOOP<type>) {
                    i = 0u;
            }
            if (shaper == functions::S<type>) {
                    i = 1u;
            }
            if (shaper == functions::T<type>) {
                    i = 2u;
            }
            if (shaper == functions::H<type>) {
                    i = 3u;
            }
            // g
            g = std::abs(g) * g_scale;
            type const s = g - std::floor(g);
            uint32_t j = uint32_t(g);
            j = j > g_steps-2u ? g_steps-2u : j;
            return (lut[i*g_steps+j]) * (1. - s)
                + s * (lut[i*g_steps+j+1]);
        }
    };
}