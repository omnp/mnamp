#include <functional>
#include <lv2/core/lv2.h>
#include "../common/onepole.h"

namespace mnamp {

    namespace curves {
        template<typename type> type f0(type x, type level=0.0) {
            return x;
        }
        template<typename type> type f1(type x, type level=0.0) {
            return (2.+level)*x/(1.+level)-x*std::abs(x)/(1.+level);
        }
        template<typename type> type f2(type x, type level=0.0) {
            return (3.+level)*x/(2.+level)-x*x*x/(2.+level);
        }
        template<typename type>
        std::function<type(type, type)> combine(std::function<type(type, type)> const & below,
                     std::function<type(type, type)> const & above, std::function<type(void)> const & threshold) {
            return [below, above, &threshold](type x, type level=0.0) {
                type t = 0.5*(1.0 + x) * threshold(); return (1.0-t)*below(x, level)+t*above(x, level);
            };
        }
        template <typename type>
        std::function<type(type, type)> invert(std::function<type(type, type)> const & f) {
            return [f](type x, type level=0.0) {
                x = -x;
                type y = f(x, level);
                y = -y;
                return y;
            };
        }
    };
    template <typename type> type clip(type x, type min, type max) {
        if (x < min) {
            x = min;
        }
        if (x > max) {
            x = max;
        }
        return x;
    }
    template <typename type>
    class Limit
    {
    private:
        OnePole<type> lowpass;
        OnePole<type> gain;
    public:
        Limit() {
            lowpass.setparams(0.5, 1.0, 1.0);
            gain.setparams(0.5, 1.0, 1.0);
        }
        type process(type const x) {
            type gain_ = 1.0;
            if (std::abs(x) > 1.0) {
                gain_ = std::fmin<type>(gain.pass(), 1.0 / std::abs(x));
            }
            type y = gain_ * x;
            lowpass.process(std::abs(x));
            if (std::abs(lowpass.pass()) <= 0.0) {
                gain.process(1.0);
            }
            else {
                gain.process(gain_);
            }
            return y;
        }
        void set_lowpass_params(type k, type q, type sr) {
            lowpass.setparams(k, q, sr);
        }
        void set_gain_params(type k, type q, type sr) {
            gain.setparams(k, q, sr);
        }
    };

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
                static const uint32_t resonance = 4u;
                static const uint32_t eps = 5u;
                static const uint32_t eq = 6u;
                static const uint32_t compensation = 7u;
                static const uint32_t volume = 8u;
                static const uint32_t gain = 9u;
                static const uint32_t curve = 10u;
                static const uint32_t threshold = 11u;
            };
            static const uint32_t ports = 12u;
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
            OnePole<type> input_filter;
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
        port_parameter<constants::names::resonance> resonance{this};
        port_parameter<constants::names::eps> eps{this};
        port_parameter<constants::names::eq> eq{this};
        port_parameter<constants::names::compensation, type, constants::conversion::db> compensation{this};
        port_parameter<constants::names::volume, type, constants::conversion::db> volume{this};
        port_parameter<constants::names::gain> gain{this};
        port_parameter<constants::names::curve, uint32_t, constants::conversion::none> curve_index{this};
        port_parameter<constants::names::threshold> threshold{this};

        using Lowpass = OnePole<type>;
        using LowpassCascade = filter_cascade<type, Lowpass, 2u>;
        using Highpass = OnePoleHigh<OnePole<type>>;
        using HighpassCascade = filter_cascade<type, Highpass, 2u>;

        LowpassCascade lowpass;
        filter_cascade<type, Lowpass, 1u> splitter;
        filter_cascade<type, Highpass, 1u> splitter_high;
        HighpassCascade highpass[1u+constants::max_stages];
        filter_parameters<type, LowpassCascade> lowpass_filter_parameters;
        filter_parameters<type, HighpassCascade> highpass_filter_parameters;

        LowpassCascade adjust[constants::max_stages];
        type const max_gain = 24.0;
        type const downfilter_factor = 6.0;
        Limit<type> main_limiter;
        std::array<Limit<type>, constants::max_stages> limiters;
        std::function<type(void)> const basic_threshold = []{return 1.0;};
        std::function<type(void)> const active_threshold = [this]{return this->threshold();};
        std::function<type(type, type)> const curve0 = curves::combine<type>(curves::f0<type>, curves::f1<type>, basic_threshold);
        std::function<type(type, type)> const curve1 = curves::combine<type>(curves::f2<type>, curves::f1<type>, basic_threshold);
        std::function<type(type, type)> const curve = curves::combine<type>(curve0, curve1, active_threshold);
        std::array<std::function<type(type, type)> const, 4u> selectable_curves
            {
                curve0,
                curve1,
                curve,
                curves::combine<type>(
                    curves::combine<type>(curves::f0<type>, curves::f2<type>, basic_threshold),
                    curves::combine<type>(curves::f1<type>, curves::f1<type>, basic_threshold),
                    active_threshold),
            };

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
            highpass_filter_parameters.setparams(37.5, 0.404, sr);
            lowpass_filter_parameters.setparams(22000.0, 0.707, sr);
            for (uint32_t h = 0; h < constants::max_stages; h++) {
                adjust[h].setparams(0.5*sr/downfilter_factor, 0.606, sr);
                limiters[h].set_lowpass_params(0.1, 1.0, sr);
                limiters[h].set_gain_params(0.1, 1.0, sr);
            }
            main_limiter.set_lowpass_params(0.1, 1.0, sr);
            main_limiter.set_gain_params(0.1, 1.0, sr);
        }
        void inline run(const uint32_t n) {
            for (uint32_t i = 0; i < constants::ports; ++i)
                if (!ports[i]) return;

            // Ports
            io_type * const out = ports[constants::names::out];
            const io_type * const x = ports[constants::names::in];
            const type cutoff = this->cutoff();
            const type resonance = this->resonance();
            const type eps = std::sqrt(this->eps());
            uint32_t const stages = this->stages();
            const type mix = std::sqrt(this->eq());
            const type compensation = this->compensation();
            const type volume = this->volume();
            const type gain = this->gain();
            uint32_t const selected_curve_index = this->curve_index();

            // Preprocessing
            splitter.setparams(cutoff / sr, 1.0  * 1000.0 / (1.0 + cutoff), 1.0);
            splitter_high.setparams((6000.0 - cutoff) / sr, 1.0 * 1000.0 / (1.0 + cutoff), 1.0);

            // Processing loop.
            for (uint32_t i = 0; i < n; ++i) {
                type t = x[i];

                highpass[0].process(t);
                t = highpass[0].pass();
                lowpass.process(t);
                t = lowpass.pass();

                splitter.process(t);
                splitter_high.process(t);
                type bass = splitter.pass();
                type high = splitter_high.pass();
                type mid = t - bass - high;
                t = (1. - mix) * bass + resonance * mid + mix * high;
                t = main_limiter.process(t);

                for (uint32_t h = 0; h < stages; ++h) {
                    t = limiters[h].process(t);
                    type a = std::abs(t);
                    a = a/(1.0 + a);
                    a = std::log2(2.0 - a);
                    type level = (max_gain - gain) * a;
                    adjust[h].setparams(0.5 * sr/downfilter_factor * a, 0.606, sr);
                    adjust[h].process(t);
                    type lo = adjust[h].pass();
                    t = lo;
                    t = selectable_curves[selected_curve_index](t, level);
                    t = (1. - eps) * lo + eps * t;
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
