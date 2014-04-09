#include "unfold.hpp"

#include <vector>

using namespace std;

class BinByBin: public Unfolder{
public:
    BinByBin(const Problem & p, const ptree&);
    
    virtual Spectrum unfold(const Spectrum & measured_distribution) const{
        const size_t n = measured_distribution.dim();
        assert(factors.size() == n);
        Spectrum result(n);
        const Matrix & mcov = measured_distribution.cov();
        for(size_t i=0; i<n; ++i){
            result[i] = factors[i] * measured_distribution[i];
            if(result[i] < 0.0){
                throw invalid_argument("BinByBin: 'nominal' reconstructed is 0.0 in some bin (i.e. factor is undefined), but trying to unfold measured spectrum with entry !=0 in that bin.");
            }
            for(size_t j=0; j<n; ++j){
                result.cov()(i,j) = factors[i] * factors[j] * mcov(i,j);
            }
        }
        return result;
    }
    
private:    
    vector<double> factors; // factors required to go from reco to gen
};


BinByBin::BinByBin(const Problem & p, const ptree&){
    Spectrum r = p.mean_reco_bkgsub();
    const Spectrum & gen = p.gen();
    const size_t n = r.dim();
    factors.resize(n);
    for(size_t i=0; i<n; ++i){
        if(r[i] <= 0.0){
            if(gen[i] > 0.0){
                throw invalid_argument("Reconstructed spectrum is <= 0.0 but true is > 0.0. This is not allowed for BinByBin as the factor would be infinite.");
            }
            else{
                factors[i] = -1.0; // if the measured is 0.0 as well, this should not harm and lead to 0 in the unfolded spectrum. Negative entries are catched in the unfolding, see below.
            }
        }
        else{
            factors[i] = gen[i] / r[i];
        }
    }
}

REGISTER_UNFOLDER(BinByBin);

