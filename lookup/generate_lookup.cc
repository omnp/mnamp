#include "../common/math.h"
#include "../common/functions.h"

#include <ios>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ostream>

template <typename type, uint32_t factor = 4u, uint32_t iterations = 32u>
struct Generator
{

    const uint32_t U_STEPS;
    const uint32_t G_STEPS;
    const uint32_t T_STEPS;
    const type U_MAX;
    const type G_MAX;
    const type T_MAX;
    type *lut;

    explicit Generator(uint32_t const u_steps = 128u, uint32_t const g_steps = 64u, uint32_t t_steps = 8u, type const u_max = 16384., type const g_max = 2., type const t_max = 2.) 
    : U_STEPS(u_steps), G_STEPS(g_steps), T_STEPS(t_steps),
      U_MAX(u_max), G_MAX(g_max), T_MAX(t_max) {
        lut = new type[T_STEPS * G_STEPS * U_STEPS];
        for (uint32_t t = 0u; t < T_STEPS; t++) {
            for (uint32_t g = 0u; g < G_STEPS; g++) {
                for (uint32_t u = 0u; u < U_STEPS; u++) {
                    type x = functions::G<type, factor, iterations>(U_MAX * type(u) / type(U_STEPS), G_MAX * type(g) / type(G_STEPS), T_MAX * type(t+1u) / type(T_STEPS));
                    lut[t*G_STEPS*U_STEPS + g*U_STEPS + u] = x;
                }
            }
        }
    }
    ~Generator () {delete[] lut;}

    void write(std::ostream & out) {
        out << "#pragma once" << std::endl;
        out << "#include <cstdint>" << std::endl;
        out << "template <typename type> struct  lookup_parameters {" << std::endl;
        out << "static const uint32_t u_steps {" << U_STEPS << "u};" << std::endl;
        out << "static const uint32_t g_steps {" << G_STEPS << "u};" << std::endl;
        out << "static const uint32_t t_steps {" << T_STEPS << "u};" << std::endl;
        out << "static const type constexpr u_max {" << U_MAX << "};" << std::endl;
        out << "static const type constexpr g_max {" << G_MAX << "};" << std::endl;
        out << "static const type constexpr t_max {" << T_MAX << "};" << std::endl;
        out << "static const type constexpr lut[t_steps][g_steps][u_steps] = " << std::endl;
        out << "{";
        for (uint32_t t = 0u; t < T_STEPS; t++) {
            out << "{";
            for (uint32_t g = 0u; g < G_STEPS; g++) {
                out << "{";
                for (uint32_t u = 0u; u < U_STEPS; u++) {
                    out << std::setprecision(32u) << lut[t*G_STEPS*U_STEPS + g*U_STEPS + u];
                    out << ",";
                    out << std::flush;
                }
                out << "}";
                out << ",";
                out << std::flush;
                out << std::endl;
            }
            out << "}";
            out << ",";
            out << std::flush;
            out << std::endl;
        }
        out << "};";
        out << std::flush;
        out << std::endl;
        out << "};";
        out << std::flush;
        out << std::endl;
    }
};

int main() {
    Generator<float, 8u, 64u> Gen{128u,32u,2u,16384.,2.,2.};
    std::ofstream file("lookup.h", std::ios_base::trunc);
    Gen.write(file);
    file.close();

    return 0;
}