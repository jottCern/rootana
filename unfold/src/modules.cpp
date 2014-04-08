#include "modules.hpp"
#include "root.hpp"
#include "random.hpp"
#include "stat.hpp"
#include "base/include/ptree-utils.hpp"

#include "TDirectory.h"
#include "TH1D.h"
#include "TH2D.h"

using namespace std;

unfold::unfold(const ptree& cfg, TFile & infile, TDirectory& out_): Module(out_){
    auto histo = gethisto<TH1D>(infile, ptree_get<string>(cfg, "hname"));
    reco = roothist_to_spectrum(*histo);
}

void unfold::run(const Problem& p, const Unfolder & unf){
    Spectrum unfolded = unfold_full(unf, p.bkg(), reco);
    auto hists = spectrum_to_roothists(unfolded, "unfolded", false);
    put(out(), move(hists.first));
    put(out(), move(hists.second));
}

REGISTER_MODULE(unfold);

toys::toys(const ptree& cfg, TFile & infile, TDirectory& out_): Module(out_){
    n = ptree_get<int>(cfg, "n", 1000);
    seed = ptree_get<uint64_t>(cfg, "seed", 0);
    if(seed == 0){
        seed = generate_seed();
    }
    all_hists = ptree_get(cfg, "all-hists", false);
    dice_stat = ptree_get(cfg, "dice-stat", true);
    dice_bkgsyst = ptree_get(cfg, "dice-bkgsyst", true);
    auto p_cfg = cfg.get_child_optional("problem");
    if(p_cfg){
        problem = Problem(infile, *p_cfg);
    }
    bool verbose = ptree_get(cfg, "verbose", false);
    if(verbose){
        // print configuration:
        cout << "Configured toys in output directory " << out_.GetName() << "; n=" << n << "; seed=" << seed
             << "; all-hists=" << all_hists << "; dice-stat=" << dice_stat << "; dice-bkgsyst=" << dice_bkgsyst
             << "; problem=" << (problem ? "configured" : "(not given, using global)") << endl;
    }
}

namespace {
    template<typename T>
    string str(T && t){
        return boost::lexical_cast<string>(forward<T>(t));
    }
    
    void save_as_th1(const Spectrum & s, const string & name, TDirectory & out, bool with_1d_errors = false, bool with_covariance = false){
        auto result = spectrum_to_roothists(s, name, with_1d_errors);
        put(out, move(result.first));
        if(with_covariance){
            put(out, move(result.second));
        }
    }
}

