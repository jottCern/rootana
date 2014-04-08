// make some test input for the unfolding framework
//
// The different models are in subdirectories of the output file.
//
// modelt: trivial 1*1 model
// * response: 1*1 matrix with 1.0
// * background: none
// * gen: one entry with N=1000
//
// modelt2: trivial 1*1 model with non-unit response
// * response: 1*1 matrix with 0.1
// * background: none
// * gen: one entry with N=1000
//
// model0: simple response
// * response: 10*10, diagonal from 0.1 to 1.0; no uncertainties
// * background: none
// * gen: double-gauss: on 0, 1 one is at 0.5 +- 0.3; the other at 0.8 +- 0.1. Nevents = 10000
//
// model1: some off-diagonals in response
// * response: 10*10 with slight off-diagonal elements (always like 0.1 0.8 0.1)
// * rest like in model0
//
// model2: non-quadratic response
// * response: nreco=20, ngen=10, some off-diagonals
// * rest like in model0
//
// model3: simple response with simple background
// * response, gen: like in model0
// * background: flat, N = 4000, no uncertainties
// 
// model4: some off-diagonals in response, with simple background
// * response, gen: like in model1
// * background: like in model3
//
// model5: simple response with background uncertainties
// * reponse: like in model0
// * background: flat, with bin-by-bin systematic uncertainties = 2 * stat uncertainty
//
// model6: trivial response with non-trivial background uncertainty
// * response: like in model0
// * background: flat, with bin-by-bin systematic uncertainties = 2 * stat. + common (100% correlated) uncertainty of 2 * stat.
//
// model7: some off-diagonal response with more complicated background uncertainties
// * response: like in model1
// * background: like in model6

#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"
#include <vector>
#include <cmath>

#include "root.hpp"

using namespace std;

void modelt_gen(TDirectory & d){
    unique_ptr<TH1D> result(new TH1D("gen", "gen", 1, 0.0, 1.0));
    result->SetBinContent(1, 1000);
    put(d, move(result));
}

void modelt_response(TDirectory & d){
    unique_ptr<TH2D> result(new TH2D("response", "response", 1, 0.0, 1.0, 1, 0.0, 1.0));
    result->SetBinContent(1,1,1.0);
    put(d, move(result));
}

void modelt2_response(TDirectory & d){
    unique_ptr<TH2D> result(new TH2D("response", "response", 1, 0.0, 1.0, 1, 0.0, 1.0));
    result->SetBinContent(1,1,0.1);
    put(d, move(result));
}

void model0_gen(TDirectory & d){
    unique_ptr<TH1D> result(new TH1D("gen", "gen", 10, 0.0, 1.0));
    
    vector<double> g1(10), g2(10);
    double g1_tot = 0.0;
    double g2_tot = 0.0;
    for(int i=0; i<10; ++i){
        double x = (i+0.5) / 10.0;
        g1_tot += g1[i] = exp(-pow((x - 0.5) / 0.3, 2) / 2);
        g2_tot += g2[i] = exp(-pow((x - 0.8) / 0.1, 2) / 2);
    }
    for(int i=0; i<10; ++i){
        result->SetBinContent(i+1, 5000 * (g1[i]/g1_tot + g2[i] / g2_tot));
    }
    put(d, move(result));
}

// response of model0:
void model0_response(TDirectory & d){
    unique_ptr<TH2D> result(new TH2D("response", "response", 10, 0.0, 1.0, 10, 0.0, 1.0));
    for(int i=1; i<=10; ++i){
        result->SetBinContent(i, i, 0.1 * i);
    }
    put(d, move(result));
}


void model1_response(TDirectory & d){
    unique_ptr<TH2D> result(new TH2D("response", "response", 10, 0.0, 1.0, 10, 0.0, 1.0));
    for(int i=1; i<=10; ++i){
        for(int j=1; j<=10; ++j){
            double p = 0.0;
            if(i==j) p = 0.8;
            if(abs(i-j) < 1.5) p = 0.1;
            result->SetBinContent(i, j, p);
        }
    }
    put(d, move(result));
}

