#pragma once

#include <list>
#include "math.h"

template <typename type>
class unit_delay {
private:
    type y;
    type saved_y;
public:
    unit_delay() : y{0.0}, saved_y{0.0} {
    }
    type process(type const x) {
        type y = this->y;
        this->y = x;
        return y;
    }
    void setparams(type const d) {
    }
    void reset() {
        y = saved_y;
    }
    void save_state() {
        saved_y = y;
    }
};

template <typename type>
class allpass {
private:
    type x;
    type y;
    type f;
    type saved_x, saved_y;
public:
    allpass() : x{0.0}, y{0.0}, f{(1.0-0.5)/(1.0+0.5)}, saved_x{0.0}, saved_y{0.0} {
    }
    type process(type const x) {
        type y = f*x + this->x - f*this->y;
        this->x = x;
        this->y = y;
        return y;
    }
    void setparams(type const d) {
        f = (1.0-d)/(1.0+d);
    }
    void reset() {
        this->x = saved_x;
        this->y = saved_y;
    }
    void save_state() {
        saved_x = x;
        saved_y = y;
    }
};

template <typename type, typename delay_type>
class second_order_section {
private:
    type b[3];
    type a[3];
    delay_type x[2];
    delay_type y[2];
    type out;
    type saved_out;
public:
    second_order_section(type b[3], type a[3]) {
        for (uint32_t i = 0; i < 3; i++) {
            this->b[i] = b[i];
            this->a[i] = a[i];
            out = 0.0;
            saved_out = 0.0;
        }
    }
    type const pass() const {
        return out;
    }
    type process(type const x) {
        type c[3];
        c[0] = x;
        for (uint32_t i = 1; i < 3; i++) {
            c[i] = this->x[i-1].process(c[i-1]);
        }
        type d[3];
        d[0] = out;
        for (uint32_t i = 1; i < 3; i++) {
            d[i] = this->y[i-1].process(d[i-1]);
        }
        type y = b[0]*c[0];
        for (uint32_t i = 1; i < 3; i++) {
            y += b[i]*c[i] - a[i] * d[i-1];
        }
        out = y;
        return y;
    }
    void setparams(type delay) {
        for (auto & x: this->x) {
            x.setparams(delay);
        }
        for (auto & y: this->y) {
            y.setparams(delay);
        }
    }
    void reset() {
        out = saved_out;
        for (auto & x: this->x) {
            x.reset();
        }
        for (auto & y: this->y) {
            y.reset();
        }
    }
    void save_state() {
        saved_out = out;
        for (auto & x: this->x) {
            x.save_state();
        }
        for (auto & y: this->y) {
            y.save_state();
        }
    }
};

template <typename type>
class filter_of_second_order_sections {
private:
    std::list<second_order_section<type, allpass<type>>> sections;
    type out;
    type saved_out;
public:
    filter_of_second_order_sections(uint32_t const number_of_sections, type const * coefficients) {
        for (uint32_t i = 0; i< number_of_sections; i++) {
            type b[3];
            type a[3];
            for (uint32_t s = 0; s < 3; s++) {
                b[s] = *coefficients++;
            }
            for (uint32_t s = 0; s < 3; s++) {
                a[s] = *coefficients++;
            }
            sections.push_back(second_order_section<type, allpass<type>>(b, a));
        }
        out = 0.0;
        saved_out = 0.0;
    }
    type const pass() const {
        return out;
    }
    type process(type const x) {
        type y = x;
        for (auto & section: sections) {
            y = section.process(y);
        }
        out = y;
        return y;
    }
    void setparams(type k, type q = 1.0, type sr = 1.0) {
        for (auto & section: sections) {
            section.setparams(0.5 + 0.5 * k);
        }
    }
    void reset() {
        out = saved_out;
        for (auto & section: sections) {
            section.reset();
        }
    }
    void save_state() {
        saved_out = out;
        for (auto & section: sections) {
            section.save_state();
        }
    }
};

