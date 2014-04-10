#ifndef UNFOLD_ROOT_HPP
#define UNFOLD_ROOT_HPP

#include "unfold.hpp"
#include "fwd.hpp"

#include "TFile.h"

// convert the root histograms hist and hist_cov to a spectrum. hist_cov might be 0.
// If use_hist_uncertainties is true, the bin errors set in hist are used as additional (!) bin-by-bin errors, i.e.
// added to the diagonal of the covariance matrix of the result.
Spectrum roothist_to_spectrum(const TH1D & hist, const TH2D * hist_cov = 0, bool use_hist_uncertainties = false);

// fill the values and uncertainties of hist in the result matrices.
std::pair<Matrix, Matrix> roothist_to_matrix(const TH2D & hist, bool transpose = false);

// convert a map<double, double> to a TGraph
std::unique_ptr<TGraph> make_tgraph(const std::map<double, double> & values, const std::string & name);

// Convert a Spectrum to root histograms.
// The first contains the central values, the second is the covariance as a TH2D with name <name>_cov containing the covariance information.
// Will throw an exception if there are infinites or NaNs in h.
std::unique_ptr<TH1D> spectrum_to_roothist(const Spectrum & s, const std::string & hname, bool set_errors);
std::unique_ptr<TH2D> matrix_to_roothist(const Matrix & m, const std::string & hname);
std::pair<std::unique_ptr<TH1D>, std::unique_ptr<TH2D> > spectrum_to_roothists(const Spectrum & s, const std::string & hname, bool set_1d_errors);

// put TH1 in the given output directory, giving up ownership (!)
void put(TDirectory & out, std::unique_ptr<TH1> h);
void put(TDirectory & out, std::unique_ptr<TGraph> tg);

#endif
