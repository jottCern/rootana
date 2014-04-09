#include "stat.hpp"

using namespace std;
    
Matrix MeanCov::cov() const {
    Matrix result(npar, npar);
    for (size_t i = 0; i < npar; i++) {
        for (size_t j = i; j < npar; j++) {
            result(j, i) = result(i, j) = count_covariance(i,j) / count;
        }
    }
    return result;
}
    
    

void MeanCov::update(const Spectrum & d, size_t weight){
    assert(d.dim() == npar);
    double factor = count * 1.0 / (count + 1);
    for(size_t i=1; i<weight; ++i){
        factor += (count*count*1.0) / ((count+i) * (count+i+1));
    }
    for(size_t i=0; i<npar; ++i){
        if(!isfinite(d[i])){
            throw runtime_error("MeanCov::update: Spectrum has non-finite entries");
        }
        double diff_i = d[i] - means_[i];
        for(size_t j=i; j<npar; ++j){
            double diff_j = d[j] - means_[j];
            count_covariance(i,j) += factor * diff_i * diff_j;
        }
        //the old means[i] is now no longer needed, as j>=i ...
        means_[i] += weight * 1.0 / (count + weight) * diff_i;
    }
    count += weight;
}

std::vector<double> rho(const Matrix & V){
    const size_t n= V.get_n_rows();
    Matrix Vinv(V);
    Vinv.invert_cholesky();
    vector<double> result(n);
    for(size_t i=0; i<n; ++i){
        result[i] = sqrt(1.0 - 1.0 / (V(i,i) * Vinv(i,i)));
    }
    return result;
}
