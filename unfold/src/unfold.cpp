#include "unfold.hpp"
#include "root.hpp"
#include "base/include/ptree-utils.hpp"

#include "TFile.h"
#include "TH1D.h"
#include "TH2D.h"

using namespace std;

Unfolder::~Unfolder(){}
Module::~Module(){}

Problem::Problem(TFile & infile, const ptree & cfg){
    auto prefix = ptree_get<string>(cfg, "prefix", "");
    
    auto hresponse = gethisto<TH2D>(infile, prefix + ptree_get<string>(cfg, "response", "response"));
    bool transpose = ptree_get<bool>(cfg, "transpose", false);
    tie(R_, R_uncertainties_) = roothist_to_matrix(*hresponse, transpose);
    
    auto hgen = gethisto<TH1D>(infile, prefix + ptree_get<string>(cfg, "gen", "gen"));
    gen_ = roothist_to_spectrum(*hgen);
    
    
    bool bkg_use_uncertainties = ptree_get<bool>(cfg, "bkg-use-uncertainties", false);
    // background is optional. The rule is that:
    // 1. if 'bkg' is given, use that; in this case, do not guess cov histogram
    // 2. if 'bkg' is not given, try the standard names
    boost::optional<string> bkg_hname = cfg.get_optional<string>("bkg");
    if(bkg_hname){
        auto hbkg = gethisto<TH1D>(infile, prefix + *bkg_hname);
        auto bkg_cov_hname = cfg.get_optional<string>("bkg-cov");
        std::unique_ptr<TH2D> hbkg_cov;
        if(bkg_cov_hname){
            hbkg_cov = gethisto<TH2D>(infile, prefix + *bkg_cov_hname);
        }
        bkg_ = roothist_to_spectrum(*hbkg, hbkg_cov.get(), bkg_use_uncertainties);
    }
    else{
        std::unique_ptr<TH1D> hbkg;
        std::unique_ptr<TH2D> hbkg_cov;
        try{
            hbkg = gethisto<TH1D>(infile, prefix + "bkg");
            hbkg_cov = gethisto<TH2D>(infile, prefix + "bkg_cov");
        }
        catch(runtime_error & ){
            // histogram not found: ignore
        }
        if(hbkg){
            bkg_ = roothist_to_spectrum(*hbkg, hbkg_cov.get(), bkg_use_uncertainties);
        }
        else{
            // empty background:
            bkg_ = Spectrum(R_.get_n_rows());
        }
    }
    check();
}


Problem::Problem(const Matrix & R, const Matrix & R_uncertainties, const Spectrum & gen, const Spectrum & bkg): R_(R), R_uncertainties_(R_uncertainties), gen_(gen), bkg_(bkg){
    check();
}

void Problem::check(){
    const size_t nreco = R_.get_n_rows();
    const size_t ngen = R_.get_n_cols();
    // check dimensions of all other objects:
    if(R_uncertainties_.get_n_rows() != nreco || R_uncertainties_.get_n_cols() != ngen) throw runtime_error("inconsistent dimensions of R and R_uncertainties");
    if(gen_.dim() != ngen) throw runtime_error("inconsistent dimensions of R and gen");
    if(bkg_.dim() != nreco) throw runtime_error("inconsistent dimensions of R and bkg");
    // check contents of response matrix: for each gen bin, sum over reco should be <= 1.0:
   for(size_t i=0; i<R_.get_n_cols(); ++i){ // gen index
       double ptot = 0.0;
       for(size_t j=0; j<R_.get_n_rows(); ++j){ // reco index
           ptot += R_(j,i);
       }
       if(ptot - 1.0 > 1e-7){
           throw runtime_error("response matrix total probability > 1!");
       }
   }
}

Spectrum unfold_full(const Unfolder & unf, const Spectrum & bkg, const Spectrum & reco){
    Spectrum bkgsub(reco);
    bkgsub.set_poisson_covariance();
    bkgsub -= bkg;
    return unf.unfold(bkgsub);
}


void propagate_uncertainty(Spectrum & y, const Spectrum & x, const std::function<Spectrum (const Spectrum &)> & f){
    const double kappa = 0.1; // 'step-size' for the linearization evaluation
    
    const size_t nx = x.dim(), ny = y.dim();
    
    // get A with
    // y_i = A_ik x_k
    Matrix A(ny, nx);
    for(size_t ix=0; ix < nx; ++ix){
        Spectrum x_shifted(x);
        double dx = kappa * sqrt(x.cov()(ix, ix));
        // if dx is 0, it means the error on x_i is zero, so it does not matter what's in the matrix anyway.
        if(dx==0.0) continue;
        x_shifted[ix] += dx;
        Spectrum y_shifted = f(x_shifted);
        for(size_t iy=0; iy<ny; iy++){
            A(iy, ix) = (y_shifted[iy] - y[iy]) / dx;
        }
    }
    
    // now set the error on y:
    // Cy_ij = A_il  Cx_lk  A_jk
    Matrix & Cy = y.cov();
    Cy.set_zero();
    const Matrix & Cx = x.cov();
    for(size_t i=0; i<ny; ++i){
        for(size_t j=0; j<ny; ++j){
            for(size_t l=0; l<nx; ++l){
                for(size_t k=0; k<nx; ++k){
                    Cy(i,j) += A(i,l) * Cx(l,k) * A(j,k);
                }
            }
        }
    }
}
