#include <lv2/core/lv2.h>
#include "../common/onepole.h"

namespace mnamp {

    template<typename type, typename io_type>
    class mnamp
    {
    private:
        type sr;
        struct constants {
            static const uint32_t max_stages {32u};
            struct names {
                static const uint32_t out = 0u;
                static const uint32_t in = 1u;
                static const uint32_t cutoff = 2u;
                static const uint32_t stages = 3u;
                static const uint32_t bias = 4u;
                static const uint32_t resonance = 5u;
                static const uint32_t eps = 6u;
                static const uint32_t eq = 7u;
                static const uint32_t compensation = 8u;
                static const uint32_t volume = 9u;
                static const uint32_t gain = 10u;
            };
            static const uint32_t ports = 11u;
            enum struct conversion {none = 0u, linear = 1u, db = 2u};
        };
        io_type * ports[constants::ports];

        inline io_type * port(uint32_t name) const {
            return ports[name];
        }

        template <uint32_t name = constants::ports, typename return_type = type, const typename constants::conversion c = constants::conversion::linear>
        struct port_parameter {
            explicit port_parameter(mnamp const * m) : m{m} {
                input_filter.setparams(1000.0, 0.707, m->sr);
            }
            mnamp const * m;
            uint32_t const index = name;
            OnePoleZD<type> input_filter;
            return_type operator()() {
                type in = *m->port(name);
                switch (c) {
                    case constants::conversion::db:
                        in = math::dbl(in);
                        input_filter.process(in);
                        in = input_filter.pass();
                        break;
                    case constants::conversion::linear:
                        input_filter.process(in);
                        in = input_filter.pass();
                        break;
                    default:
                        break;
                }
                return in;
            }
        };

        port_parameter<constants::names::cutoff> cutoff{this};
        port_parameter<constants::names::stages, uint32_t, constants::conversion::none> stages{this};
        port_parameter<constants::names::bias> bias{this};
        port_parameter<constants::names::resonance> resonance{this};
        port_parameter<constants::names::eps> eps{this};
        port_parameter<constants::names::eq> eq{this};
        port_parameter<constants::names::compensation, type, constants::conversion::db> compensation{this};
        port_parameter<constants::names::volume, type, constants::conversion::db> volume{this};
        port_parameter<constants::names::gain> gain{this};

        using Lowpass = OnePoleZD<type>;
        using LowpassCascade = filter_cascade<type, Lowpass, 6u>;
        using Highpass = OnePoleHigh<OnePoleZD<type>>;
        using HighpassCascade = filter_cascade<type, Highpass, 6u>;

        LowpassCascade lowpass;
        LowpassCascade splitter;
        HighpassCascade highpass[1u+constants::max_stages];
        filter_parameters<type, LowpassCascade> lowpass_filter_parameters;
        filter_parameters<type, HighpassCascade> highpass_filter_parameters;

        LowpassCascade adjust[constants::max_stages];
        type const max_gain = 24.0;

    public:
        explicit mnamp(type rate) : sr(rate) {
            lowpass_filter_parameters.append(lowpass);
            for (uint32_t h = 0; h < 1u + constants::max_stages; h++)
                highpass_filter_parameters.append(highpass[h]);
        }
        ~mnamp() {}
    private:
    public:
        void connect_port(const uint32_t port, void *data) {
            if (port < constants::ports)
                ports[port] = (io_type *) data;
        }
        void activate() {
            highpass_filter_parameters.setparams(15.0, 0.404, sr);
            lowpass_filter_parameters.setparams(19000.0, 0.707, sr);
            for (uint32_t h = 0; h < constants::max_stages; h++) {
                adjust[h].setparams(0.5*sr/8, 0.606, 1.0);
            }
        }
        void inline run(const uint32_t n) {
            for (uint32_t i = 0; i < constants::ports; ++i)
                if (!ports[i]) return;

            // Ports
            io_type * const out = ports[constants::names::out];
            const io_type * const x = ports[constants::names::in];
            const type cutoff = this->cutoff();
            const type bias = this->bias();
            const type resonance = this->resonance();
            const type eps = std::sqrt(this->eps());
            uint32_t const stages = this->stages();
            const type mix = std::sqrt(this->eq());
            const type compensation = this->compensation();
            const type volume = this->volume();
            const type gain = this->gain();

            // Preprocessing
            splitter.setparams(cutoff, resonance * 1000.0 / (1.0 + cutoff), sr);
            highpass_filter_parameters.setparams(15.0/sr, 0.404, 1.0);
            lowpass_filter_parameters.setparams(19000.0/sr, 0.707, 1.0);

            // Processing loop.
            for (uint32_t i = 0; i < n; ++i) {
                type t = x[i];

                highpass[0].process(t);
                t = highpass[0].pass();
                lowpass.process(t);
                t = lowpass.pass();

                splitter.process(t);
                type bass = splitter.pass();
                type high = t - bass;
                bass = (1. - mix) * bass + mix * high;
                t = bass;

                for (uint32_t h = 0; h < stages; ++h) {
                    type a = std::abs(t);
                    a = a/(1.0 + a);
                    type b = a;
                    a = 1.0 - a;
                    adjust[h].setparams(0.5 * sr/5 * a, 0.606, 1.0);
                    adjust[h].process(t);
                    type lo = adjust[h].pass();
                    t = lo;
                    t = t + std::abs(t) * bias;
                    t = t / 2.0;
                    // type ga = max_gain - gain;
                    type ga = max_gain - gain;
                    t = ((5.0+ga)/(4.0+ga))*t - (t*t*t*t*t)/(4.0+ga);
                    t = (1. - eps * b) * lo + eps * b * t;
                    t = t * compensation;
                    highpass[h+1].process(t);
                    t = highpass[h+1].pass();
                }
                t = t * volume;
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
        mnamp<type, io_type> *a = new mnamp<type, io_type>{type(rate)};
        return a;
    }

    template <typename type, typename io_type>
    void connect_port (LV2_Handle instance, const uint32_t port, void *data)
    {
        mnamp<type, io_type> *a = (mnamp<type, io_type> *) instance;
        if (!a) return;
        a->connect_port(port, data);
    }

    template <typename type, typename io_type>
    void activate (LV2_Handle instance)
    {
        mnamp<type, io_type> *a = (mnamp<type, io_type> *) instance;
        if (!a) return;
        a->activate();
    }

    template <typename type, typename io_type>
    void inline run (LV2_Handle instance, const uint32_t n)
    {
        mnamp<type, io_type> *amp = (mnamp<type, io_type> *) instance;
        if (!amp) return;
        amp->run(n);
    }
    void deactivate (LV2_Handle instance)
    {}
    template <typename type, typename io_type>
    void cleanup (LV2_Handle instance)
    {
        mnamp<type, io_type> *a = (mnamp<type, io_type> *) instance;
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