vector<double> gauss(int nbins, double xmin, double xmax, double mean, double width, double norm){
    double dx = (xmax - xmin) / nbins;
    vector<double> result(nbins);
    double sum = 0.0;
    for(int i=0; i<nbins; ++i){
        double x = xmin + (i + .5) * dx;
        result[i] = exp(-0.5 * pow((x - mean)/width, 2));
        sum += result[i];
    }
    for(int i=0; i<nbins; ++i){
        result[i] *= norm / sum;
    }
    return result;
}

void model2_response(TDirectory & d){
    int nrec = 20, ngen = 10;
    // x = reco
    unique_ptr<TH2D> result(new TH2D("response", "response", 20, 0.0, 1.0, 10, 0.0, 1.0));
    for(int j=1; j<=ngen; ++j){
        double gen = (j - 0.5) / ngen;
        auto g = gauss(nrec, 0.0, 1.0, gen, 0.05, 0.8);
        for(int i=1; i<=nrec; ++i){
            result->SetBinContent(i, j, g[i-1]);
        }
    }
    put(d, move(result));
}

void model3_bkg(TDirectory & d){
    int nrec = 10;
    unique_ptr<TH1D> result(new TH1D("bkg", "bkg", 10, 0.0, 1.0));
    for(int j=1; j<=nrec; ++j){
        result->SetBinContent(j, 4000. / nrec);
    }
    put(d, move(result));
}

void model5_bkg(TDirectory & d){
    int nrec = 10;
    unique_ptr<TH1D> result(new TH1D("bkg", "bkg", 10, 0.0, 1.0));
    for(int j=1; j<=nrec; ++j){
        result->SetBinContent(j, 4000. / nrec);
    }
    unique_ptr<TH2D> result_cov(new TH2D("bkg_cov", "bkg_cov", 10, 0.0, 1.0, 10, 0.0, 1.0));
    for(int j=1; j<=nrec; ++j){
        result_cov->SetBinContent(j, j, 4 * result->GetBinContent(j));
    }
    put(d, move(result));
    put(d, move(result_cov));
}

void model6_bkg(TDirectory & d){
    int nrec = 10;
    unique_ptr<TH1D> result(new TH1D("bkg", "bkg", 10, 0.0, 1.0));
    for(int j=1; j<=nrec; ++j){
        result->SetBinContent(j, 4000. / nrec);
    }
    unique_ptr<TH2D> result_cov(new TH2D("bkg_cov", "bkg_cov", 10, 0.0, 1.0, 10, 0.0, 1.0));
    for(int i=1; i<=nrec; ++i){
        for(int j=1; j<=nrec; ++j){
            double sigma2_i_stat = result->GetBinContent(i);
            double sigma2_j_stat = result->GetBinContent(j);
            // the correlated part is rho * sigma_i * sigma_j:
            double cov = sqrt(sigma2_i_stat * sigma2_j_stat);
            if(i==j){
                cov *= 2;
            }
            result_cov->SetBinContent(i, j, cov);
        }
    }
    put(d, move(result));
    put(d, move(result_cov));
}

int main(){
    TH1::AddDirectory(0);
    
    TFile f("in.root", "recreate");
    
    TDirectory * d = f.mkdir("modelt");
    modelt_response(*d);
    modelt_gen(*d);
    
    d = f.mkdir("modelt2");
    modelt2_response(*d);
    modelt_gen(*d);
    
    d = f.mkdir("model0");
    model0_response(*d);
    model0_gen(*d);
    
    d = f.mkdir("model1");
    model0_gen(*d);
    model1_response(*d);
    
    d = f.mkdir("model2");
    model0_gen(*d);
    model2_response(*d);
    
    d = f.mkdir("model3");
    model0_gen(*d);
    model0_response(*d);
    model3_bkg(*d);
    
    d = f.mkdir("model4");
    model0_gen(*d);
    model1_response(*d);
    model3_bkg(*d);
    
    d = f.mkdir("model5");
    model0_gen(*d);
    model0_response(*d);
    model5_bkg(*d);
    
    d = f.mkdir("model6");
    model0_gen(*d);
    model0_response(*d);
    model6_bkg(*d);
    
    d = f.mkdir("model7");
    model0_gen(*d);
    model1_response(*d);
    model6_bkg(*d);
}