template <typename type>
struct elliptic : public filter_of_second_order_sections<type> {
private:
#if 0
    static constexpr type const coefficients[36] = {
        0.006454787526846158737803271776556357508525252342224121093750000,0.012890658885432960609196406664977985201403498649597167968750000,0.006454787526846158737803271776556357508525252342224121093750000,1.000000000000000000000000000000000000000000000000000000000000000,0.169966609543426516726327690776088275015354156494140625000000000,0.015052717847847542353978411711068474687635898590087890625000000,
        1.000000000000000000000000000000000000000000000000000000000000000,1.975018372878258654523619952669832855463027954101562500000000000,1.000000000000000222044604925031308084726333618164062500000000000,1.000000000000000000000000000000000000000000000000000000000000000,0.223685318484879047673530294559895992279052734375000000000000000,0.080450051735102362515661411634937394410371780395507812500000000,
        1.000000000000000000000000000000000000000000000000000000000000000,1.937677967760867092650300946843344718217849731445312500000000000,0.999999999999999888977697537484345957636833190917968750000000000,1.000000000000000000000000000000000000000000000000000000000000000,0.319650620317228073563597945394576527178287506103515625000000000,0.202582875080657004440709556547517422586679458618164062500000000,
        1.000000000000000000000000000000000000000000000000000000000000000,1.895856889569053072008841809292789548635482788085937500000000000,1.000000000000000000000000000000000000000000000000000000000000000,1.000000000000000000000000000000000000000000000000000000000000000,0.440856244198904068110067555608111433684825897216796875000000000,0.370456846880764356644277768282336182892322540283203125000000000,
        1.000000000000000000000000000000000000000000000000000000000000000,1.860713727074052581400565031799487769603729248046875000000000000,1.000000000000000222044604925031308084726333618164062500000000000,1.000000000000000000000000000000000000000000000000000000000000000,0.572054792763574848635244052275083959102630615234375000000000000,0.579471381427372311812007410480873659253120422363281250000000000,
        1.000000000000000000000000000000000000000000000000000000000000000,1.840861672787295955089348353794775903224945068359375000000000000,1.000000000000000000000000000000000000000000000000000000000000000,1.000000000000000000000000000000000000000000000000000000000000000,0.704553845315165738760754265967989340424537658691406250000000000,0.840123363475171691661103068327065557241439819335937500000000000,
    };
#else
    static constexpr type const coefficients[48] = {
        0.000537182291099954560380236312511215146514587104320526123046875,0.001070471062759227424965313701932245749048888683319091796875000,0.000537182291099954560380236312511215146514587104320526123046875,1.000000000000000000000000000000000000000000000000000000000000000,-0.132785891567293756754253308827173896133899688720703125000000000,0.011244438005022680884814612056743499124422669410705566406250000,
        1.000000000000000000000000000000000000000000000000000000000000000,1.937749128393652764401622334844432771205902099609375000000000000,0.999999999999999888977697537484345957636833190917968750000000000,1.000000000000000000000000000000000000000000000000000000000000000,-0.092243261489979078149303859390784054994583129882812500000000000,0.061405075610963713583778655902278842404484748840332031250000000,
        1.000000000000000000000000000000000000000000000000000000000000000,1.841918526327663174768645149015355855226516723632812500000000000,1.000000000000000222044604925031308084726333618164062500000000000,1.000000000000000000000000000000000000000000000000000000000000000,-0.019312903953970395054540176715818233788013458251953125000000000,0.153718325438834818585931429879565257579088211059570312500000000,
        1.000000000000000000000000000000000000000000000000000000000000000,1.726943231485984719952853083668742328882217407226562500000000000,0.999999999999999777955395074968691915273666381835937500000000000,1.000000000000000000000000000000000000000000000000000000000000000,0.073011280627985966629189817922451766207814216613769531250000000,0.275822761676608130265719864837592467665672302246093750000000000,
        1.000000000000000000000000000000000000000000000000000000000000000,1.613966469077836585199747787555679678916931152343750000000000000,0.999999999999999777955395074968691915273666381835937500000000000,1.000000000000000000000000000000000000000000000000000000000000000,0.171257070091513446952191657146613579243421554565429687500000000,0.415922957927145675594005069797276519238948822021484375000000000,
        1.000000000000000000000000000000000000000000000000000000000000000,1.518845547185534217149438518390525132417678833007812500000000000,1.000000000000000222044604925031308084726333618164062500000000000,1.000000000000000000000000000000000000000000000000000000000000000,0.264532585053095237181963739203638397157192230224609375000000000,0.566555218678222560768631410610396414995193481445312500000000000,
        1.000000000000000000000000000000000000000000000000000000000000000,1.451367373878487798677383580070454627275466918945312500000000000,1.000000000000000000000000000000000000000000000000000000000000000,1.000000000000000000000000000000000000000000000000000000000000000,0.345488132206482556618709622853202745318412780761718750000000000,0.726537656947958465636361324868630617856979370117187500000000000,
        1.000000000000000000000000000000000000000000000000000000000000000,1.416609242818929415008710748224984854459762573242187500000000000,0.999999999999999777955395074968691915273666381835937500000000000,1.000000000000000000000000000000000000000000000000000000000000000,0.409689608992544529453283530529006384313106536865234375000000000,0.902050034666729283472363931650761514902114868164062500000000000,
    };
#endif
public:
    elliptic() : filter_of_second_order_sections<type>{8, coefficients} {
    }
};