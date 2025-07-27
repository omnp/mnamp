#pragma once

#include "math.h"
#include "filter.h"

template <typename type_> struct OnePole : public filter<type_, OnePole<type_>>
{
public:
    using type = type_;
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
        s = 0.0;
    }
};

template <typename base> struct OnePoleHigh : public filter<typename base::type, OnePoleHigh<base>>
{
public:
    using type = typename base::type;
private:
    base low;
    type t;
public:
    explicit OnePoleHigh() {
    }
    type const pass() const {
        return t;
    }
    void process(type const x) {
        low.process(x);
        t = x - low.pass();
    }
    void setparams(type k, type q, type sr) {
        low.setparams(k, q, sr);
    }
    void reset() {
        low.reset();
        t = 0.0;
    }
};
