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
