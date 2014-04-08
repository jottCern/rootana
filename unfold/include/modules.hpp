#ifndef UNFOLD_MODULES_HPP
#define UNFOLD_MODULES_HPP

#include "unfold.hpp"


/** \brief Perform background subtraction and unfolding of given reco data
 *
 * Configuration:
 * \code
 *  type unfold
 *  input { ; some SpectrumSource specifying the reco spectrum, e.g.:
 *     histogram hreco
 *  }
 *  assume_poisson false   ; optional, default is true
 * \endcode
 * 
 * \c input is the reconstructed spectrum to unfold
 * 
 * \c assume_poisson : if true (default), the covariance of the input spectrum is ignored and replaces by Poisson (sqrt(n)) uncertainties.
 * 
 * The background is subtracted and the resulting histogram with covariance matrix is saved in the output as TH1D and TH2D, resp.
 */
class unfold: public Module{
public:
    unfold(const ptree& cfg, TFile & infile, TDirectory& out);
    virtual void run(const Problem& p, const Unfolder & unf);
    
private:
    Spectrum reco;
};


/** \brief Make toys to derive uncertainties or validate error propagation
 * 
 * Configuration:
 * \code
 * type toys
 * n 1000
 * seed 1 ; optional random seed. Default is 0 = auto
 * all-hists true ; optional. Default is false
 * dice-stat true ;optional. Default is true
 * dice-bkgsyst true ; optional. Default is true
 * problem { ; optional; default is to use global problem
 * }
 * \endcode
 * 
 * \c n is the number of toys
 * 
 * \c seed is the random number generator seed. The dfault value of 0 will generate a random seed based on local time, process id and hostname which should be very unlikely
 *    to produce collissions.
 * 
 * \c dice-stat and \c dice-bkgsyst control how toys are made (see below). The default is to randomize everything.
 * 
 * \c problem controls which histograms are used to generate toy data from. The dfault is to use the global 'problem' definition. Specifying here another one allows
 *   to test how the unfolding deals with other input (e.g. systematically shifted)
 * 
 * The sequence of operations for each toy is:
 *  1. generate signal-only Poisson random numbers on reco level  (reco_n)
 *  2. generate reco-level background according to the background systematics covariance using multivariate normal (bkg_s)
 *  3. generate background Poisson, using bkg_s as input (bkg_n)
 *  4. unfold sum of reco-level signal + background (unfolded)
 * 
 * In case \c dice-stat is \c false, steps 1. and 3. are skipped. In case \c dice-bkgsyst is \c false, step 2. is skipped
 * 
 * As default, the module produces:
 *  a. ensemble mean and covariance of the "unfolded" spectra: "unfolded_mean", "unfolded_cov"
 *  b. pull distributions on gen-level for each bin seperately (output of step 4 compared to input): "pull_i" where is the bin index of the gen-level spectrum
 *  c. mean covariance matrix for the unfolded result, as estimated by the unfolding: "unfolded_cov_est_mean"
 * 
 * The basic sanity checks to perform are:
 *  - unfolded_mean should be close to the input gen histogram
 *  - unfolded_cov should be close to unfolded_cov_est_mean
 *  - pull_i should be approximately normal around 0 with width 1
 * 
 * In case \c all-hists is \c true, all histograms in parantheses above are also saved, i.e. reco_n_i, bkg_s_i, bkg_n_i, unfolded_i where i is the toy index.
 */
class toys: public Module {
public:
    toys(const ptree& cfg, TFile & infile, TDirectory& out);
    virtual void run(const Problem& p, const Unfolder & unf);
    
private:
    // from the config:
    int n;
    uint64_t seed;
    bool all_hists, dice_stat, dice_bkgsyst;
    boost::optional<Problem> problem;
};


class write_input: public Module {
public:
    write_input(const ptree& cfg, TFile & infile, TDirectory& out);
    virtual void run(const Problem& p, const Unfolder & unf);
};


/** \brief Abstract base class to modify Problems for testing
 * 
 * Typically used within a unfold_reweighted module. For details, see documentation of the derived classes.
 */
class ProblemReweighter {
public:
    virtual Problem operator()(const Unfolder & u, const Problem & p) = 0;
    virtual ~ProblemReweighter();
};

typedef Registry<ProblemReweighter, std::string, const ptree&> ProblemReweighterRegistry;
#define REGISTER_PROBLEM_REWEIGHTER(T) namespace { int dummy##T = ::ProblemReweighterRegistry::register_<T>(#T); }

