#include "regunfolder.hpp"
#include "stat.hpp"
#include "root.hpp"
#include "poly.hpp"
#include "utils.hpp"

#include "base/include/ptree-utils.hpp"

#include "TGraph.h"


using namespace std;

RegUnfolder::RegUnfolder(const Problem & p_, const ptree & cfg): p(p_), tau(-1.0), tau_min(-1.0){
    tau_cfg = cfg.get_optional<double>("tau");
    if(tau_cfg) tau = *tau_cfg;

    string regtype = ptree_get<string>(cfg, "regtype", "curvature");
    string s_tau_scanmethod = ptree_get<string>(cfg, "tau-scanmethod", "rho-amean");
    if(s_tau_scanmethod=="rho-amean"){
        tauscan = ts_rho_amean;
    }
    else if(s_tau_scanmethod=="rho-gmean"){
        tauscan = ts_rho_gmean;
    }
    else if(s_tau_scanmethod=="rho-max"){
        tauscan = ts_rho_max;
    }
    else if(s_tau_scanmethod=="disable"){
        tauscan = ts_disable;
    }
    else if(s_tau_scanmethod=="all"){
        tauscan = ts_all;
    }
    else{
        throw runtime_error("invalid setting tau-scanmethod=" + s_tau_scanmethod);
    }

    if(regtype=="curvature"){
        // set L:
        size_t ngen = p.R().get_n_cols();
        L = Matrix(ngen, ngen);
        for(size_t i=0; i<ngen; ++i){
            for(size_t j=0; j<ngen; ++j){
                if(i==j){
                    L(i,j) = -2;
                }
                else if(abs(i - j)==1){ 
                    L(i,j) = 1;
                    // TODO: boundary?!
                }
            }
        }
    }
    else{
        throw invalid_argument("only regtype=curvature is supported");
    }
    size_t ngen = p.gen().dim();
    // a column vector for the sought-after true spectrum t:
    t = poly_matrix("t", ngen, 1);
    delta2 = L * (t - poly_matrix(p.gen().get_values()));
}

Spectrum RegUnfolder::unfold(const Spectrum & r) const {
    Spectrum result = unfold_nouncertainty(r);
    propagate_uncertainty(result, r, bind(&RegUnfolder::unfold_nouncertainty, this, _1));
    return result;
}

void RegUnfolder::do_regularization_scan(TDirectory & out){
    if(tauscan == ts_disable) return;
    if(p.R().is_diagonal()){
        cerr << "NOTE: response matrix is diagonal; it does not make sense to perform regularization scan." << endl;
        tau = 0.0;
        return;
    }
    
    Spectrum r = p.mean_reco_bkgsub();
    if(tauscan == ts_rho_amean || tauscan == ts_all){
        auto f_tau = [&](double t) -> double {tau=t; return amean(rho(unfold(r).cov()));};
        save_values sv(f_tau);
        tau_min = find_xmin(ref(sv));
        unique_ptr<TGraph> tg = make_tgraph(sv.fvals(), "tau_scan_amean");
        put(out, move(tg));
    }
    
    if(tauscan == ts_rho_gmean){
        auto f_tau = [&](double t) -> double {tau=t; return gmean(rho(unfold(r).cov()));};
        save_values sv(f_tau);
        tau_min = find_xmin(ref(sv));
        unique_ptr<TGraph> tg = make_tgraph(sv.fvals(), "tau_scan_gmean");
        put(out, move(tg));
    }
    
    if(tauscan == ts_rho_max || tauscan == ts_all){
        auto f_tau = [&](double t) -> double {tau=t; return max(rho(unfold(r).cov()));};
        save_values sv(f_tau);
        tau_min = find_xmin(ref(sv));
        unique_ptr<TGraph> tg = make_tgraph(sv.fvals(), "tau_scan_max");
        put(out, move(tg));
    }
}

Spectrum RegUnfolder::unfold_nouncertainty(const Spectrum & r) const{
    if(tau < 0.0 || L.get_n_rows() != p.gen().dim()) throw invalid_argument("unfold called, but regularization not set up!");
    const size_t ngen = p.gen().dim();
    
    // calculate delta1 := R * t - r for the first term:
    auto delta1 = p.R() * t - poly_matrix(r.get_values());
    
    // calculate delta2 := L * (t - gen) for the second term
    //auto delta2 = L * (t - poly_matrix(p.gen().get_values()));

    // chi2 = delta1^T C^{-1} delta1 + tau * delta2^T * delta2,
    // where C is the covariance matrix of r:
    Matrix r_cov_inverse = r.cov();
    r_cov_inverse.invert_cholesky();
    auto chi2 = delta1.transpose() * r_cov_inverse * delta1 + tau * delta2.transpose() * delta2;
    
    auto solution = chi2(0,0).argmin();
    
    Spectrum result(ngen);
    for(size_t i=0; i<ngen; ++i){
        result[i] = solution[t(i,0)];
    }
    return result;
}


REGISTER_UNFOLDER(RegUnfolder);
