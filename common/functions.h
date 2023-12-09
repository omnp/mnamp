#pragma once

#include "math.h"
#include "filter.h"
#include <cstdint>

namespace functions {
    template <typename type>
    type inline NOOP(type x, type limit) {
        return x;
    }
    template <typename type>
    type inline S(type x, type limit) {
        return limit * x / (1. + std::abs(x));
    }
    template <typename type>
    type inline T(type x, type limit) {
        return limit * std::tanh(x);
    }
    template <typename type>
    type inline H(type x, type limit) {
        if (x > limit)
            x = limit;
        if (x < -limit)
            x = -limit;
        return x;
    }
    template <typename type, typename filter_concrete, typename table_type>
    type inline minimize(const type g, filter_concrete & filter1, const table_type & table, const uint32_t factor, const type tension = 1e-6, const uint32_t iterations = 1u, type (*S)(type, type) = functions::S<type>) {
        uint32_t k = 0;
        type dlt = 0.0;
        type r = 1.0;
        filter1.setparams(0.5 / factor, 0.5, 1.0);
        while (k < iterations) {
            type t = 0.0;
            type s = 0.0;
            filter1.reset();
            for (uint32_t j = 0; j < factor; j++) {
                type y = S(table[j] * g * r, 1.);
                filter1.process(y);
                s += std::abs(y);
                t += std::abs(filter1.pass());
            }
            k += 1u;
            if (std::abs(s - t) > tension) {
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
