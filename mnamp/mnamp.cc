#include <lv2/core/lv2.h>
#include "../common/math.h"
#include "../common/resampler.h"
#include "../common/svfilter.h"
#include "../common/soft.h"

namespace mnamp {

    template<typename type, typename io_type>
    class mnamp
    {
    private:
        type sr;
        struct constants {
            static const uint32_t max_stages {4u};
            static const uint32_t max_factor {128u};
            struct names {
                static const uint32_t out = 0u;
                static const uint32_t in = 1u;
                static const uint32_t gain1 = 2u;
                static const uint32_t gain2 = 3u;
                static const uint32_t toggle = 4u;
                static const uint32_t cutoff = 5u;
                static const uint32_t stages = 6u;
                static const uint32_t bias = 7u;
                static const uint32_t resonance = 8u;
                static const uint32_t factor = 9u;
                static const uint32_t eps = 10u;
                static const uint32_t eq = 11u;
                static const uint32_t compensation = 12u;
                static const uint32_t volume = 13u;
            };
            static const uint32_t ports = 14u;
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
            SVFilterAdapter<filters::lowpass, type> input_filter;
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

        port_parameter<constants::names::gain1, type, constants::conversion::db> gain1{this};
        port_parameter<constants::names::gain2, type, constants::conversion::db> gain2{this};
        port_parameter<constants::names::toggle, uint32_t, constants::conversion::none> toggle{this};
        port_parameter<constants::names::cutoff> cutoff{this};
        port_parameter<constants::names::stages, uint32_t, constants::conversion::none> stages{this};
        port_parameter<constants::names::bias> bias{this};
        port_parameter<constants::names::resonance> resonance{this};
        port_parameter<constants::names::factor, uint32_t, constants::conversion::none> factor{this};
        port_parameter<constants::names::eps> eps{this};
        port_parameter<constants::names::eq> eq{this};
        port_parameter<constants::names::compensation, type, constants::conversion::db> compensation{this};
        port_parameter<constants::names::volume, type, constants::conversion::db> volume{this};

        using Lowpass = SVFilterAdapter<filters::lowpass, type>;
        using LowpassCascade = filter_cascade<type, Lowpass, 2u>;
        using Highpass = SVFilterAdapter<filters::highpass, type>;
        using HighpassCascade = filter_cascade<type, Highpass, 2u>;
        
        Lowpass gain_filter;
        LowpassCascade lowpass[constants::max_stages];
        SVFilter<type> splitter[constants::max_stages];
        HighpassCascade highpass[2u*constants::max_stages];
        filter_parameters<type, LowpassCascade> lowpass_filter_parameters;
        filter_parameters<type, HighpassCascade> highpass_filter_parameters;

        resampler<type, SVFilterAdapter<filters::lowpass, type>, constants::max_factor> oversamplers[constants::max_stages];
        type const nl_cutoff_base = 2000.0;
        type nl_cutoff = nl_cutoff_base;
        SVFilter<type> nl_highpass[constants::max_stages];
        SVFilter<type> nl_lowpass0[constants::max_stages];
        SVFilter<type> nl_lowpass1[constants::max_stages];

