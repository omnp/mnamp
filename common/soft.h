#include <cmath>
#include <cstdint>

template<typename type> type f(type const x, type const a, type const b) {
    /** Amplify with proportional pre-bias */
    type y = a*(x + std::abs(x)*b);
    return y;
}

template<typename type> class Soft {
    /** A sort of soft clipper. */
public:
    Soft(uint32_t const iterations = 56u) {
        y = dy = ddy = 0.0;
        this->iterations = iterations;
    }
    void set_iterations(uint32_t const iterations) {
        this->iterations = iterations;
    }
    type operator()(type const x, type a, type b) {
        return diff(x, a, b);
    }
protected:
    const uint32_t bits = 16u; /*16-bit ready*/
    const type quantum = 1.0/(1 << (bits - 1u));
    type diff(type x, type a, type b) {
        uint32_t k = 1u;
        type dy = f(x, a, b * (std::abs(this->y))) - this->y;
        type ddy = dy - this->dy;
        type z = dy * 0.5;
        while (std::abs(this->y + dy) >= 1.0) {
            if (std::abs(dy - z) > 4.0*quantum) {
                dy -= z;
            }
            else {
                dy = z - dy;
            }
            z *= 0.5;
            ddy = dy - this->dy;
            k += 1;
            if (k > iterations) {
                break;
            }
        }
        this->y = this->y + dy;
        this->dy = dy;
        this->ddy = ddy;
        return this->y;
    }
private:
    type y;
    type dy;
    type ddy;
    uint32_t iterations;
};
