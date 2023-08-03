#pragma once

#include "math.h"

namespace functions {
    #include "bqfilter.h"

    template <typename type>
    type inline S(type x, type limit) {
        return limit * x / (1. + std::abs(x));
    }
    template <typename type, const uint32_t internal_factor = 4u, const uint32_t iterations = 1u>
    type inline G(const type u, const type g, const type tension=1.) {
        uint32_t k = 0;
        BQFilter<type> fl(8u);
        type bf[internal_factor];
        type const cutoff = 0.5 / internal_factor;
        type const Q = 0.700;
        type dlt = 0.0;
        type r = g;
        fl.setk(cutoff);
        fl.setq(Q);
        while (k < iterations) {
            type s = 0.0;
            fl.reset();
            fl.process(u * r);
            bf[0] = fl.lp * internal_factor;
            bf[0] = S<type>(bf[0], 1.);
            for (uint32_t i = 1u; i < internal_factor; i++) {
                fl.process(0.0);
                bf[i] = fl.lp * internal_factor;
                bf[i] = S<type>(bf[i], 1.);
            }
            fl.reset();
            for (uint32_t i = 0u; i < internal_factor; i++) {
                fl.process(bf[i]);
                s += std::abs(std::abs(bf[i]) - std::abs(fl.lp));
            }
            if (std::abs(s) > tension * 1.e-3) {
                dlt = r * .5 / (1u << k);
                if (r - dlt >= 0.0)
                    r -= dlt;
            }
            else {
                break;
            }
            k += 1u;
        }
        return r;
    }
}