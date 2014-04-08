#include "unfold.hpp"
#include "root.hpp"
#include "TUnfold.h"
#include "utils.hpp"

using namespace std;

class TUnfolder: public Unfolder{
public:
    TUnfolder(const Problem & p, const ptree & cfg);
    
    virtual void do_regularization_scan(TDirectory & out);
    
    virtual Spectrum unfold(const Spectrum & measured_distribution) const;
    virtual ~TUnfolder(){}
    
private:
    
    // config parameters:
    enum e_tauscan { ts_rho_amean, ts_rho_gmean, ts_rho_max, ts_all, ts_disable };
    e_tauscan tauscan;
    boost::optional<double> tau_cfg;    
    
    // tau value to use for the next call to 'unfold':
    double tau;
    
    double tau_min; // value found by scan
    
    const Problem & p;
    unique_ptr<TUnfold> tunfold;
};


void TUnfolder::do_regularization_scan(TDirectory & out){
    if(p.R().is_diagonal()){
        cerr << "NOTE: response matrix is diagonal; it does not make sense to perform regularization scan." << endl;
        tau = 0.0;
        return;
    }
    
    Spectrum r = p.mean_reco_bkgsub();
    r.set_poisson_covariance();
    auto rhist = spectrum_to_roothist(r, "measured", true);
    auto rho_tau = [&](double t) -> double {return tunfold->DoUnfold(t, rhist.get());};
    save_values sv(rho_tau);
    tau_min = find_xmin(ref(sv));
    unique_ptr<TGraph> tg = make_tgraph(sv.fvals(), "tau_scan");
    put(out, move(tg));
    
    if(!tau_cfg){
        tau = tau_min;
        cout << "Using tau = " << tau << endl;
    }
    
    unique_ptr<TH1D> hbias(tunfold->GetBias("bias", "bias"));
    put(out, move(hbias));
}

TUnfolder::TUnfolder(const Problem & p_, const ptree & cfg): tau(-1.0), p(p_){
    tau_cfg = cfg.get_optional<double>("tau");
    if(tau_cfg) tau = *tau_cfg;
    
    const Matrix & R = p.R();
    const Matrix & R_uncertainties = p.R_uncertainties();
    const int nreco = R.get_n_rows();
    const int ngen = R.get_n_cols();
    
    // fill A matrix for TUnfold: the 2D spectrum gen versus reco, scaled to the prediction
    // y-axis is gen, x-axis is reco
    const Spectrum & gen = p.gen();
    unique_ptr<TH2D> A(new TH2D("A", "A", nreco, 0, nreco, ngen, 0, ngen));
    for(int i=1; i<=nreco; ++i){
        for(int j=1; j<=ngen; ++j){
            A->SetBinContent(i, j, R(i-1, j-1) * gen[j-1]);
            A->SetBinError(i, j, R_uncertainties(i-1, j-1) * gen[j-1]);
        }
    }
    // add gen-but-not-reco events in overflow bin of A:
    for(int j=1; j<=ngen; ++j){
        // total number of events for this bin in gen, as currently in A:
        double gen_tot = 0.0;
        for(int i=1; i<=nreco; ++i){
            gen_tot += A->GetBinContent(i, j);
        }
        // the rest is in the overflow bin:
        A->SetBinContent(nreco + 1, j, max(0.0, gen[j-1] - gen_tot));
    }
    
    /*for(int i=1; i<=nreco+1; ++i){
        for(int j=1; j<=ngen; ++j){
            cout << "A(rec=" << i << ",gen=" << j << ") = "<< A->GetBinContent(i, j) << endl;
        }
    }*/
    
    tunfold.reset(new TUnfold(A.get(), TUnfold::kHistMapOutputVert, TUnfold::kRegModeCurvature, TUnfold::kEConstraintNone));
    
    // set bias distribution to gen:
    //auto genhist = spectrum_to_roothist(gen, "gen");
    //tunfold->SetBias(genhist.get());
}

Spectrum TUnfolder::unfold(const Spectrum & measured_distribution) const{
    if(tau < 0.0) throw runtime_error("invalid value for tau (regularization not set up?)");
    // FIXME: use covariance of measured_distribution!
    Spectrum m(measured_distribution);
    m.set_poisson_covariance();
    auto mhist = spectrum_to_roothist(m, "measured", true);
    auto rho_max = tunfold->DoUnfold(tau, mhist.get(), 1.0);
    if(rho_max >= 1.0){
        throw runtime_error("call to TUnfold::DoUnfold failed");
    }
    unique_ptr<TH1D> hresult(tunfold->GetOutput("x", "x"));
    unique_ptr<TH2D> hcov(tunfold->GetEmatrix("cov", "cov"));
    return roothist_to_spectrum(*hresult, hcov.get());
}

REGISTER_UNFOLDER(TUnfolder)
