#pragma once

#include "../common/math.h"

namespace lookup {
    template <typename type, template <typename T> typename base_lookup> class lookup_table 
    {
    private:
        static const uint32_t u_steps {base_lookup<type>::u_steps};
        static const uint32_t g_steps {base_lookup<type>::g_steps};
        static const uint32_t t_steps {base_lookup<type>::t_steps};
        static const type constexpr u_max {base_lookup<type>::u_max};
        static const type constexpr g_max {base_lookup<type>::g_max};
        static const type constexpr t_max {base_lookup<type>::t_max};
        static const type constexpr (&lut)[base_lookup<type>::t_steps][base_lookup<type>::g_steps][base_lookup<type>::u_steps] {base_lookup<type>::lut};
        static const type constexpr u_scale {u_steps / u_max};
        static const type constexpr g_scale {g_steps / g_max};
        static const type constexpr t_scale {t_steps / t_max};
    public:
        static type inline lookup(type u, type g, type tension=1.) {
            // u
            u = std::abs(u) * u_scale;
            type const t = u - std::floor(u);
            uint32_t k = uint32_t(u);
            k = k > u_steps-2u ? u_steps-2u : k;
            // g
            g = std::abs(g) * g_scale;
            type const s = g - std::floor(g);
            uint32_t j = uint32_t(g);
            j = j > g_steps-2u ? g_steps-2u : j;
            // tension
            tension = std::abs(tension) * t_scale;
            type const r = tension - std::floor(tension);
            uint32_t i = uint32_t(tension);
            i = i > t_steps-2u ? t_steps-2u : i;
            return ((lut[i][j][k] * (1.-t) + t * lut[i][j][k+1]) * (1. - s)
                + s * (lut[i][j+1][k] * (1.-t) + t * lut[i][j+1][k+1])) * (1. - r)
                + r * ((lut[i+1][j][k] * (1.-t) + t * lut[i+1][j][k+1]) * (1. - s)
                + s * (lut[i+1][j+1][k] * (1.-t) + t * lut[i+1][j+1][k+1]));
        }
    };
}