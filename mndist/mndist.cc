#include <lv2/core/lv2.h>
#include "../common/math.h"

namespace mndist {
    #include "../common/bqfilter.h"
    #include "../common/svfilter.h"
    #include "../common/functions.h"
#ifdef USE_LUT
    #include "../lookup/lookup.h"
    #include "../lookup/interpolate.h"
#endif

    template<typename type, typename io_type>
    class mndist
    {
    private:
        struct constants {
            static const uint32_t max_stages {16u};
            static const uint32_t max_factor {16u};
            struct names {
                static const uint32_t out = 0u;
                static const uint32_t in = 1u;
                static const uint32_t gain1 = 2u;
                static const uint32_t gain2 = 3u;
                static const uint32_t toggle = 4u;
                static const uint32_t cutoff = 5u;
                static const uint32_t stages = 6u;
                static const uint32_t resonance = 7u;
                static const uint32_t factor = 8u;
                static const uint32_t eps = 9u;
                static const uint32_t tension = 10u;
            };
            static const uint32_t ports = 11u;
        };
        io_type * ports[constants::ports];
        BQFilter<type> filter[constants::max_stages][2u];
        BQFilter<type> filterlp[constants::max_stages][2u];
        Filter<type> splitter[constants::max_stages];
        Filter<type> highpass[constants::max_stages];
        BQFilter<type> gains[constants::max_stages];
        type sr;
        type buffer[constants::max_factor];
    public:
        explicit mndist(type rate) : sr(rate) {}
        ~mndist() {}
    private:
#ifdef USE_LUT
        type inline G(type u, type g, type tension=1.) {
            return lookup::lookup_table<type, lookup_parameters>::lookup(u, g, tension);
        }
#else
        type inline G(type u, type g, type tension=1.) {
            return functions::G<type,4u,1u>(u, g, tension);
        }
#endif
    public:
        void connect_port(const uint32_t port, void *data) {
            if (port < constants::ports)
                ports[port] = (io_type *) data;
        }
        void activate() {
            for (uint32_t j = 0; j < constants::max_stages; j++)
                for (uint32_t i = 0; i < 2u; i++) {
                    filter[j][i] = BQFilter<type>(8u);
                }
            for (uint32_t j = 0; j < constants::max_stages; j++) {
                highpass[j] = Filter<type>();
                highpass[j].setparams(0.001 * (48000.0 / sr), 0.900, 1.0);
            }
            for (uint32_t j = 0; j < constants::max_stages; j++) {
                gains[j] = BQFilter<type>(1u);
                gains[j].setk(0.0001 * 48000.0 / sr);
                gains[j].setq(0.625);
            }
            for (uint32_t j = 0; j < constants::max_stages; j++)
                for (uint32_t i = 0; i < 2u; i++) {
                    filterlp[j][i].setk(3600.0 / sr);
                    filterlp[j][i].setq(0.71);
                }
        }
        void inline run(const uint32_t n) {
            for (uint32_t i = 0; i < constants::ports; ++i)
                if (!ports[i]) return;

            // Ports
            io_type * const out = ports[constants::names::out];
            const io_type * const x = ports[constants::names::in];
            const type gain1 = *ports[constants::names::gain1];
            const type gain2 = *ports[constants::names::gain2];
            const uint32_t toggle = uint32_t(*ports[constants::names::toggle]);
            const type cutoff = *ports[constants::names::cutoff];
            const type resonance = *ports[constants::names::resonance];
            const type eps = *ports[constants::names::eps];
            const type tension = *ports[constants::names::tension];
            uint32_t const factor = *ports[constants::names::factor];
            uint32_t const stages = uint32_t(*ports[constants::names::stages]);
            const type gain = (1-toggle)*gain1 + (toggle)*gain2;

            // Preprocessing
            const uint32_t sampling = factor;
            for (uint32_t j = 0; j < constants::max_stages; j++) {
                for (uint32_t i = 0; i < 2u; i++) {
                    filter[j][i].setk(0.5 / sampling);
                    filter[j][i].setq(0.625);
                }
                splitter[j].setparams(cutoff / sr, resonance, 1.0);
            }

            // Processing loop.
            for (uint32_t i = 0; i < n; ++i) {
                out[i] = x[i];
            }
            for (uint32_t i = 0; i < n; ++i) {
                type t = out[i];
                splitter[0].process(t);
                type bass = splitter[0].lp;
                t = splitter[0].hp;
                
                for (uint32_t h = 0; h < stages; ++h) {
                    t = t * gain;

                    filterlp[h][0].process(t);
                    t = filterlp[h][0].lp;
                    filter[h][0].process(t);
                    buffer[0] = filter[h][0].lp;
                    for (uint32_t j = 1; j < sampling; j++) {
                        filter[h][0].process(0.0);
                        buffer[j] = filter[h][0].lp;
                    }
                    for (uint32_t j = 0; j < sampling; j++) {
                        t = buffer[j] * sampling;
                        gains[h].process(1.0);
                        type g = G(t, std::abs(gains[h].lp), tension);
                        gains[h].process(g);
                        g = std::abs(gains[h].lp);
                        t = functions::S<type>(g * t, 1.);
                        buffer[j] = t;
                    }
                    for (uint32_t j = 0; j < sampling; j++) {
                        filter[h][1].process(buffer[j]);
                        t = filter[h][1].lp;
                    }
                    filterlp[h][1].process(t);
                    t = filterlp[h][1].lp;
                    highpass[h].process(t);
                    t = highpass[h].hp;
                }
                
                t = (1. - eps) * bass + eps * t;
                out[i] = t;
            }
        }
    };

