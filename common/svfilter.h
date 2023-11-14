#pragma once

#include "math.h"

template<typename type>
struct Filter
/* This is a filter implemented from the following paper:
 * "Improving the Chamberlin Digital State Variable Filter"
 * by Victor Lazzarini, Joseph Timoney
 * https://arxiv.org/abs/2111.05592v2
 */
{
public:
    type s1,s2;
    type hp,bp,lp,br,ap;
    
    Filter() : s1(0.0f), s2(0.0f), hp(0.0f), bp(0.0f), lp(0.0f), br(0.0f), ap(0.0f)
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

enum filter_type {highpass, lowpass, allpass};
template<typename type, uint32_t N>
struct FilterCascade
{
public:
    type pass;
    filter_type function;

    FilterCascade(filter_type const function = lowpass) : function(function) {}

    void setparams(const type k, const type q, const type sr) {
        for (uint32_t i = 0; i < n; i++) {
            filters[i].setparams(k, q, sr);
        }
    }

    void process(const type x) {
        type s = x;
        for (uint32_t i = 0; i < n; i++) {
            filters[i].process(s);
            switch (function) {
                case allpass:
                    s = filters[i].ap;
                    break;
                case highpass:
                    s = filters[i].hp;
                    break;
                default:
                case lowpass:
                    s = filters[i].lp;
                    break;
            }
        }
        pass = s;
    }

    void reset() {
        for (uint32_t i = 0; i < n; i++) {
            filters[i].reset();
        }
        pass = 0.0;
    }

private:
    const uint32_t n{N};
    Filter<type> filters[N];
};
