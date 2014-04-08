#ifndef UNFOLD_SPECTRUM_HPP
#define UNFOLD_SPECTRUM_HPP

#include "matrix.hpp"
#include <vector>
#include <cassert>

/** \brief A spectrum; like a pair (vector, Matrix) of measured values and their covariance matrix
 */
class Spectrum {
public:
    
    explicit Spectrum(size_t n = 0): elems(n, 0.0), cov_(n, n){
    }
    
    explicit Spectrum(const std::vector<double> & values): elems(values), cov_(elems.size(), elems.size()){}
    
    void set_zero(){
        fill(elems.begin(), elems.end(), 0.0);
        cov_.set_zero();
    }
    
    // dimension
    size_t dim() const{
        return elems.size();
    }
    
    // the covariance matrix element for i,j.
    Matrix & cov() {
        return cov_;
    }
    
    const Matrix & cov() const {
        return cov_;
    }
    
    // the measurement in bin i
    double & operator[](size_t i){
        assert(i < this->elems.size());
        return elems[i];
    }
    
    double operator[](size_t i) const {
        return const_cast<Spectrum*>(this)->operator[](i);
    }
    
    const std::vector<double> & get_values() const{
        return elems;
    }
    
    // calculate
    //    result[i] = coeff_this * this[i] + coeff * rhs[i]
    // and return the result.
    // Assumes that there is NO correlation between own entries and entries
    // of the other measurement!
    Spectrum add(double coeff_this, double coeff, const Spectrum & rhs) const;
    
    void operator+=(const Spectrum & rhs){
        *this = add(1.0, 1.0, rhs);
    }
    
    void operator-=(const Spectrum & rhs){
        *this = add(1.0, -1.0, rhs);
    }
    
    // set the covariance to a diagonal Poisson (sqrt(n)) covariance matrix
    void set_poisson_covariance();
    
private:
    std::vector<double> elems;
    Matrix cov_;
};

std::ostream & operator<<(std::ostream & out, const Spectrum & s);

inline Spectrum operator+(const Spectrum & rhs, const Spectrum & lhs){
    Spectrum result(rhs);
    result += lhs;
    return result;
}

inline Spectrum operator-(const Spectrum & rhs, const Spectrum & lhs){
    Spectrum result(rhs);
    result -= lhs;
    return result;
}

// NOTE: does NOT set the covariance of the result
Spectrum operator*(const Matrix & R, const Spectrum & t);


#endif
