#ifndef UNFOLD_STAT_HPP
#define UNFOLD_STAT_HPP

#include "matrix.hpp"
#include "unfold.hpp"

#include <algorithm>

// calculate sliding mean and covariance from a number of spectra
class MeanCov {
public:
    
    explicit MeanCov(size_t n): npar(n), count(0), means_(npar, 0.0), count_covariance(npar, npar){}
    
    Matrix cov() const;
    
    const std::vector<double> & means() const{
        return means_;
    }
    
    Spectrum spectrum() const{
        Spectrum result(means());
        result.cov() = cov();
        return result;
    }
    
    void update(const Spectrum & s, size_t weight = 1);
        
private:
    size_t npar;
    size_t count;
    std::vector<double> means_;
    Matrix count_covariance;
};


// aithmetic mean
inline double amean(const std::vector<double> & vs){
    double sum = std::accumulate(vs.begin(), vs.end(), 0.0);
    return sum / vs.size();
}

// geometric mean
inline double gmean(const std::vector<double> & vs){
    double prod = std::accumulate(vs.begin(), vs.end(), 1.0, [](double a, double b){return a*b; });
    return pow(prod, 1.0 / vs.size());
}

// maximum
inline double max(const std::vector<double> & vs){
    return std::accumulate(vs.begin(), vs.end(), -std::numeric_limits<double>::infinity(), [](double a, double b){return std::max(a,b);});
}

// get the vector of global correlation coefficients rho_j with
//   rho_j = sqrt(1 - (V_jj * V^{-1}_jj)^-1)
// where V is the covariance matrix.
std::vector<double> rho(const Matrix & V);

#endif
