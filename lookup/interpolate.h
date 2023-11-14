#pragma once

#include "../common/math.h"

namespace lookup {
    template <typename type, template <typename T> typename base_lookup> class lookup_table 
    {
    private:
        static const uint32_t g_steps {base_lookup<type>::g_steps};
        static const type constexpr g_max {base_lookup<type>::g_max};
        static const type constexpr (&lut)[base_lookup<type>::g_steps] {base_lookup<type>::lut};
        static const type constexpr g_scale {g_steps / g_max};
    public:
        static type inline lookup(type g) {
            // g
            g = std::abs(g) * g_scale;
            type const s = g - std::floor(g);
            uint32_t j = uint32_t(g);
            j = j > g_steps-2u ? g_steps-2u : j;
            return (lut[j]) * (1. - s)
                + s * (lut[j+1]);
        }
    };
}