    public:
        explicit mnamp(type rate) : sr(rate) {
            for (uint32_t i = 0; i < constants::max_stages; i++) {
                lowpass_filter_parameters.append(lowpass[i]);
                highpass_filter_parameters.append(highpass[2*i]);
                highpass_filter_parameters.append(highpass[2*i+1]);
            }
        }
        ~mnamp() {}
    private:
    public:
        void connect_port(const uint32_t port, void *data) {
            if (port < constants::ports)
                ports[port] = (io_type *) data;
        }
        void activate() {
            gain_filter.setparams(1000.0, 0.707, sr);
            highpass_filter_parameters.setparams(25.0, 0.404, sr);
            lowpass_filter_parameters.setparams(19000.0, 0.707, sr);
            for (uint32_t i = 0; i < constants::max_stages; ++i) {
                oversamplers[i].upsampler.setparams(0.475 / 2, 0.606, 1.0);
                oversamplers[i].downsampler.setparams(0.475 / 2, 0.606, 1.0);
                oversamplers[i].upfactor = 2;
                oversamplers[i].downfactor = 2;
                nl_highpass[i].setparams(0.25 / 2, 0.606, 1.0);
                nl_lowpass0[i].setparams(nl_cutoff / sr, 0.606, 1.0);
                nl_lowpass1[i].setparams(nl_cutoff / sr, 0.606, 1.0);
            }
        }
        void inline run(const uint32_t n) {
            for (uint32_t i = 0; i < constants::ports; ++i)
                if (!ports[i]) return;

            // Ports
            io_type * const out = ports[constants::names::out];
            const io_type * const x = ports[constants::names::in];
            const type gain1 = this->gain1();
            const type gain2 = this->gain2();
            const uint32_t toggle = this->toggle();
            const type cutoff = this->cutoff();
            const type bias = this->bias();
            const type resonance = this->resonance();
            const type eps = std::sqrt(this->eps());
            uint32_t const factor = this->factor();
            uint32_t const stages = this->stages();
            gain_filter.process((1-toggle)*gain1 + (toggle)*gain2);
            const type gain = gain_filter.pass();
            const type mix = std::sqrt(this->eq());
            const type compensation = this->compensation();
            const type volume = this->volume();

            // Preprocessing
            const uint32_t sampling = 2u;
            for (uint32_t j = 0; j < constants::max_stages; j++) {
                splitter[j].setparams(cutoff, resonance * 1000.0 / (1.0 + cutoff), sr);
                oversamplers[j].upsampler.setparams(0.475 / sampling, 0.606, 1.0);
                oversamplers[j].downsampler.setparams(0.475 / sampling, 0.606, 1.0);
                oversamplers[j].upfactor = sampling;
                oversamplers[j].downfactor = sampling;
                highpass_filter_parameters.setparams(25.0/sr, 0.404, 1.0);
                lowpass_filter_parameters.setparams(19000.0/sr, 0.707, 1.0);
                nl_highpass[j].setparams(0.25/sampling, 0.606, 1.0);
                nl_lowpass0[j].setparams(nl_cutoff / sr, 0.606, 1.0);
                nl_lowpass1[j].setparams(nl_cutoff / sr, 0.606, 1.0);
            }

            // Processing loop.
            for (uint32_t i = 0; i < n; ++i) {
                type t = x[i];

                splitter[0].process(t);
                type bass = splitter[0].lp;
                type high = splitter[0].hp;
                t = splitter[0].bp;
                bass = (1. - mix) * bass + mix * high;
                t = (1. - eps) * bass + eps * t;
                t = t * gain;

                for (uint32_t h = 0; h < stages; ++h) {
                    highpass[2*h].process(t);
                    t = highpass[2*h].pass();
                    lowpass[h].process(t);
                    t = lowpass[h].pass();

                    nl_lowpass0[h].process(t);
                    oversamplers[h].upsample(nl_lowpass0[h].lp);
                    type energy_low = 0.0;
                    type energy_high = 0.0;
                    for (uint32_t j = 0; j < sampling; ++j) {
                        type t_ = oversamplers[h].buffer[j] * sampling;
                        t_ = g<type>(t_, 1.0, bias / (h + 1));
                        nl_highpass[h].process(t_);
                        type t__ = nl_highpass[h].lp;
                        energy_high += (t_ - t__)*(t_ - t__);
                        energy_low += t__ * t__;
                    }
                    if (energy_high > 0.0) {
                        nl_cutoff = nl_cutoff_base / (1.0 + energy_high);
                        nl_lowpass0[h].setparams(nl_cutoff / sr, 0.606, 1.0);
                        nl_lowpass1[h].setparams(nl_cutoff / sr, 0.606, 1.0);
                    }
                    nl_lowpass1[h].process(t);
                    t = nl_lowpass1[h].lp;
                    t = g<type>(t, 1.0, bias / (h + 1));

                    highpass[2*h+1].process(t);
                    t = highpass[2*h+1].pass();
                    t = t * compensation;
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
