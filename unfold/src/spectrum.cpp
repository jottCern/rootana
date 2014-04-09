#include "spectrum.hpp"

#include <stdexcept>
#include <cmath>
#include <iostream>

using namespace std;

void Spectrum::set_poisson_covariance(){
    const size_t n = elems.size();
    for(size_t i=0; i<n; ++i){
        for(size_t j=0; j<n; ++j){
            cov_(i,j) = i==j ? elems[i] : 0.0;
        }
    }
}

Spectrum Spectrum::add(double coeff_this, double coeff, const Spectrum & rhs) const {
    assert(elems.size()==rhs.elems.size());
    const size_t n = elems.size();
    Spectrum result(n);
    for(size_t i=0; i<n; ++i){
        result.elems[i] = coeff_this * elems[i] + coeff * rhs.elems[i]; 
    }
    const double ct2 = coeff_this * coeff_this;
    const double c2 = coeff * coeff;
    for(size_t i=0; i<n; ++i){
        for(size_t j=0; j<n; ++j){
            result.cov_(i,j) = ct2 * cov_(i,j) + c2 * rhs.cov_(i, j);
        }
    }
    return result;
}

Spectrum operator*(const Matrix & R, const Spectrum & t){
    const size_t ngen = R.get_n_cols();
    const size_t nrec = R.get_n_rows();
    Spectrum result(nrec);
    if(t.dim() != ngen){
        throw runtime_error("response * gen: wrong dimensions");
    }
    for(size_t i=0; i<nrec; ++i){
        for(size_t j=0; j<ngen; ++j){
            result[i] += R(i,j) * t[j];
        }
    }
    return result;
}

std::ostream & operator<<(std::ostream & out, const Spectrum & s){
    out << "(";
    const size_t n = s.dim();
    for(size_t i=0; i<n; ++i){
        out << s[i] << " +- " << sqrt(s.cov()(i,i)) << ((i==n-1)?"":", ");
    }
    out << ")";
    return out;
}
