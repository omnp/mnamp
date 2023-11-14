#include "../common/math.h"
#include "../common/functions.h"

#include <ios>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include <vector>
#include <cmath>

template <typename type, uint32_t factor = 4u, uint32_t iterations = 32u, typename table_type = std::vector<type>>
struct Generator
{

    const uint32_t G_STEPS;
    const type G_MAX;
    type *lut;

    explicit Generator(uint32_t const g_steps = 64u, type const g_max = 1.) 
    : G_STEPS(g_steps),
      G_MAX(g_max) {
        std::vector<type> table;
        for (uint32_t i = 0; i < G_STEPS; i++)
            table.push_back(std::sin(2*M_PI*(0.5*G_STEPS)*type(i)/type(G_STEPS)));
        lut = new type[G_STEPS];
        for (uint32_t g = 0u; g < G_STEPS; g++) {
            type x = functions::G<type, factor, iterations>( G_MAX * type(g) / type(G_STEPS-1u), table);
            lut[g] = x;
        }
    }
    ~Generator () {delete[] lut;}

    void write(std::ostream & out) {
        out << "#pragma once" << std::endl;
        out << "#include <cstdint>" << std::endl;
        out << "template <typename type> struct  lookup_parameters {" << std::endl;
        out << "static const uint32_t g_steps {" << G_STEPS << "u};" << std::endl;
        out << "static const type constexpr g_max {" << G_MAX << "};" << std::endl;
        out << "static const type constexpr lut[g_steps] = " << std::endl;
        out << "{";
        for (uint32_t g = 0u; g < G_STEPS; g++) {
            out << std::setprecision(32u) << lut[g];
            out << ",";
            out << std::flush;
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
    Generator<float, 64u, 64u> Gen(512u,math::dbl<float>(120.0));
    std::ofstream file("lookup.h", std::ios_base::trunc);
    Gen.write(file);
    file.close();

    return 0;
}