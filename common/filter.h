#pragma once

#include <cstdint>
#include <list>

namespace filters
{
    enum filter_responses {highpass, lowpass, bandpass, bandreject, allpass};
}

template <typename type> class averaging
{
public:
    inline type const pass() const {
        return state;
    }
    inline void process(const type x) {
        state = 0.5 * (state + x);
    }
    inline void reset() {
        state = 0.0;
    }
private:
    type state{0.0};
};

template <typename type, typename filter_concrete> class filter_parameters;

template <typename type, class deriving> class filter
{
public:
    inline type const pass() const {
        return deriving::pass();
    }
    inline void process(const type x) {
        deriving::process(x);
    };
    inline void reset() {
        deriving::reset();
    };
    inline void setparams(type k, type q, type sr) {
        deriving::setparams(k, q, sr);
    };
};

template <filters::filter_responses filter_response, typename type, class filter_type> class filter_adapter : public filter<type, filter_adapter<filter_response, type, filter_type>>
{
private:
    explicit filter_adapter() = 0;
};

template<typename type, typename filter_type, const uint32_t number>
struct filter_cascade : public filter<type, filter_cascade<type, filter_type, number>>
{
public:
    inline type const pass() const {
        return filters[number-1u].pass();
    }
    void process(const type x) {
        type s = x;
        for (uint32_t i = 0; i < number; i++) {
            filters[i].process(s);
            s = filters[i].pass();
        }
    }
    void reset() {
        for (uint32_t i = 0; i < n; i++) {
            filters[i].reset();
        }
    }
    void setparams(const type k, const type q, const type sr) {
        for (uint32_t i = 0; i < number; i++) {
            filters[i].setparams(k, q, sr);
        }
    }
private:
    const uint32_t n{number};
    filter_type filters[number];
};

template <typename type, typename filter_concrete> class filter_parameters
{
private:
    type k,q,sr;
    std::list<filter_concrete *> filters;
public:
    explicit filter_parameters() : k{0.49},q{0.5},sr{1.0} {};
    void append(filter_concrete & f) {
        filters.push_back(&f);
        f.setparams(k,q,sr);
    }
    void setparams(type k, type q, type sr) {
        this->k = k;
        this->q = q;
        this->sr = sr;
        for (auto f: filters)
            f->setparams(k, q, sr);
    }
};

template <typename type, typename filter_concrete> class filter_cascade_parameters : public filter_parameters<type, filter_concrete>
{
private:
    const uint32_t order;
    explicit filter_cascade_parameters(const uint32_t order) : order{order} {}
};