    template <typename type, typename io_type>
    LV2_Handle instantiate (
        const struct LV2_Descriptor *descriptor,
        const double rate,
        const char *bundle_path,
        const LV2_Feature *const *features) 
    {
        mndist<type, io_type> *a = new mndist<type, io_type>{type(rate)};
        return a;
    }

    template <typename type, typename io_type>
    void connect_port (LV2_Handle instance, const uint32_t port, void *data)
    {
        mndist<type, io_type> *a = (mndist<type, io_type> *) instance;
        if (!a) return;
        a->connect_port(port, data);
    }

    template <typename type, typename io_type>
    void activate (LV2_Handle instance)
    {
        mndist<type, io_type> *a = (mndist<type, io_type> *) instance;
        if (!a) return;
        a->activate();
    }

    template <typename type, typename io_type>
    void inline run (LV2_Handle instance, const uint32_t n)
    {
        mndist<type, io_type> *amp = (mndist<type, io_type> *) instance;
        if (!amp) return;
        amp->run(n);
    }
    void deactivate (LV2_Handle instance)
    {}
    template <typename type, typename io_type>
    void cleanup (LV2_Handle instance)
    {
        mndist<type, io_type> *a = (mndist<type, io_type> *) instance;
        if (!a) return;
        delete a;
    }
    const void *extension_data (const char *uri)
    {
        return nullptr;
    }
}

// Plugin linkage.

#ifdef USE_DOUBLE
using processing_type = double;
using io_type = float;
#else
using processing_type = float;
using io_type = float;
#endif

static LV2_Descriptor const descriptor =
{
    .URI = "urn:omnp:mndist",
    .instantiate = mndist::instantiate<processing_type, io_type>,
    .connect_port = mndist::connect_port<processing_type, io_type>,
    .activate = mndist::activate<processing_type, io_type>,
    .run = mndist::run<processing_type, io_type>,
    .deactivate = mndist::deactivate,
    .cleanup = mndist::cleanup<processing_type, io_type>,
    .extension_data = mndist::extension_data,
};

LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor (const uint32_t index)
{
    if (index == 0) return &descriptor;
    return nullptr;
}