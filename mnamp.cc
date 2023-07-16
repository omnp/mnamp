#include <lv2/core/lv2.h>
#include "svfilter.h"
#include "math.h"

namespace mnamp {
    template <typename type> struct BiQuad
    {
    public:
        type delay[4] = {0.,0.,0.,0.};
        type process(type x, type g, type b1, type b2, type a1, type a2) {
            type y = g * x + b1 * delay[0] + b2 * delay[1] - a1 * delay[2] - a2 * delay[3];
            delay[1] = delay[0];
            delay[0] = x;
            delay[3] = delay[2];
            delay[2] = y;
            return y;
        }
    };
    template <typename type> struct BQFilter
    {
    public:
        uint32_t n;
        BiQuad<type> B[8u];
        type lp;
        type k;
        type w;
        type q;
        type a;
        type t;
        type a0;
        type a1;
        type a2;
        type b0;
        type b1;
        type b2;
        type g;

        BQFilter(uint32_t const n = 1u) : n{n > 8u ? 8u : n} {
            k = 0.25;
            q = .625;
            preprocess();
        }
        void setk(type x) {
            k = x;
            preprocess();
        }
        void setq(type x) {
            q = x;
            preprocess();
        }
        void preprocess() {
            w = 2.*M_PI * k;
            a = std::sin(w) / (2.*q);
            t = cos(w);
            a0 = (1.+a);
            a1 = ( -2. * t ) / a0;
            a2 = ( 1. - a ) / a0;
            b0 = ( (1. - t)/2. ) / a0;
            b1 = ( 1. - t ) / a0;
            b2 = ( (1. - t)/2. ) / a0;
            g = b0 / a0; 
        }
        void process(type x) {
            for (uint32_t i = 0; i < n; i++) {
                x = B[i].process(x, g, b1, b2, a1, a2);
            }
            lp = x;
        }
    };

    const uint32_t MNAMP_N_PORTS {9u};
    const uint32_t MAX_STAGES {12u};
    template<typename type, typename input_type>
    struct mnamp
    /* This is the amp's structure.
    */
    {
        input_type * ports[MNAMP_N_PORTS];
        BQFilter<type> filter[MAX_STAGES][2u];
        Filter<type> highpass[MAX_STAGES];
        type sr;
        static enum {
            out = 0,
            in = 1,
            gain = 2,
            stages = 3,
            drive1 = 4,
            drive2 = 5,
            resonance = 6,
            factor = 7,
            eps = 8, 
        } names;
        bool status = false;
    };

    template <typename type, typename input_type>
    LV2_Handle instantiate (
        const struct LV2_Descriptor *descriptor,
        const double rate,
        const char *bundle_path,
        const LV2_Feature *const *features) 
    {
        mnamp<type, input_type> *a = new mnamp<type, input_type>;
        a->sr = rate;
        return a;
    }

    template <typename type, typename input_type>
    void connect_port (LV2_Handle instance, const uint32_t port, void *data)
    {
        mnamp<type, input_type> *a = (mnamp<type, input_type> *) instance;
        if (!a) return;

        if (port < MNAMP_N_PORTS)
            a->ports[port] = (input_type *) data;
    }

    template <typename type, typename input_type>
    void activate (LV2_Handle instance)
    {
        mnamp<type, input_type> *a = (mnamp<type, input_type> *) instance;
        if (!a) return;

        for (uint32_t j = 0; j < MAX_STAGES; j++)
            for (uint32_t i = 0; i < 2u; i++)
                a->filter[j][i] = BQFilter<type>(8u);
        for (uint32_t j = 0; j < MAX_STAGES; j++)
            a->highpass[j] = Filter<type>();
        a->status = false;
    }

    template <typename type, typename input_type>
    void run (LV2_Handle instance, const uint32_t n)
    {
        mnamp<type, input_type> *amp = (mnamp<type, input_type> *) instance;
        if (!amp) return;
        for (uint32_t i = 0; i < MNAMP_N_PORTS; ++i)
            if (!amp -> ports[i]) return;

        // Ports
        input_type * const out = amp->ports[amp->out];
        const input_type * const x = amp->ports[amp->in];
        const type gain = *amp->ports[amp->gain];
        const type drive1 = *amp->ports[amp->drive1];
        const type drive2 = *amp->ports[amp->drive2];
        const type resonance = *amp->ports[amp->resonance];
        const type eps = *amp->ports[amp->eps];
        uint32_t const factor = *amp->ports[amp->factor];
        uint32_t const stages = uint32_t(*amp->ports[amp->stages]);
        const auto f = [](type x, type limit) {return limit * x / (1. + std::abs(x));};
        
        // Preprocessing
        const uint32_t sampling = factor;
        for (uint32_t j = 0; j < MAX_STAGES; j++) {
            for (uint32_t i = 0; i < 2u; i++) {
                amp->filter[j][i].setk(0.5 / factor);
                amp->filter[j][i].setq(resonance);
            }
            amp->highpass[j].setparams(0.001, 0.900, 1.0);
        }
        
        // Post set values
        //
        type buffer[sampling];
        
        // Processing loop.
        for (uint32_t i = 0; i < n; ++i) {
            out[i] = x[i];
        }
        for (uint32_t i = 0; i < n; ++i) {
            for (uint32_t h = 0; h < stages; ++h) {
                type t = out[i];
                type b = 0.0;
                type u = 0.0;
                type v = 0.0;
                type s = 0.0;
                
                amp->filter[h][0].process(sampling * t);
                buffer[0] = amp->filter[h][0].lp;
                for (uint32_t j = 1; j < sampling; j++) {
                    amp->filter[h][0].process(0.0);
                    buffer[j] = amp->filter[h][0].lp;
                }

                for (uint32_t j = 0; j < sampling; j++) {
                    t = buffer[j];
                    u = t;
                    u = f(u, 1.);
                    b = u;
                    b = (1.0 + b) * 0.5;
                    s = math::sgn<>(u);
                    u = std::abs(u);
                    v = drive2 * u;
                    u = u + std::pow(u, eps + (2.*(stages-1-h)+1.0)) * (1. - u);
                    u = drive1 * u;
                    u = b * u + (1.0 - b) * v;
                    u = u * s;
                    buffer[j] = u;
                }

                for (uint32_t j = 0; j < sampling; j++) {
                    amp->filter[h][1].process(buffer[j]);
                    u = amp->filter[h][1].lp;
                }
                u = u * gain;
                amp->highpass[h].process(u);
                u = amp->highpass[h].hp;
                out[i] = u;
            }
        }
    }
    void deactivate (LV2_Handle instance)
    {}
    template <typename type, typename input_type>
    void cleanup (LV2_Handle instance)
    {
        mnamp<type, input_type> *a = (mnamp<type, input_type> *) instance;
        if (!a) return;
        delete a;
    }
    const void *extension_data (const char *uri)
    {
        return nullptr;
    }
}

// Plugin linkage.

using processing_type = float;
using io_type = float;

static LV2_Descriptor const descriptor =
{
    .URI = "urn:omnp:mnamp",
    .instantiate = mnamp::instantiate<processing_type, io_type>,
    .connect_port = mnamp::connect_port<processing_type, io_type>,
    .activate = mnamp::activate<processing_type, io_type>,
    .run = mnamp::run<processing_type, io_type>,
    .deactivate = mnamp::deactivate,
    .cleanup = mnamp::cleanup<processing_type, io_type>,
    .extension_data = mnamp::extension_data,
};

LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor (const uint32_t index)
{
    if (index == 0) return &descriptor;
    return nullptr;
}
