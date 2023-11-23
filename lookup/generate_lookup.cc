#include "../common/math.h"
#include "../common/functions.h"
#include "../common/svfilter.h"

#include <cstdint>
#include <ios>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include <vector>

template <typename type, uint32_t factor = 4u, uint32_t iterations = 32u, const uint32_t cascade_order = 8u, const type tension = 1e-6, typename table_type = std::vector<type>>
struct Generator
{

    const uint32_t G_STEPS;
    const type G_MAX;
    type *lut;

    explicit Generator(uint32_t const g_steps = 64u, type const g_max = 1.) 
    : G_STEPS(g_steps),
      G_MAX(g_max) {
        filter_cascade<type, SVFilterAdapter<filters::lowpass, type>, cascade_order> lowpass;
        filter_cascade<type, SVFilterAdapter<filters::highpass, type>, cascade_order> highpass;
        type const cutoff = 0.5 / factor;
        type const Q = 0.500;
        lowpass.setparams(cutoff, Q, 1.0);
        highpass.setparams(cutoff, Q, 1.0);
        std::vector<type> table;
        type sign = 1.0;
        for (uint32_t i = 0; i < 4u/*G_STEPS*/; i++) {
            type x = sign;
            lowpass.process(x);
            x = lowpass.pass() * factor;
            table.push_back(x);
            for (uint32_t j = 1; j < factor; j++) {
                lowpass.process(0.0);
                table.push_back(lowpass.pass() * factor);
            }
            if ((i+1) % 2 == 0)
                sign = -sign;
        }
        lut = new type[G_STEPS];
        for (uint32_t g = 0u; g < G_STEPS; g++) {
            type x = functions::approximate<type, filter_cascade<type, SVFilterAdapter<filters::highpass, type>, cascade_order>, std::vector<type>, iterations>( G_MAX * type(g) / type(G_STEPS-1u), highpass, table, factor, tension);
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
    Generator<float, 4u, 64u, 8u, 1e-6f> Gen(2048u,math::dbl<float>(120.0));
    std::ofstream file("lookup.h", std::ios_base::trunc);
    Gen.write(file);
    file.close();

    return 0;
}