/** \brief Reweight the gen-level spectrum linearly in the bin index
 * 
 * Configuration:
 * \code
 * type genlinear
 * slope 0.1   ; optional, default is 0.0
 * offset -0.3 ; optional, default is 0.0
 * weight-mode relative ; optional. Default is "absolute"
 * norm-factor 1.0      ; optional. Default is -1.0
 * norm-factor-ref reco ; optional. Default is "gen"
 * \endcode
 * 
 * \c slope and \c offset determine the shape of the weight function. The weight w_i for each bin is given by   
 *    w_i =  offset + slope * i / nbins
 * where i is the bin index for the gen spectrum (0..nbins-1).
 * 
 * \c weight-mode determines how the weights w_i (see above) are applied: in case of "absolute", the bin content is
 *   scaled directly with 1 + w_i. In case of "relative", the quantity w_i * e_i  is added to the bin content, where e_i is the
 *   estimated error of the Asimov unfolding of the nominal problem.
 * 
 * \c norm-factor and \c norm-factor-ref determine whether or not the overall normalization of the spectrum is corrected after applying
 *   the reweighting as described above. If \c norm-factor is negative, no correction is applied. Otherwise, the gen-level spectrum
 *   is reweighted with an overall factor such that the gen-level spectrum integral is \c norm-factor times the gen-level spectrum integral.
 *   The reweighting can also be chosen such that the reco-level integral is controlled; use \c norm-factor-ref reco for that (note that technically,
 *   always the gen-level spectrum is reweighted).
 */
class genlinear: public ProblemReweighter {
public:
    explicit genlinear(const ptree & cfg);
    virtual Problem operator()(const Unfolder & u, const Problem & p);
    
private:
    double slope, offset;
    bool w_is_absolute;
    double norm_factor;
    bool nf_is_gen;
};


/** \brief Reweight a problem and unfold the reweighted Asimov spectrum
 * 
 * The configuration consists of a list of named ProblemReweighter configurations, e.g.:
 * \code
 * type unfold_rewighted
 * linear0 { 
 *   type genlinear ; a ProblemReweighter, see configuration there
 *   slope 0.1
 * }
 * scaled {
 *    type genlinear
 *    norm-factor 2.0
 * }
 * \endcode 
 * 
 * The module will go through all ProblemReweighters, apply them to the global Problem and unfold the
 * reweighted Asimov spectrum (i.e., p.mean_reco_bkgsub() where p is the reweighted problem).
 * 
 * The result of the reweighting is saved as "XX_gen" and "XX_mean_reco" where "XX" is the name ("linear0" or "linear1" in
 * the example above).
 * 
 * The result of the unfolding is saved in the output directory as "XX_unfolded" and "XX_unfolded_cov" where "XX"
 * is the name.
 */
class unfold_reweighted: public Module {
public:
    unfold_reweighted(const ptree& cfg, TFile & infile, TDirectory& out);
    virtual void run(const Problem& p, const Unfolder & unf);
    
private:
    std::vector<std::unique_ptr<ProblemReweighter>> reweighters;
    std::vector<std::string> reweighter_names;
};



/** \brief Linearize the unfolding algorithm
 *
 * A bias-free unfolder will unfold the Asimov data to the input distribution. However, due to
 * biases of the regularization, this is not perfectly true.
 * A possible measure for this bias is given by linearizing the unfolder: For true spectrum x, the
 * result of the unfolding in a neighborhood of x can be written as linear function:
 *  u_i = A_ik x_k
 * where u is the unfolded spectrum and x is the true (input) spectrum. Ideally, A is the identity
 * matrix, any deviation points to biases of the procedure.
 * Knowing A_ik numerically can be used to estimate the actual bias for some input spectra directly
 * via the above formula. What is usually desirable is that the deviation from the identity matrix has a
 * very small effect on the final result compared to the total error. To quantify this impact, the bias-covariance
 * matrix can be built which is defined as
 *   B = sum_i   b_i^T b_i
 * where b_i is a vector of biases if changing the input spectrum by 1sigma (along the eigenvector directions). B
 * is a measure of the bias in the same units as the covariance; it could be added as bias error to the usual covariance.
 *
 * This module calculates both A and B which are ngen * ngen matrices.
 *
 * \code
 * type lin_unfolding
 * 
 * \endcode
 */
class lin_unfolding: public Module {
public:
    lin_unfolding(const ptree& cfg, TFile & infile, TDirectory& out);
    virtual void run(const Problem& p, const Unfolder & unf);
};

#endif

