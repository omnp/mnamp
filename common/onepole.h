#pragma once

#include "math.h"
#include "filter.h"

template <typename type> struct OnePole : public filter<type, OnePole<type>>
{
private:
    type a;
    type b;
    type y;
    type s;
public:
    explicit OnePole() {
        a = 0.0;
        b = 1.0;
        y = 0.0;
        s = 0.0;
    }
    type const pass() const {
        return s;
    }
    void process(type const x) {
        y = b * x + a * y;
        s = y;
    }
    void setparams(type k, type q, type sr) {
        type f = k/sr;
        a = std::exp(-2.0 * M_PI * f);
        b = 1.0 - a;
    }
    void reset() {
        y = 0.0;
    }
};
