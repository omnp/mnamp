#pragma once

#include <cstddef>
#include <cstdint>
#ifdef _WIN64
    #define _USE_MATH_DEFINES
#endif
#include <cmath>

namespace math {
    template<typename T> struct constants {
        static const T constexpr zero{0};
        static const T constexpr positive{1};
        static const T constexpr negative{-1};
    };
    template<typename T> inline T sgn(T const x) {
        if (x > constants<T>::zero) return constants<T>::positive;
        if (x < constants<T>::zero) return constants<T>::negative;
        return x;
    }
}