#include <lv2/core/lv2.h>
#include "svfilter.h"

namespace mnamp {
    const uint32_t MNAMP_N_PORTS {6u};
    const uint32_t MAX_STAGES {4u};
    template<typename type, typename input_type>
    struct mnamp
    /* This is the amp's structure.
    */
    {
        input_type * ports[MNAMP_N_PORTS];
        Filter<type> filter[2u*MAX_STAGES];
        type sr;
        static enum {
            out = 0,
            in = 1,
            gain = 2,
            factor = 3,
            drive1 = 4,
            drive2 = 5,
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

        for (uint32_t i = 0; i < 2u*MAX_STAGES; i++)
            a->filter[i] = Filter<type>();
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
        uint32_t const factor = uint32_t(*amp->ports[amp->factor]);
        
        // Preprocessing
        if (!amp->status) {
            for (uint32_t i = 0; i < 2u*MAX_STAGES; ++i)
                amp->filter[i].setparams(0.5, 1.0, 2.0);
        }
        // Post set values
        amp->status = true;
        //
        
        // Processing loop.
        for (uint32_t i = 0; i < n; ++i) {
            type t = x[i];
            type b = 0.0;
            type u = 0.0;
            type v = 0.0;
            type s = 0.0;
            for (uint32_t j = 0; j < factor; j++) {
                amp->filter[j].process(t);
                amp->filter[j].process(0.0);
                t = 2.0*amp->filter[j].lp;
            }
            u = t;
            u = u / (1.0 + std::abs(u));
            b = (1.0 + u) * 0.5;
            s = math::sgn<>(u);
            u = std::abs(u);
            v = u * s;
            u = u + gain * u * (1 - u);
            u = u * s;
            u = (drive1 * b * u + drive2 * (1.0 - b) * v)/(drive1 >= drive2 ? drive1 : drive2);
            for (uint32_t j = 0; j < factor; j++) {
                amp->filter[MAX_STAGES+j].process(u);
                u = amp->filter[MAX_STAGES+j].lp;
            }
            out[i] = u;
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