void toys::run(const Problem& p_global, const Unfolder & unf){
    const Problem & p = problem ? *problem : p_global;
    // if an alternative problem for toy generation has been set, make sure it matches the dimentionality of the global one
    if(p.R().get_n_rows() != p_global.R().get_n_rows() || p.R().get_n_cols() != p_global.R().get_n_cols()){
        throw runtime_error("problem for generating toys not compatible with global one");
    }
    size_t nrec = p.R().get_n_rows();
    size_t ngen = p.R().get_n_cols();
    
    // prepare output:
    // pull histograms for each bin on gen level:
    MeanCov pull_mc(ngen);
    // sum of the estimated covariance matrices, for "unfolded_cov_est_mean"
    Matrix sum_cov_est(ngen, ngen);
    
    // ensemble mean and covariance of the various intermediate histograms:
    MeanCov bkg_n_mc(nrec);
    MeanCov bkg_s_mc(nrec);
    MeanCov reco_n_mc(nrec);
    // ensemble mean and covariance of the unfolded spectra:
    MeanCov unfolded_mc(ngen);
    
    // run toys:
    Spectrum reco_mean = p.R() * p.gen();
    rnd_engine rnd;
    rnd.seed(seed);
    for(int i=0; i<n; ++i){
        // step 1: generate reco_n from reco_mean:
        Spectrum reco_n = randomize(rnd, reco_mean, false, dice_stat);
        if(all_hists) save_as_th1(reco_n, string("reco_n_") + str(i), out());
        reco_n_mc.update(reco_n);
        // step 2: reco-level background with systematics but no statistics:
        Spectrum bkg_s = randomize(rnd, p.bkg(), dice_bkgsyst, false);
        if(all_hists) save_as_th1(bkg_s, string("bkg_s_") + str(i), out());
        bkg_s_mc.update(bkg_s);
        // step 3: reco-level background with poisson:
        Spectrum bkg_n = randomize(rnd, bkg_s, false, dice_stat);
        if(all_hists) save_as_th1(bkg_n, string("bkg_n_") + str(i), out());
        bkg_n_mc.update(bkg_n);
        // step 4: unfold reco-level signal + background, using the global problem:
        Spectrum unfolded = unfold_full(unf, p_global.bkg(), bkg_n + reco_n);
        if(all_hists) save_as_th1(unfolded, string("unfolded_") + str(i), out());
        unfolded_mc.update(unfolded);
        
        Spectrum pulls = unfolded - p.gen();
        for(size_t i=0; i<ngen; ++i){
            pulls[i] /= sqrt(unfolded.cov()(i,i));
        }
        pull_mc.update(pulls);
        sum_cov_est += unfolded.cov();
    }
    
    // write output:
    save_as_th1(pull_mc.spectrum(), "pull", out(), true, false);
    save_as_th1(bkg_n_mc.spectrum(), "bkg_n", out(), true, true);
    save_as_th1(bkg_s_mc.spectrum(), "bkg_s", out(), true, true);
    save_as_th1(reco_n_mc.spectrum(), "reco_n", out(), true, true);
    save_as_th1(unfolded_mc.spectrum(), "unfolded", out(), true, true);
    
    
    std::unique_ptr<TH2D> unfolded_cov_est_mean(new TH2D("unfolded_cov_est_mean", "unfolded_cov_est_mean", ngen, 0, ngen, ngen, 0, ngen));
    for(size_t i=0; i<ngen; ++i){
        for(size_t j=0; j<ngen; ++j){
            unfolded_cov_est_mean->SetBinContent(i+1,j+1,sum_cov_est(i,j) / n);
        }
    }
    put(out(), move(unfolded_cov_est_mean));
}

REGISTER_MODULE(toys)


write_input::write_input(const ptree& cfg, TFile & infile, TDirectory& out): Module(out){}

void write_input::run(const Problem& p, const Unfolder & unf){
    save_as_th1(p.gen(), "gen", out());
    save_as_th1(p.bkg(), "bkg", out(), false, true);
    save_as_th1(p.mean_reco_bkgsub(), "mean_reco_bkgsub", out(), false, true);
    
}

REGISTER_MODULE(write_input)

namespace {

// get the derivative matrix A s.t. A_ik = d / dxk  f_i
// uses 1% of bin size as step size.
Matrix derivative(const function<Spectrum (Spectrum)> & f, const Spectrum & x){
   size_t n = x.dim();
   Spectrum y0 = f(x);
   Matrix result(n,n);
   for(size_t k=0; k<n; ++k){
       Spectrum x_shifted(x);
       double dx = 0.01 * fabs(x[k]);
       if(dx == 0.0) dx = 1.0;
       x_shifted[k] += dx;
       Spectrum y = f(x_shifted);
       for(size_t i=0; i<n; ++i){
           result(i,k) = (y[i] - y0[i]) / dx;
       }
   }
   return result;
}

}

lin_unfolding::lin_unfolding(const ptree & cfg, TFile & infile, TDirectory & out): Module(out){}

void lin_unfolding::run(const Problem & p, const Unfolder & unf){
    // The unfold function, taking the true histogram as input:
    auto unfold = [&](const Spectrum & x){Spectrum r = p.R() * x; r.set_poisson_covariance(); return unf.unfold(r);};
    Matrix A = derivative(unfold, p.gen());
    auto Ahist = matrix_to_roothist(A, "A");
    put(out(), move(Ahist));
}

REGISTER_MODULE(lin_unfolding)

ProblemReweighter::~ProblemReweighter(){}

