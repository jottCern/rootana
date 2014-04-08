#include "root.hpp"
#include "plot/include/utils.hpp"

#include "TROOT.h"
#include "TText.h"
#include "TGraph.h"
#include "TColor.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TH2D.h"

#include <memory>
#include <string>
#include <iomanip>
#include <sstream>

using namespace std;


namespace {

int corr_palette[100];

bool fill_cp(){
   double r[]    = {1., 1., 0.};
   double g[]    = {0., 1., 0.};
   double b[]    = {0., 1., 1.};
   double stop[] = {0., .5, 1.0};
   int base = TColor::CreateGradientColorTable(3, stop, r, g, b, 100);
   for (int i=0; i<100; ++i){
       corr_palette[i] = base + i;
   }
   return true;
}

bool dummy = fill_cp();

// set x axis ndivisions to a reasonable setting for integer bin indices
void set_integer_ndiv(TAxis * ax, int max = 10){
    int div = ax->GetNbins() / max;
    if(ax->GetNbins() % max > 0) div += 1;
    ax->SetNdivisions(ax->GetNbins() / div);
}

Histogram read_histo(TFile & in, const string & hname, const string & legend, int color){
    Histogram result;
    result.histo = gethisto<TH1D>(in, hname);
    result.legend = legend;
    result.histo->SetLineColor(color);
    result.histo->SetLineWidth(2);
    result.histo->GetXaxis()->SetTitle("Bin");
    result.process = result.histo->GetName();
    set_integer_ndiv(result.histo->GetXaxis());
    return move(result);
}

}

void compare_input_unfolded(TFile & in, const string & compare_to, const string & name){
    Histogram gen = read_histo(in, "input/gen", "true", kBlue);
    gen.options["legend_align"] = "left";
    gen.options["ytext"] = "Events / Bin";
    
    Histogram unfolded = read_histo(in, compare_to + "/unfolded", "unfolded " + name, kRed);
    unfolded.options["use_errors"] = "1";
    
    gen.options["draw_ratio"] = "unfolded / gen";
    gen.options["ratio_line1"] = "1";
    
    vector<Histogram> histos;
    histos.emplace_back(move(gen));
    histos.emplace_back(move(unfolded));
    draw_histos(histos, "compare_input_unfolded_" + name + ".eps");
}



// given a covariance matrix, make the corresponding correlation matrix
unique_ptr<TH2D> make_corr(TH2D & cov){
    int n = cov.GetNbinsX();
    unique_ptr<TH2D> corr(new TH2D(cov));
    for(int i=1; i<=n; ++i){
        for(int j=0; j<=n; ++j){
            double rho = cov.GetBinContent(i, j) / sqrt(cov.GetBinContent(i, i) * cov.GetBinContent(j, j));
            corr->SetBinContent(i, j, rho);
        }
    }
    corr->SetMinimum(-1.0);
    corr->SetMaximum(1.0);
    set_integer_ndiv(corr->GetXaxis());
    set_integer_ndiv(corr->GetYaxis());
    return move(corr);
}

// aclculate bin-by-bin h1 - h2:
unique_ptr<TH2D> diff(TH2D & h1, TH2D & h2){
    unique_ptr<TH2D> result(new TH2D(h1));
    result->Add(&h2, -1.0);
    return move(result);
}

void plot_corr(TH2D & corr, const string & filename){
    unique_ptr<TCanvas> c(new TCanvas("c", "c", 600, 600));
    // make a copy with corr values:
    //auto corr = make_corr(cov);
    
    gPad->SetRightMargin(0.15);
    gPad->SetLeftMargin(0.12);
    gPad->SetBottomMargin(0.12);
    gStyle->SetPalette(100, corr_palette);
    
    corr.SetMinimum(-1.0);
    corr.SetMaximum(1.0);
    corr.Draw("colz");
    
    // draw the corr coeff as text:
    vector<unique_ptr<TObject> > objects; // save objects here for later destruction
    int n = corr.GetNbinsX();
    assert(n == corr.GetNbinsY());
    for(int ix=1; ix <= n; ++ix){
        for(int iy=1; iy <= n; iy++){
            if(ix == iy) continue;
            char buf[10];
            snprintf(buf, 10, "%+.2f", corr.GetBinContent(ix, iy));
            TText * t = new TText(corr.GetXaxis()->GetBinCenter(ix), corr.GetYaxis()->GetBinCenter(iy), buf);
            objects.emplace_back(t);
            t->SetTextAlign(22); // h-center / v-center
            t->SetTextColor(1);
            t->SetTextSize(0.02);
            t->Draw();
        }
    }
    
    c->Print(filename.c_str());
}


