#ifndef UNFOLD_UNFOLD_HPP
#define UNFOLD_UNFOLD_HPP

#include "base/include/registry.hpp"
#include "fwd.hpp"
#include "matrix.hpp"
#include "spectrum.hpp"

#include <boost/property_tree/ptree.hpp>

using boost::property_tree::ptree;

/** \brief Abstract base class for an unfolding algorithm
 *
 * At this level, the algorithm does not know about background subtraction: the input
 * for unfolding at this point is a measured spectrum with full statistical+systematic covariance matrix.
 * 
 * The result is the unfolded spectrum with all uncertainties included in the measured spectrum included.
 * 
 * To unfold a spectrum with background, use the free function 'unfold_full' which can be used
 * with any Unfolder.
 */ 
class Unfolder {
public:
    
    // after the setup, perform a regularization scan. What this means exactly
    // (or whether this is implemented at all) depends on the used algorithm. Typically, the regularization parameter
    // is varied (using the information from the setup) to minimize some risk function.
    virtual void do_regularization_scan(TDirectory & out) {}
    
    virtual Spectrum unfold(const Spectrum & measured_distribution) const = 0;
    
    virtual ~Unfolder();
};

typedef Registry<Unfolder, std::string, const Problem&, const ptree&> UnfolderRegistry;

#define REGISTER_UNFOLDER(T) namespace { int dummy##T = ::UnfolderRegistry::register_<T>(#T); }


/** \brief Perform background subtraction and unfolding
 * 
 * Background is subtracted from the spectrum 'reco' and passed to the Unfolder 'unf'.
 * Poisson uncertainties for reco are assumed; the error set on reco is ignored(!).
 */
Spectrum unfold_full(const Unfolder & unf, const Spectrum & bkg, const Spectrum & reco);


/** \brief Propagatation of uncertainty of a Spectrum-valued function
 *
 * Modifies the covariance matrix of y according to the error propagation formulae with y = f(x). Note that
 * the central values for y are not modified by this function, they have to be set correctly before calling this function.
 * 
 * The error propagation is performed by calling the function f exactly x.dim() times, each time with a slightly
 * different x to get an approximate linearization of f around x. With the Hesse matrix A, f is given approximately by 
 * 
 *  f_i = A_ij x_j  + c   (sum over j implied)
 * 
 * where c is a constant which does not matter. Then, the covariance matrix for y is
 * 
 * Cy_ij = A_il  Cx_lk  A_jl  (sum over l,k implied)
 * 
 * where Cx is the covariance matrix of x.
 */
void propagate_uncertainty(Spectrum & y, const Spectrum & x, const std::function<Spectrum (const Spectrum &)> & f);

class Module {
public:
    // constructor arguments: const ptree& cfg, TFile& in, TDirectory& out
    
    explicit Module(TDirectory & out__): out_(out__){}
    
    TDirectory & out(){
        return out_;
    }
    
    // put the histogram in the output directory and destroy it (!)
    //void put(TH1 * h);
    
    virtual void run(const Problem& p, const Unfolder & unf) = 0;
    virtual ~Module();
    
private:
    TDirectory & out_;
};

typedef Registry<Module, std::string, const ptree&, TFile&, TDirectory&> ModuleRegistry;

#define REGISTER_MODULE(T) namespace { int dummy##T = ::ModuleRegistry::register_<T>(#T); }

// unfolding problem. Corresponds to the
// configuration file entry 'problem'; see description there to get started.
class Problem {
public:
    Problem(TFile & infile, const ptree & cfg);
    
    Problem(const Matrix & R, const Matrix & R_uncertainties, const Spectrum & gen, const Spectrum & bkg);    
    
    // the response matrix
    const Matrix & R() const{
        return R_;
    }
    
    // the response matrix bin-by-bin errors:
    const Matrix & R_uncertainties() const{
        return R_uncertainties_;
    }
    
    // gen-level spectrum, without cuts. Note that its cov is identical zero.
    const Spectrum & gen() const{
        return gen_;
    }
    
    // sum of all backgrounds including full covariance of MC stat and syst uncertainties (but without Poisson errors)
    const Spectrum & bkg() const{
        return bkg_;
    }
    
    // expected ('Asimov') background-subtracted reconstructed spectrum. Same as:
    //   (R * gen + bkg) - bkg
    // where independent Poisson uncertainties are assumed for the (R*gen + bkg) part and the full
    // background covariance is used for the '- bkg' part
    Spectrum mean_reco_bkgsub() const {
        Spectrum result = R_ * gen_ + bkg_;
        result.set_poisson_covariance();
        result -= bkg_;
        return result;
    }
    
private:
    Matrix R_, R_uncertainties_;
    Spectrum gen_, bkg_;
    
    void check();
};

#endif
