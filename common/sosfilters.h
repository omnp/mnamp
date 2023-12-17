#pragma once

#include <list>
#include "math.h"

template <typename type>
class unit_delay {
private:
    type y;
public:
    unit_delay() : y{0.0} {
    }
    type process(type const x) {
        type y = this->y;
        this->y = x;
        return y;
    }
    void setparams(type const d) {
    }
    void reset() {
        y = 0.0;
    }
};

template <typename type>
class allpass {
private:
    type x;
    type y;
    type f;
public:
    allpass() : x{0.0}, y{0.0}, f{1.0-0.5/1.0+0.5} {
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
        this->x = 0.0;
        this->y = 0.0;
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
public:
    second_order_section(type b[3], type a[3]) {
        for (uint32_t i = 0; i < 3; i++) {
            this->b[i] = b[i];
            this->a[i] = a[i];
            out = 0.0;
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
        out = 0.0;
        for (auto & x: this->x) {
            x.reset();
        }
        for (auto & y: this->y) {
            y.reset();
        }
    }
};

template <typename type>
class filter_of_second_order_sections {
private:
    std::list<second_order_section<type, allpass<type>>> sections;
    type out;
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
        out = 0.0;
        for (auto & section: sections) {
            section.reset();
        }
    }
};

template <typename type>
struct elliptic : public filter_of_second_order_sections<type> {
private:
#if 0
    static constexpr type const coefficients[48] = {
        0.00026022352951219306,0.0005179331011814694,0.000260223529512193,1.0,-0.3655164591307115,0.04212240009290082,
        1.0,1.9176006070965395,1.0000000000000002,1.0,-0.31063833850009076,0.09877626385672311,
        1.0,1.7932524394352412,1.0000000000000002,1.0,-0.21430878766643274,0.20046828955662493,
        1.0,1.647957876211558,1.0,1.0,-0.09692948590068894,0.3299900260444628,
        1.0,1.509207870578951,1.0,1.0,0.021846816492606714,0.4718719926499198,
        1.0,1.3953934513158361,0.9999999999999998,1.0,0.12752251743395235,0.6168980734275151,
        1.0,1.3162858663701156,1.0,1.0,0.21115732852085423,0.7633434849152582,
        1.0,1.2760586721095413,1.0,1.0,0.26740711881203233,0.916800765217106,
    };
#elif 0
    static constexpr type const coefficients[24] = {
        0.05395999780114766,0.107887382886284,0.05395999780114763,1.0,0.40615677322734156,0.054221870653169336,
        1.0,1.9951064708380206,0.9999999999999997,1.0,0.5073836200650397,0.17573570497482588,
        1.0,1.9890642194164385,1.0000000000000004,1.0,0.6835196180419236,0.4067166089586155,
        1.0,1.984808239305464,1.0000000000000002,1.0,0.9105177166266577,0.7569388493312981,
    };
#elif 0
    static constexpr type const coefficients[36] = {
        0.006454787526846159,0.01289065888543296,0.006454787526846159,1.0,0.16996660954342652,0.015052717847847542,
        1.0,1.9750183728782587,1.0000000000000002,1.0,0.22368531848487905,0.08045005173510236,
        1.0,1.937677967760867,0.9999999999999999,1.0,0.3196506203172281,0.202582875080657,
        1.0,1.895856889569053,1.0,1.0,0.44085624419890407,0.37045684688076436,
        1.0,1.8607137270740526,1.0000000000000002,1.0,0.5720547927635748,0.5794713814273723,
        1.0,1.840861672787296,1.0,1.0,0.7045538453151657,0.8401233634751717,
    };
#elif 0
    static constexpr type const coefficients[36] = {
        0.00048017353596342340774430490490942702308529987931251526,0.00095590657443617871053681556148262643546331673860549927,0.00048017353596342319090387040780854022159473970532417297,1.00000000000000000000000000000000000000000000000000000000,-0.76650438312137991747619025773019529879093170166015625000,0.16759968153777776711521596553211566060781478881835937500,
        1.00000000000000000000000000000000000000000000000000000000,1.92255672933091936727123538730666041374206542968750000000,0.99999999999999966693309261245303787291049957275390625000,1.00000000000000000000000000000000000000000000000000000000,-0.61624079751998150911163065757136791944503784179687500000,0.25994651030503729272069790567911695688962936401367187500,
        1.00000000000000000000000000000000000000000000000000000000,1.81241212836464082869269986986182630062103271484375000000,0.99999999999999966693309261245303787291049957275390625000,1.00000000000000000000000000000000000000000000000000000000,-0.38537532298149723697378021824988536536693572998046875000,0.40924252135143335262768005122779868543148040771484375000,
        1.00000000000000000000000000000000000000000000000000000000,1.69644182473612437078713810478802770376205444335937500000,0.99999999999999977795539507496869191527366638183593750000,1.00000000000000000000000000000000000000000000000000000000,-0.15435451995098858901656058151274919509887695312500000000,0.57677938205213274969196390884462743997573852539062500000,
        1.00000000000000000000000000000000000000000000000000000000,1.60455054518762629811590159079059958457946777343750000000,0.99999999999999988897769753748434595763683319091796875000,1.00000000000000000000000000000000000000000000000000000000,0.02558675800450352713633073165055975550785660743713378906,0.74330730919781451415673245719517581164836883544921875000,
        1.00000000000000000000000000000000000000000000000000000000,1.55473890290099192590389520773896947503089904785156250000,0.99999999999999966693309261245303787291049957275390625000,1.00000000000000000000000000000000000000000000000000000000,0.13013093936499886549285065484582446515560150146484375000,0.91069068871117331287479146340047009289264678955078125000,
    };
#elif 0
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