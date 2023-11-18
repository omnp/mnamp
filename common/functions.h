#pragma once

#include "math.h"

#include "svfilter.h"

namespace functions {
    template <typename type>
    type inline S(type x, type limit) {
        return limit * x / (1. + std::abs(x));
    }
    template <typename type, const uint32_t cascade_order = 8u, const uint32_t iterations = 1u, typename table_type>
    type inline G(const type g, const table_type & table, const uint32_t factor, const type tension = 1e-6) {
        uint32_t k = 0;
        FilterCascade<type, cascade_order> filter1(highpass);
        type const cutoff = 0.5 / factor;
        type const Q = 0.500;
        type dlt = 0.0;
        type r = 1.0;
        filter1.setparams(cutoff, Q, 1.0);
        filter1.function = filter_type::highpass;
        while (k < iterations) {
            type t = 0.0;
            filter1.reset();
            for (uint32_t j = 0; j < factor; j++) {
                type y = S<type>(table[j] * g * r, 1.);
                filter1.process(y);
                t += std::abs(filter1.pass);
            }
            k += 1u;
            if (std::abs(t)/factor > tension) {
                dlt = 1.0 / type(1u << k);
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