void plot_reg_graphs(TFile & in){
    std::vector<string> result;
    TDirectory * dir = dynamic_cast<TDirectory*>(in.Get("regularization"));
    assert(dir);
    get_names_of_type(result, dir, "TGraph");
    for(size_t i=0; i<result.size(); ++i){
        unique_ptr<TCanvas> c(new TCanvas("c", "c", 600, 600));
        c->SetLogx();
        TGraph * g = dynamic_cast<TGraph*>(dir->Get(result[i].c_str()));
        g->SetMarkerStyle(20);
        g->SetMarkerSize(0.5);
        g->SetLineWidth(2);
        g->SetLineColor(kGray);
        g->Draw("APL");
        g->GetHistogram()->GetXaxis()->SetTitle("#tau");
        g->GetHistogram()->GetYaxis()->SetTitle("#rho");
        stringstream name;
        name << "reg_graph_" << result[i] << ".eps";
        c->Print(name.str().c_str());
    }
}

void plot_pull(TFile & in){
    Histogram h = read_histo(in, "nominal_toys/pull", "Toy mean and std.-dev.", kBlue);
    h.options["ytext"] = "(unfolded - true) / error";
    h.options["use_errors"] = "1";
    h.options["use_yrange"] = "1";
    h.histo->SetMinimum(-2.0);
    h.histo->SetMaximum(2.0);
    vector<Histogram> histos;
    histos.emplace_back(move(h));
    draw_histos(histos, "pull.eps");
}

struct hvec{
   explicit hvec(Histogram && h) { vec.push_back(move(h)); }
   hvec & operator()(Histogram && h){ vec.push_back(move(h)); return *this; }
   vector<Histogram> vec;
};

void plot_reweighted(TFile & in, const string & name){
    auto gen = read_histo(in, "input/gen", "original true", kBlue);
    auto gen_r = read_histo(in, "reweighted/" + name + "_gen", "true reweighted", kRed);
    auto unfolded_r = read_histo(in, "reweighted/" + name + "_unfolded", "reweighted unfolded", kGreen);
    gen.process = "gen_orig";
    gen.options["draw_ratio"] = name + "_unfolded / " + name + "_gen";
    gen.options["ratio_line1"] = "1";
    gen.options["ratio_ytext"] = "unfolded / true";
    gen.options["ytext"] = "Events / Bin";
    gen.options["legend_align"] = "left";
    unfolded_r.options["use_errors"] = "1";
    hvec histos(move(gen));
    histos(move(gen_r))(move(unfolded_r));
    draw_histos(histos.vec, "reweight_" + name + ".eps");
}

int main(){
    TH1::AddDirectory(false);
    TFile in("out.root", "read");
    
    plot_pull(in);
    plot_reg_graphs(in);
    
    compare_input_unfolded(in, "asimov_toy", "asimov");
    compare_input_unfolded(in, "nominal_toys", "toys");
    
    auto cov_asimov = gethisto<TH2D>(in, "asimov_toy/unfolded_cov_est_mean");
    auto cov_toys = gethisto<TH2D>(in, "nominal_toys/unfolded_cov");
    auto cov_toys_est_mean = gethisto<TH2D>(in, "nominal_toys/unfolded_cov_est_mean");
    plot_corr(*make_corr(*cov_asimov), "corr_asimov.eps");
    plot_corr(*make_corr(*cov_toys), "corr_toys.eps");
    plot_corr(*make_corr(*cov_toys_est_mean), "corr_toys_est_mean.eps");
    
    // plot difference in correlation:
    auto corr_asimov = make_corr(*cov_asimov);
    auto corr_toys = make_corr(*cov_toys);
    auto dcorr = diff(*corr_asimov, *corr_toys);
    plot_corr(*dcorr, "dcorr_asimov_toys.eps");
    
    auto bkg_s_cov = gethisto<TH2D>(in, "nominal_toys/bkg_s_cov");
    plot_corr(*bkg_s_cov, "bkg_s_cov_toys.eps");
    auto bkg_cov = gethisto<TH2D>(in, "input/bkg_cov");
    plot_corr(*bkg_cov, "bkg_s_cov_input.eps");
    
    // plot result of reweighting test:
    plot_reweighted(in, "linear");
    plot_reweighted(in, "scaled");
    plot_reweighted(in, "rel2");
    plot_reweighted(in, "relm2");
}

