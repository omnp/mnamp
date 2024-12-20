#include <cmath>
#include "math.h"
#pragma once

template<typename type> type inline pow(type const x, type const p) {
    return std::exp(p * std::log(x));
}

template<typename type> type inline soft(type const x, type const p = 4.0) {
    return x / pow(1.0 + pow(std::abs(x), p), 1.0/p);
}

template<typename type> type inline soft2(type const x, type const p = 4.0) {
    const type mu = soft(1.0, p);
    return soft(x, p)/mu;
}

template<typename type> type softabs(type const x, type const p = 4.0) {
    const type mu = soft(1.0, p);
    return x * soft(x, p)/mu;
}

template<typename type> type soft2abs(type const x, type const p = 4.0) {
    const type mu = soft2(1.0, p);
    return x * soft2(x, p)/mu;
}

template<typename type> type f(type const x, type const a, type const b) {
    /** Amplify with bias */
    type y = a*(x+softabs(x)*b);
    return y;
}

template<typename type> type h(type const x, type const a, type const b, type const p = 4.0) {
    type const fx = f(x, a, b);
    type const base = 2.0 + std::abs(x) - b;
    type const c = pow(base, fx);
    type const d = pow(base, -fx);
    return 1.0/(1.0 + d/c) - 1.0/(c/d + 1.0);
 }

template<typename type> type g(type const x, type const a, type const b, type const p = 4.0) {
    type const c = h(0.0, a, b, p);
    type const m = 1.0 + std::abs(c);
    type const fx = h(x, a, b, p);
    return (fx - c)/m;
}

template<typename type> type integrate1_trapezoidal(type const u, type const v, type const a, type const b, type (*f)(type const x, type const a, type const b), uint32_t const n = 128u) {
    type const d = (v - u) / type(n);
    type t = f(u, a, b);
    type I = 0.0;
    for (uint32_t i = 1u; i <= n; i++) {
        type x, y;
        x = u + type(i) * d;
        y = f(x, a, b);
        I += (y + t);
        t = y;
    }
    return 0.5 * I * d;
}

template<typename type> class Soft {
    /** A sort of soft clipper. */
public:
    Soft(type const sr = 48000.0) {
        x = y = d = 0.0;
        this->sr = sr;
    }
    void set_samplerate(type const sr) {
        this->sr = sr;
    }
    type operator()(type const x, type const a, type const b) {
        return diff(x, a, b);
    }
protected:
    const type quantum = 1e-24;
    type diff(type const x, type const a, type const b) {
        type y = 0.0;
        type x_ = x;
        type x1_ = this->x;
        type dx = x_ - x1_;
        if (std::abs(dx) < quantum) {
            x_ += quantum;
            dx = x_ - x1_;
        }
        y = g(x_, a, b, 4.0);
        type y1 = this->y;
        type dy = y - y1;
        type d = dy / dx;
        dy = d * dx;
        this->x += dx;
        this->y += dy;
        this->d = d;
        return this->y;
    }
private:
    type x;
    type y;
    type d;
    type sr = 48000.0;
};
