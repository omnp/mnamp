#include <cmath>
#include <cstdint>
#include "math.h"

template<typename type> class Soft;

template<typename type> type f(type const x, type const a, type const b) {
    /** Amplify with bias */
    type y = a*(x+b);
    return y;
}

template<typename type> type g(type const x, type const a, type const b) {
    type const fx = f(x,a,b * std::abs(x));
    //return math::sgn(fx) * std::min(std::abs(fx), 1.0);
    //return std::tanh(fx);
    return fx / (1.0 + std::abs(fx));
}

template<typename type> type integrate1_trapezoidal(Soft<type> * const soft, type const u, type const v, type const a, type const b, type (*f)(type const x, type const a, type const b), uint32_t const n = 128u) {
    type const d = (v - u) / type(n);
    type t = 0.0;
    type I = 0.0;
    for (uint32_t i = 0u; i < n; i++) {
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
        x = y = 0.0;
        this->sr = sr;
    }
    void set_samplerate(type const sr) {
        this->sr = sr;
    }
    type operator()(type const x, type const a, type const b) {
        return diff(x, a, b);
    }
protected:
    const uint32_t bits = 24u;
    const type quantum = 1.0/(1 << (bits - 1u));
    type diff(type const x, type const a, type const b) {
        type y = 0.0;
        type dx = x - this->x;
        while (std::abs(dx) < quantum) {
            dx += quantum;
        }
        //y = g(this->x + dx, a, b);
        y = integrate1_trapezoidal(this, this->x, this->x + dx, a, b, g, 64u) / dx;
        type dy = y - this->y;
        type d = dy / dx;
        if (std::abs(d) > 1.0) {
            d = math::sgn(d);
        }
        dy = d * dx;
        this->x += dx;
        this->y += dy;
        return this->y;
    }
private:
    type x;
    type y;
    type sr = 48000.0;
};
