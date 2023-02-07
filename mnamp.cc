#include <lv2/core/lv2.h>
#include "svfilter.h"
#include "functions.h"

namespace mnamp {
    const uint32_t MNAMP_N_PORTS {23u};
    template<typename type, typename input_type>
    struct mnamp
    /* This is the amp's structure.
    */
    {
        input_type * ports[MNAMP_N_PORTS];
        Filter<type> filter[6];
        type sr;
        type mem[8];
        input_type params[MNAMP_N_PORTS];
        type processed[MNAMP_N_PORTS + 9];
        type psr;
        static enum {
            out = 0,
            in = 1,
            gain = 2,
            drive1 = 3,
            drive2 = 4,
            drive3 = 5,
            drive4 = 6,
            blend = 7,
            level = 8,
            hp = 9,
            qhp = 10,
            lp = 11,
            qlp = 12,
            mlp = 13,
            qmlp = 14,
            k = 15,
            h = 16,
            mode = 17,
            factor = 18,
            offset = 19,
            evened = 20,
            divider = 21,
            post = 22,
            a = 23, b = 24, c = 25, d = 26, e = 27, f = 28, 
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

        for (uint32_t i = 0; i < 6u; i++)
            a->filter[i] = Filter<type>();
        for (uint32_t i = 0; i < 8u; i++)
            a->mem[i] = 0.0f;
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
        const type drive3 = *amp->ports[amp->drive3];
        const type drive4 = *amp->ports[amp->drive4];
        const type blend = *amp->ports[amp->blend];
        const type level = *amp->ports[amp->level];
        const type hp = *amp->ports[amp->hp];
        const type Qhp = *amp->ports[amp->qhp];
        type lp = *amp->ports[amp->lp];
        const type Qlp = *amp->ports[amp->qlp];
        type mlp = *amp->ports[amp->mlp];
        const type Qmlp = *amp->ports[amp->qmlp];
        uint32_t const k = uint32_t(*amp->ports[amp->k]);
        uint32_t const h = uint32_t(*amp->ports[amp->h]);
        uint32_t const mode = uint32_t(*amp->ports[amp->mode]);
        uint32_t const factor = uint32_t(*amp->ports[amp->factor]);
        type const offset = *amp->ports[amp->offset];
        type const evened = *amp->ports[amp->evened];
        type const divider = *amp->ports[amp->divider];
        uint32_t const post = uint32_t(*amp->ports[amp->post]);
        
        // Preprocessing
        if (lp >= amp->sr / 2.0f)
            lp = amp->sr / 2.0f - 1.0f;
        if (mlp >= amp->sr)
            mlp = amp->sr;
        if (!amp->status || amp->sr != amp->psr || *amp->ports[amp->hp] != amp->params[amp->hp] || *amp->ports[amp->qhp] != amp->params[amp->qhp])
            amp->filter[0].setparams(hp, Qhp, amp->sr);
        if (!amp->status || amp->sr != amp->psr || *amp->ports[amp->lp] != amp->params[amp->lp] || *amp->ports[amp->qlp] != amp->params[amp->qlp])
            amp->filter[1].setparams(lp, Qlp, amp->sr);
        if (!amp->status || amp->sr != amp->psr) {
            amp->filter[2].setparams(amp->sr / 2.0f, 1.0f, factor*amp->sr);
            amp->filter[3].setparams(amp->sr / 2.0f, 1.0f, factor*amp->sr);
        }
        if (!amp->status || amp->sr != amp->psr || *amp->ports[amp->hp] != amp->params[amp->hp] || *amp->ports[amp->qhp] != amp->params[amp->qhp])
            amp->filter[4].setparams(hp, Qhp, amp->sr);
        if (!amp->status || amp->sr != amp->psr || *amp->ports[amp->mlp] != amp->params[amp->mlp] || *amp->ports[amp->qmlp] != amp->params[amp->qmlp])
            amp->filter[5].setparams(mlp, Qmlp, 2.0f*amp->sr);
        
        if (!amp->status || *amp->ports[amp->gain] != amp->params[amp->gain])
            amp->processed[amp->gain] = pow(10.0f, gain/20.0f);
        if (!amp->status || *amp->ports[amp->level] != amp->params[amp->level])
            amp->processed[amp->level] = pow(10.0f, level/20.0f);
        if (!amp->status || *amp->ports[amp->gain] != amp->params[amp->gain] || *amp->ports[amp->k] != amp->params[amp->k]) {
            if (!amp->status || *amp->ports[amp->k] != amp->params[amp->k])
                amp->processed[amp->a] = 100000.0f * OddPolynomial<type>(100000.0f, 100000.0f, k);
            amp->processed[amp->b] = OddPolynomial<type>(amp->processed[amp->gain], amp->processed[amp->a], k);
        }
        if (!amp->status || *amp->ports[amp->gain] != amp->params[amp->gain] || *amp->ports[amp->h] != amp->params[amp->h]) {
            if (!amp->status || *amp->ports[amp->h] != amp->params[amp->h])
                amp->processed[amp->c] = 100000.0f * EvenPolynomial<type>(100000.0f, 100000.0f, h);
            amp->processed[amp->d] = EvenPolynomial<type>(amp->processed[amp->gain], amp->processed[amp->c], h);
        }
        if (!amp->status)
            amp->processed[amp->e] = 1.0f * Exponential<type>(100000.0f, 1.0f);
        if (!amp->status || *amp->ports[amp->gain] != amp->params[amp->gain])
            amp->processed[amp->f] = Exponential<type>(amp->processed[amp->gain], amp->processed[amp->e]);
        if (!amp->status || *amp->ports[amp->drive1] != amp->params[amp->drive1])
            amp->processed[amp->drive1] = pow(10.0f, drive1/20.0f);
        if (!amp->status || *amp->ports[amp->drive2] != amp->params[amp->drive2])
            amp->processed[amp->drive2] = pow(10.0f, drive2/20.0f);
        if (!amp->status || *amp->ports[amp->drive3] != amp->params[amp->drive3])
            amp->processed[amp->drive3] = pow(10.0f, drive3/20.0f);
        if (!amp->status || *amp->ports[amp->drive4] != amp->params[amp->drive4])
            amp->processed[amp->drive4] = pow(10.0f, drive4/20.0f);
        type drive = 1.0f;
        type predrive = 1.0f;
        auto odd = [&drive,&predrive,&k,&amp](type const x) -> type {return OddPolynomial(predrive * amp->processed[amp->gain] * x, amp->processed[amp->a], k) * drive / amp->processed[amp->b];};
        auto evn = [&drive,&predrive,&h,&amp](type const x) -> type {return EvenPolynomial(predrive * amp->processed[amp->gain] * x, amp->processed[amp->c], h) * drive / amp->processed[amp->d];};
        auto exp = [&drive,&predrive,&amp](type const x) -> type {return Exponential(predrive * amp->processed[amp->gain] * x, amp->processed[amp->e]) * drive / amp->processed[amp->f];};
        // Post set values
        for (uint32_t i = 2; i < MNAMP_N_PORTS; i++)
            amp->params[i] = *amp->ports[i];
        amp->psr = amp->sr;
        amp->status = true;
        //
        
        // Processing loop.
        for (uint32_t i = 0; i < n; ++i) {
            amp->filter[0].process(x[i]);
            amp->filter[1].process(amp->filter[0].hp);
            type t = amp->filter[1].lp;
            type t_ = t + offset * t;
            type u = 0.0f;
            for (uint32_t j = 0; j < factor; j++) {
                amp->filter[2].process(t_);
                t_ = 0.0f;
                u = amp->filter[2].lp;
                drive = 1.0f;
                if (u > 0.0f)
                    drive = amp->processed[amp->drive1];
                if (u < 0.0f)
                    drive = amp->processed[amp->drive2];
                drive = 0.5 * (drive + amp->mem[1]);
                amp->mem[1] = drive;
                predrive = 1.0f;
                if (u > 0.0f)
                    predrive = amp->processed[amp->drive3];
                if (u < 0.0f)
                    predrive = amp->processed[amp->drive4];
                predrive = 0.5 * (predrive + amp->mem[2]);
                amp->mem[2] = predrive;
                switch (mode) {
                    default:
                    case 0:
                        u = u * factor;
                        break;
                    case 2:
                        u = exp(u*factor);
                        break;
                    case 3:
                        u = evn(u*factor);
                        break;
                    case 1:
                        u = odd(u*factor);
                        break;
                }
                type evnd {(1.0f - evened)*abs(u)};
                switch (post) {
                    default:
                    case 0:
                        break;
                    case 2:
                        u = (1.0f - evnd)*u + evnd * math::sgn<>(u) * evenpowers(abs(u), h, divider);
                        break;
                    case 1:
                        u = (1.0f - evnd)*u + evnd * u * oddpowers(abs(u), k, divider);
                        break;
                }
                amp->filter[3].process(u);
            }
            amp->filter[5].process(amp->filter[3].lp);
            u = (1.0f - blend) * amp->mem[3] + blend * amp->filter[5].lp;
            amp->mem[3] = t;
            amp->filter[4].process(u);
            out[i] = amp->processed[amp->level] * amp->filter[4].hp;
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