#pragma once

#include "math.h"

template <typename type> struct BiQuad
{
public:
    type delay[4] = {0.,0.,0.,0.};
    type process(type x, type g, type b1, type b2, type a1, type a2) {
        type y = g * x + b1 * delay[0] + b2 * delay[1] - a1 * delay[2] - a2 * delay[3];
        delay[1] = delay[0];
        delay[0] = x;
        delay[3] = delay[2];
        delay[2] = y;
        return y;
    }
};

template <typename type> struct BQFilter
{
private:
    uint32_t n;
    BiQuad<type> B[8u];
    type k;
    type w;
    type q;
    type a;
    type t;
    type a0;
    type a1;
    type a2;
    type b0;
    type b1;
    type b2;
    type g;
    void preprocess() {
        w = 2.*M_PI * k;
        a = std::sin(w) / (2.*q);
        t = cos(w);
        a0 = (1.+a);
        a1 = ( -2. * t ) / a0;
        a2 = ( 1. - a ) / a0;
        b0 = ( (1. - t)/2. ) / a0;
        b1 = ( 1. - t ) / a0;
        b2 = ( (1. - t)/2. ) / a0;
        g = b0 / a0; 
    }
public:
    type lp;
    BQFilter(uint32_t const n = 1u) : n{n > 8u ? 8u : n} {
        k = 0.25;
        q = .625;
        preprocess();
    }
    void setk(type x) {
        k = x;
        preprocess();
    }
    void setq(type x) {
        q = x;
        preprocess();
    }
    void process(type x) {
        for (uint32_t i = 0; i < n; i++) {
            x = B[i].process(x, g, b1, b2, a1, a2);
        }
        lp = x;
    }
    void reset() {
        for (uint32_t i = 0; i < n; i++) {
            for (uint32_t j = 0; j < 4u; j++)
                B[i].delay[j] = 0.0;
        }
    }
};