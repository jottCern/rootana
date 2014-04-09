#ifndef UNFOLD_REGUNFOLDER_HPP
#define UNFOLD_REGUNFOLDER_HPP

#include "unfold.hpp"
#include "poly.hpp"

/** \brief Regularized unfolding
 *
 * Regularized unfolding finds the minimum of 
 *  f(t) = (R * t - r)^T C_r^-1 (R * t - r)   +   tau *  (L * (t - t0))^T (L * (t - t0))
 * 
 * where t is the true-level spectrum, R is the response matrix, r is the (background-subtracted) reco-level spectrum,
 * L is the regularization matrix, t0 is the nominal gen-level spectrum, and tau the regularization strength.
 * 
 * Depending on how L is chosen, different deviations of the spectrum to the nominal one are constrained. A standard
 * choice is to make L calculate the second derivatives such that deviations with large curvature are disfavored.
 * 
 * The tau 
 * 
 * Configuration:
 * \code
 * type RegUnfolder
 * regtype curvature ; optional
 * tau-scanmethod rho-amean ; optional
 * tau 1e-6 ; optional
 * \endcode
 * 
 * \c regtype controls the form of the regularization matrix L. The only value supported so far is "curvature".
 * 
 * \c tau-scanmethod is the method used to scan for tau. Possible values are:
 *    - "rho-amean" use the value for tau which yields minimum arithmetic mean for the global correlation vector rho
 *    - "rho-gmean": same as "rho-amean", but use geometric mean instead
 *    - "rho-max": minimize the use maximum entry in the rho vector
 *    - "disable": do not scan for tau; in this case, you have to specify a value
 * 
 * \c tau if given, use this value for the unfolding; the scan is still done but the result is ignored for the unfolding.
 */  
class RegUnfolder: public Unfolder{
public:
    RegUnfolder(const Problem & p, const ptree & cfg);
    
    virtual void do_regularization_scan(TDirectory & out);
    
    virtual Spectrum unfold(const Spectrum & measured_distribution) const;
    
private:
    Spectrum unfold_nouncertainty(const Spectrum & m) const;
    
    // config parameters:
    Matrix L;
    enum e_tauscan { ts_rho_amean, ts_rho_gmean, ts_rho_max, ts_all, ts_disable };
    e_tauscan tauscan;
    boost::optional<double> tau_cfg;
    
    const Problem & p;
    
    // tau value to use for the next call to 'unfold':
    double tau;
    
    // value of tau found in the regularization scan (or -1.0 if no scan is done):
    double tau_min;

    poly_matrix t, delta2;
};

#endif

