#include "root.hpp"
#include "base/include/ptree-utils.hpp"

#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TGraph.h"

#include <memory>

using namespace std;

std::pair<Matrix, Matrix> roothist_to_matrix(const TH2D & hist, bool transpose){
    int ngen = hist.GetNbinsY();
    int nreco = hist.GetNbinsX();
    if(transpose){
        swap(ngen, nreco);
    }
    
    Matrix m = Matrix(nreco, ngen);
    Matrix m_e = Matrix(nreco, ngen);
    
    for(int i=0; i<nreco; ++i){
        for(int j=0; j<ngen; ++j){
            int nx = i+1;
            int ny = j+1;
            if(transpose) swap(nx, ny);
            m(i,j) = hist.GetBinContent(nx, ny);
            m_e(i,j) = hist.GetBinError(nx, ny);
        }
    }
    return pair<Matrix, Matrix>(move(m), move(m_e));
}

Spectrum roothist_to_spectrum(const TH1D & hist, const TH2D * cov, bool use_hist_uncertainties){
    const int n = hist.GetNbinsX();
    if(cov){
        if(cov->GetDimension() != 2 || cov->GetNbinsX() != n || cov->GetNbinsY() != n){
            throw runtime_error(string("covariance histogram '") + cov->GetName() + "' has wrong dimension");
        }
    }
    Spectrum s(n);
    for(int i=0; i<n; ++i){
        double c = hist.GetBinContent(i+1);
        if(!isfinite(c)){
            throw runtime_error(string("non-finite entry in histogram '") + hist.GetName() + "'");
        }
        s[i] = c;
        if(use_hist_uncertainties){
            double e = hist.GetBinError(i+1);
            if(!isfinite(e)){
                throw runtime_error(string("non-finite error in histogram '") + hist.GetName() + "'");
            }
            s.cov()(i,i) += e*e;
        }
    }
    if(cov){
        for(int i=0; i<n; ++i){
            for(int j=0; j<n; ++j){
                double c_ij = cov->GetBinContent(i+1, j+1);
                double c_ji = cov->GetBinContent(j+1, i+1);
                if(!isfinite(c_ij)){
                    throw runtime_error(string("covariance histogram '") + cov->GetName() + "' does have non-finite entry");
                }
                if(fabs(c_ij - c_ji) > 1e-8 * max(fabs(c_ij), 1.0)){
                    cerr << "covariance histogram '" << cov->GetName() <<  "' is not symmetric: C(" << i << ","<< j << ") = " << c_ij << "; transposed: " << c_ji << "; diff = " << (c_ij - c_ji) << endl;
                }
                s.cov()(i,j) += c_ij;
            }
        }
    }
    return s;
}

std::pair<std::unique_ptr<TH1D>, std::unique_ptr<TH2D> > spectrum_to_roothists(const Spectrum & s, const std::string & hname, bool set_1d_errors){
    return std::pair<std::unique_ptr<TH1D>, std::unique_ptr<TH2D>>(spectrum_to_roothist(s, hname, set_1d_errors), matrix_to_roothist(s.cov(), hname + "_cov"));
}

std::unique_ptr<TH2D> matrix_to_roothist(const Matrix & m, const string & hname){
    unique_ptr<TH2D> res(new TH2D(hname.c_str(), hname.c_str(), m.get_n_rows(), 0, m.get_n_rows(), m.get_n_cols(), 0, m.get_n_cols()));
    for(size_t i=0; i<m.get_n_rows(); ++i){
        for(size_t j=0; j<m.get_n_cols(); ++j){
            res->SetBinContent(i+1, j+1, m(i,j));
        }
    }
    return move(res);
}

std::unique_ptr<TH1D> spectrum_to_roothist(const Spectrum & s, const std::string & hname, bool set_errors){
    const int n = s.dim();
    unique_ptr<TH1D> values(new TH1D(hname.c_str(), hname.c_str(), n, 0, n));
    for(int i=0; i<n; ++i){
        values->SetBinContent(i+1, s[i]);
        if(set_errors){
            double err = sqrt(s.cov()(i,i));
            values->SetBinError(i+1, err);
        }
    }
    return move(values);
}

unique_ptr<TGraph> make_tgraph(const map<double, double> & values, const string & name){
    const size_t n = values.size();
    unique_ptr<TGraph> result(new TGraph(n));
    result->SetName(name.c_str());
    double * x = result->GetX();
    double * y = result->GetY();
    size_t i = 0;
    for(auto & xy : values){
        x[i] = xy.first;
        y[i] = xy.second;
        ++i;
    }
    return result;
}

void put(TDirectory & out, std::unique_ptr<TH1> h){
    out.cd();
    h->SetDirectory(&out);
    h->Write();
    // h is deleted ...
}

void put(TDirectory & out, std::unique_ptr<TGraph> tg){
   out.cd();
   tg->Write();
}
