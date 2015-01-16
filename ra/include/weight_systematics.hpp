#ifndef RA_WEIGHT_SYSTEMATICS_HPP
#define RA_WEIGHT_SYSTEMATICS_HPP

#include "identifier.hpp"
#include <tuple>
#include <vector>
#include <stdexcept>

/* This file defines data structures to be used for encoding
 * systematics on the event weight, such as lepton id efficiency,
 * pileup weight, pdf weights, top pt reweighting, etc.
 * 
 * A general way to encode such systematics is to save the dependence of the scale factor for
 * the nominal event weight as a function of a nuisances parameter which has a normal prior with
 * mean 0 and standard deviation 1.
 * For example, to encode the uncertainty from B tagging efficiency, one can save the
 * event weight as a function of the nuisance parameter "SFBsyst" where the scale factor for the event weight
 * at SFBsyst=-1 is obtained by evaluating the -1sigma value for the scale factor, and at SFBsyst=+1 for the +1sigma variation.
 * (note that as the value is the *additional* event weight (i.e. in addition to the nominal correction),
 * the weight at parameter value 0 will always be 1.)
 * 
 * While this general concept suggests a class interface for setting the scale factor for arbitrary
 * parameter values, only +-1 is usually used. Therefore, only this case is implemented (although it could
 * be extended if needed in the future).
 */
namespace ra {

class weight_systematics {
public:
    void set(const identifier & syst_par, float minus_weight_factor, float plus_weight_factor){
        all_syst.emplace_back(syst_par, minus_weight_factor, plus_weight_factor);
    }
    
    // parameter name, sf at -1.0, sf at +1.0
    typedef std::tuple<identifier, float, float> single_syst;

    const std::vector<single_syst> & get_systs(const identifier & syst_par) const{
        return all_systs;
    }
    
private:
    
    std::vector<single_syst> all_syst;
};

}

#endif
