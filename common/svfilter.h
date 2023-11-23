#pragma once

#include "math.h"
#include "filter.h"

template<typename type>
struct SVFilter
/* This is a filter implemented from the following paper:
 * "Improving the Chamberlin Digital State Variable Filter"
 * by Victor Lazzarini, Joseph Timoney
 * https://arxiv.org/abs/2111.05592v2
 */
{
public:
    type s1,s2;
    type hp,bp,lp,br,ap;
    
    SVFilter() : s1(0.0f), s2(0.0f), hp(0.0f), bp(0.0f), lp(0.0f), br(0.0f), ap(0.0f)
    {
        setparams(1.0f, 1.0f, 2.0);
    }

    void process(const type x)
    {
        const type k = this->k;
        const type d = this->d;
        const type q = this->q;
        type u;
    
        this->hp = (x - (1.0 / q + k) * this->s1 - this->s2) / d;
        u = this->hp * k;
        this->bp = u + this->s1;
        this->s1 = u + this->bp;
        u = this->bp * k;
        this->lp = u + this->s2;
        this->s2 = u + this->lp;
        this->br = this->hp + this->lp;
        this->ap = this->br + (1.0/q) * this->bp;
    }
    void setparams(const type k, const type q, const type sr) {
        this->sr = sr;
        this->q = q;
        this->k = tan(M_PI * k / this->sr);
        this->d = 1.0 + this->k/this->q + this->k*this->k;
    }
    void reset() {
        this->s1 = 0.0;
        this->s2 = 0.0;
        this->ap = this->bp = this->br = this->hp = this->lp = 0.0;
    }
private:
    type k;
    type q;
    type d;
    type sr;
};

template<filters::filter_responses filter_response, typename type> struct SVFilterAdapter : public filter_adapter<filter_response, type, SVFilterAdapter<filter_response, type>>
{
private:
    explicit SVFilterAdapter() = 0;
};

template<typename type>
struct SVFilterAdapter<filters::lowpass, type>
{
private:
    SVFilter<type> svf;
public:
    inline type const pass() const {
        return svf.lp;
    }
    inline void process(const type x) {
        svf.process(x);
    };
    inline void reset() {
        svf.reset();
    };
    inline void setparams(type k, type q, type sr) {
        svf.setparams(k, q, sr);
    };    
};

template<typename type>
struct SVFilterAdapter<filters::highpass, type>
{
private:
    SVFilter<type> svf;
public:
    inline type const pass() const {
        return svf.hp;
    }
    inline void process(const type x) {
        svf.process(x);
    };
    inline void reset() {
        svf.reset();
    };
    inline void setparams(type k, type q, type sr) {
        svf.setparams(k, q, sr);
    };    
};

template<typename type>
struct SVFilterAdapter<filters::bandpass, type>
{
private:
    SVFilter<type> svf;
public:
    inline type const pass() const {
        return svf.bp;
    }
    inline void process(const type x) {
        svf.process(x);
    };
    inline void reset() {
        svf.reset();
    };
    inline void setparams(type k, type q, type sr) {
        svf.setparams(k, q, sr);
    };    
};

template<typename type>
struct SVFilterAdapter<filters::bandreject, type>
{
private:
    SVFilter<type> svf;
public:
    inline type const pass() const {
        return svf.br;
    }
    inline void process(const type x) {
        svf.process(x);
    };
    inline void reset() {
        svf.reset();
    };
    inline void setparams(type k, type q, type sr) {
        svf.setparams(k, q, sr);
    };    
};

template<typename type>
struct SVFilterAdapter<filters::allpass, type>
{
private:
    SVFilter<type> svf;
public:
    inline type const pass() const {
        return svf.ap;
    }
    inline void process(const type x) {
        svf.process(x);
    };
    inline void reset() {
        svf.reset();
    };
    inline void setparams(type k, type q, type sr) {
        svf.setparams(k, q, sr);
    };    
};
