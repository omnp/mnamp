#pragma once

#include "math.h"

#include "svfilter.h"

namespace functions {
    template <typename type>
    type inline S(type x, type limit) {
        return limit * x / (1. + std::abs(x));
    }
    template <typename type, const uint32_t internal_factor = 4u, const uint32_t iterations = 1u, typename table_type>
    type inline G(const type g, const table_type & table) {
        const uint32_t factor = internal_factor;
        const type tension = 1e-6;
        uint32_t k = 0;
        FilterCascade<type, 8u> filter0(lowpass);
        FilterCascade<type, 8u> filter1(highpass);
        type bf[factor];
        type const cutoff = 0.5 / factor;
        type const Q = 0.700;
        type dlt = 0.0;
        type r = 1.0;
        filter0.setparams(cutoff, Q, 1.0);
        filter1.setparams(cutoff, Q, 1.0);
        filter1.function = filter_type::highpass;
        while (k < iterations) {
            type t = 0.0;
            filter0.reset();
            filter1.reset();
            for (type x: table) {
                filter0.process(x * g * r * factor);
                bf[0] = filter0.pass;
                bf[0] = S<type>(bf[0], 1.);
                for (uint32_t i = 1u; i < factor; i++) {
                    filter0.process(0.0);
                    bf[i] = filter0.pass;
                    bf[i] = S<type>(bf[i], 1.);
                }
                for (uint32_t i = 0u; i < factor; i++) {
                    filter1.process(bf[i]);
                    t += std::abs(filter1.pass);
                }
            }
            k += 1u;
            if (std::abs(t) > tension) {
                dlt = r / type(1u << k);
                if (r - dlt >= 0.0)
                    r -= dlt;
            }
            else {
                break;
            }
        }
        return r;
    }
}