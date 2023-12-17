#include "../common/math.h"
#include "../common/functions.h"
#include "../common/sosfilters.h"

#include <cstdint>
#include <ios>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

template <typename type, uint32_t cascade_order = 1u>
struct Generator
{

    const uint32_t G_STEPS;
    const type G_MAX;
    type *lut;

    explicit Generator(uint32_t factor = 4u, uint32_t iterations = 32u, const type tension = 1e-6, uint32_t const g_steps = 64u, type const g_max = 1.) 
    : G_STEPS(g_steps),
      G_MAX(g_max) {
        filter_cascade<type, elliptic<type>, cascade_order> lowpass;
        filter_cascade<type, elliptic<type>, cascade_order> highpass;
        //type const cutoff = 0.5 / factor;
        //type const Q = 0.500;
        lowpass.setparams(type(factor) / type(2.0), 1.0, 1.0);
        highpass.setparams(type(factor) / type(2.0), 1.0, 1.0);
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
        lut = new type[4u*G_STEPS];
        for (uint32_t i = 0; i < 4u; i++) {
            for (uint32_t g = 0u; g < G_STEPS; g++) {
                type x = 0.0;
                switch (i) {
                    case 0: {
                        x = functions::minimize<type, filter_cascade<type, elliptic<type>, cascade_order>, std::vector<type>>( G_MAX * type(g) / type(G_STEPS-1u), highpass, table, factor, tension, iterations, functions::NOOP);
                        break;
                    }
                    case 1: {
                        x = functions::minimize<type, filter_cascade<type, elliptic<type>, cascade_order>, std::vector<type>>( G_MAX * type(g) / type(G_STEPS-1u), highpass, table, factor, tension, iterations, functions::S);
                        break;
                    }
                    case 2: {
                        x = functions::minimize<type, filter_cascade<type, elliptic<type>, cascade_order>, std::vector<type>>( G_MAX * type(g) / type(G_STEPS-1u), highpass, table, factor, tension, iterations, functions::T);
                        break;
                    }
                    case 3: {
                        x = functions::minimize<type, filter_cascade<type, elliptic<type>, cascade_order>, std::vector<type>>( G_MAX * type(g) / type(G_STEPS-1u), highpass, table, factor, tension, iterations, functions::H);
                        break;
                    }
                }
                lut[i*G_STEPS+g] = x;
            }
        }
    }
    ~Generator () {delete[] lut;}

    void write(std::ostream & out) {
        out << "#pragma once" << std::endl;
        out << "#include <cstdint>" << std::endl;
        out << "template <typename type> struct  lookup_parameters {" << std::endl;
        out << "static const uint32_t g_steps {" << G_STEPS << "u};" << std::endl;
        out << "static const type constexpr g_max {" << G_MAX << "};" << std::endl;
        out << "static const type constexpr lut[4u*g_steps] = " << std::endl;
        out << "{";
        for (uint32_t i = 0; i < 4u; i++) {
            for (uint32_t g = 0u; g < G_STEPS; g++) {
                out << std::setprecision(32u) << lut[i*G_STEPS+g];
                out << ",";
                out << std::flush;
            }
        }
        out << "};";
        out << std::flush;
        out << std::endl;
        out << "};";
        out << std::flush;
        out << std::endl;
    }
};

template <typename type>
type read_arg(char const * arg) {
    std::stringstream s;
    s << arg;
    type r;
    s >> r;
    return r;
}

#ifdef CASCADE_ORDER
uint32_t const cascade_order = CASCADE_ORDER;
#else
uint32_t const cascade_order = 4u;
#endif

int main(int count, char *args[]) {
    enum argument {type = 1u, factor, iterations, tension, steps, max_db, arguments};
    if (count >= arguments) {
        uint32_t const factor_in = read_arg<uint32_t>(args[factor]);
        uint32_t const iterations_in = read_arg<uint32_t>(args[iterations]);
        uint32_t const steps_in = read_arg<uint32_t>(args[steps]);
        if (read_arg<std::string>(args[type]) == "float") {
            float const tension_in = read_arg<float>(args[tension]);
            float const max_db_in = read_arg<float>(args[max_db]);
            Generator<float, cascade_order> Gen(factor_in, iterations_in, tension_in, steps_in, math::dbl<float>(max_db_in));
            std::ofstream file("lookup.h", std::ios_base::trunc);
            Gen.write(file);
            file.close();
        }
        else if (read_arg<std::string>(args[type]) == "double") {
            float const tension_in = read_arg<double>(args[tension]);
            float const max_db_in = read_arg<double>(args[max_db]);
            Generator<double, cascade_order> Gen(factor_in, iterations_in, tension_in, steps_in, math::dbl<double>(max_db_in));
            std::ofstream file("lookup.h", std::ios_base::trunc);
            Gen.write(file);
            file.close();
        }
    }
    else {
        std::cout << "\n\tRequires " << arguments << " arguments!\n" << std::endl;
    }
    return 0;
}
