#pragma once

#include "math.h"

template<typename T> inline T S(T const x) {
    return x / (std::abs(x) + 1.0f);
}

template<typename T> inline T oddpowers(T const a, uint32_t const k) {
    T p = a;
    T y = p;
    for (uint32_t i = 1; i < k; i++) {
        p = p*a*a;
        y += p;
    }
    return y;    
}

template<typename T> inline T oddpowers(T const a, uint32_t const k, T const f) {
    T g = f;
    T p = a;
    T y = p / g;
    for (uint32_t i = 1; i < k; i++) {
        p = p*a*a;
        g = g + 2.*f;
        y += p / g;
    }
    return y;    
}

template<typename T> inline T evenpowers(T const a, uint32_t const k) {
    T p = a*a;
    T y = p;
    for (uint32_t i = 1; i < k; i++) {
        p = p*a*a;
        y += p;
    }
    return y;    
}

template<typename T> inline T evenpowers(T const a, uint32_t const k, T const f) {
    T g = f;
    T p = a*a;
    T y = p / g;
    for (uint32_t i = 1; i < k; i++) {
        p = p*a*a;
        g = g + 2.*f;
        y += p / g;
    }
    return y;    
}

template<typename type> inline type OddPolynomial(type const x, type const c, uint32_t const k) {
    if (x == 0.0f)
        return 0.0f;
    type s = math::sgn<>(x);
    type b = s*x;
    type y = 0.0f;
    type d = 1.0f;
    type e = 0.0f;
    type a = 0.0f;
    while (std::abs(d) > 0.0f) {
        a = e*c;
        y = oddpowers(a, k);
        d = std::abs(d);
        if (y > b)
            d = -d;
        else if (y < b);
        else break;
        d = 0.5f * d;
        e += d;
    }
    return s*e;
}

template<typename type> inline type EvenPolynomial(type const x, type const c, uint32_t const k) {
    if (x == 0.0f)
        return 0.0f;
    type s = math::sgn<>(x);
    type b = s*x;
    type y = 0.0f;
    type d = 1.0f;
    type e = 0.0f;
    type a = 0.0f;
    while (std::abs(d) > 0.0f) {
        a = e*c;
        y = evenpowers(a, k);
        d = std::abs(d);
        if (y > b)
            d = -d;
        else if (y < b);
        else break;
        d = 0.5f * d;
        e += d;
    }
    return s*e;
}

template<typename type> inline type Exponential(type const x, type const c) {
    if (x == 0.0f)
        return 0.0f;
    type s = math::sgn<>(x);
    type b = s*x;
    type e = log(1.0f + b) * c;
    return s*e;
}