genlinear::genlinear(const ptree & cfg){
    slope = ptree_get<double>(cfg, "slope", 0.0);
    offset = ptree_get<double>(cfg, "offset", 0.0);
    auto wm = ptree_get<string>(cfg, "weight-mode", "absolute");
    w_is_absolute = wm == "absolute";
    if(!w_is_absolute && wm != "relative"){
         throw runtime_error("invalid weight-mode '" + wm + "' (allowed: 'absolute' or 'relative')");
    }
    norm_factor = ptree_get<double>(cfg, "norm-factor", -1.0);
    auto nf = ptree_get<string>(cfg, "norm-factor-ref", "gen");
    nf_is_gen = nf == "gen";
    if(!nf_is_gen && nf != "reco"){
         throw runtime_error("invalid norm-factor-ref '" + nf + "' (allowed: 'gen' or 'reco')");
    }
}

Problem genlinear::operator()(const Unfolder & u, const Problem & p){
    Spectrum gen = p.gen();
    Spectrum asimov_unfolded = unfold_full(u, p.bkg(), p.R() * p.gen() + p.bkg());
    
    size_t ngen = gen.dim();
    for(size_t i=0; i<ngen; ++i){
        double w_i = offset + slope * i / ngen;
        if(w_is_absolute){
            gen[i] += w_i * gen[i];
        }
        else{
            // w_i is scale factor for the uncertainty of the unfolded spectrum:
            gen[i] += w_i * sqrt(asimov_unfolded.cov()(i,i));
        }
    }
    
    // overall factor for gen-level spectrum:
    double factor = 1.0;
    if(norm_factor >= 0.0){
        if(nf_is_gen){
            double s_gen_before = 0.0, s_gen_after = 0.0;
            for(size_t i=0; i<ngen; ++i){
                s_gen_before += p.gen()[i];
                s_gen_after += gen[i];
            }
            factor = norm_factor * s_gen_before / s_gen_after;
        }
        else{
            Spectrum r_before = p.R() * p.gen();
            Spectrum r_after = p.R() * gen;
            const size_t nreco = r_before.dim();
            double s_reco_before = 0.0, s_reco_after = 0.0;
            for(size_t i=0; i<nreco; ++i){
                s_reco_before += r_before[i];
                s_reco_after += r_after[i];
            }
            factor = norm_factor * s_reco_before / s_reco_after;
        }
    }
    if(factor != 1.0){
        for(size_t i=0; i<ngen; ++i){
            gen[i] *= factor;
        }
    }
    gen.cov().set_zero();
    return Problem(p.R(), p.R_uncertainties(), gen, p.bkg());
}


REGISTER_PROBLEM_REWEIGHTER(genlinear)


unfold_reweighted::unfold_reweighted(const ptree& cfg, TFile & infile, TDirectory& out): Module(out){
    for(auto & pcfg : cfg){
        if(pcfg.first == "type") continue;
        string type = ptree_get<string>(pcfg.second, "type");
        reweighter_names.push_back(pcfg.first);
        reweighters.emplace_back(ProblemReweighterRegistry::build(type, pcfg.second));
    }
}

void unfold_reweighted::run(const Problem& p, const Unfolder & unf){
    for(size_t i=0; i<reweighters.size(); ++i){
        Problem p_reweighted = (*reweighters[i])(unf, p);
        Spectrum r = p_reweighted.mean_reco_bkgsub();
        put(out(), spectrum_to_roothist(p_reweighted.gen(), reweighter_names[i] + "_gen", false));
        put(out(), spectrum_to_roothist(r, reweighter_names[i] + "_mean_reco", false));
        Spectrum unfolded = unfold_full(unf, p.bkg(), r + p_reweighted.bkg());
        auto hunfolded = spectrum_to_roothists(unfolded, reweighter_names[i] + "_unfolded", true);
        put(out(), move(hunfolded.first));
        put(out(), move(hunfolded.second));
    }
}

REGISTER_MODULE(unfold_reweighted)
