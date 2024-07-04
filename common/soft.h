#include <cmath>
#include <cstdint>
#include "math.h"

template<typename type> type f(type const x, type const a, type const b) {
    /** Amplify with bias */
    type y = a*(x+b);
    return y;
}

template<typename type> type softabs(type const x) {
    static const type mu = 1e-1;
    return x * std::tanh(x/mu);
}

template<typename type> type g(type const x, type const a, type const b) {
    type fx = f(x,a,0.0);
    fx = fx / (1.0 + softabs(fx));
    fx = fx + b;
    return fx;
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
        //y = integrate1_trapezoidal(x1_, x1_ + dx, a, b, g, 64u) / dx;
        y = g(x_, a, b);
        type y1 = this->y;
        type dy = y - y1;
        type d = dy / dx;
        if (std::abs(d) > 2.0*M_PI) {
            d = math::sgn(d) * 2.0*M_PI;
        }
        if (std::abs(d - this->d) < quantum * sr) {
            d -= math::sgn(d) * quantum * sr;
        }
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
