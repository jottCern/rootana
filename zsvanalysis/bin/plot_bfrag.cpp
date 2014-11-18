#include "plot/include/utils.hpp"

#include "TH2D.h"

#include <iostream>


using namespace std;
using namespace ra;

void compare_shapes(const vector<TH1*> hists, const vector<string> & names, const string & outname){
    vector<Histogram> histos;
    assert(hists.size() == names.size());
    vector<int> colors {kBlack, kBlue, kRed, kGreen};
    for(size_t i=0; i < hists.size(); ++i){
        Histogram h;
        h.histo.reset(hists[i]);
        h.legend = names[i];
        h.process = names[i];
        h.histo->SetLineWidth(2);
        h.histo->SetLineColor(colors[i]);
        h.histo->Scale(1.0 / h.histo->Integral());
        histos.emplace_back(move(h));
    }
    draw_histos(histos, outname);
}

int main(){
    const string outputdir = "plot_bfrag_out/";
    const string inputdir = "/nfs/dust/cms/user/ottjoc/omega-out/ntuple-latest/bfrag/";
    shared_ptr<ProcessHistograms> ttbar(new ProcessHistogramsTFile(inputdir+"ttbar*.root", "ttbar"));
    shared_ptr<ProcessHistograms> dy(new ProcessHistogramsTFile({inputdir+"dy[1-4]jetsbb*.root", inputdir+"dybb{vis,other}.root"}, "dy"));
    shared_ptr<ProcessHistograms> dybbm(new ProcessHistogramsTFile({inputdir+"dybbmbb*.root"}, "dybbm"));
    
    // compare MC B fragmentation of 'isolated' Bs between ttbar and DY:
    
    for(string hname : {"bfrag_jpt_mc_isob", "bfrag_jpt_isob"}){
        auto dyfrag = dy->get_histogram("all", hname);
        auto dybbmfrag = dybbm->get_histogram("all", hname);
        auto ttfrag = ttbar->get_histogram("all", hname);
        
        TH2D * dyfrag2d = dynamic_cast<TH2D*>(dyfrag.histo.get());
        assert(dyfrag2d);
        TH2D * dybbmfrag2d = dynamic_cast<TH2D*>(dybbmfrag.histo.get());
        assert(dybbmfrag2d);
        TH2D * ttfrag2d = dynamic_cast<TH2D*>(ttfrag.histo.get());
        assert(ttfrag2d);
        
        vector<double> ptbins {20.0, 30., 40., 60., 90., 130., 200.};
        
        for(size_t i=0; i+1 < ptbins.size(); ++i){
            auto dyp = dyfrag2d->ProjectionY("_py", dyfrag2d->GetXaxis()->FindBin(ptbins[i]), dyfrag2d->GetXaxis()->FindBin(ptbins[i+1]));
            auto dybbmp = dybbmfrag2d->ProjectionY("_py", dyfrag2d->GetXaxis()->FindBin(ptbins[i]), dyfrag2d->GetXaxis()->FindBin(ptbins[i+1]));
            auto ttbarp = ttfrag2d->ProjectionY("_py", dyfrag2d->GetXaxis()->FindBin(ptbins[i]), dyfrag2d->GetXaxis()->FindBin(ptbins[i+1]));
            stringstream name;
            name << outputdir << hname << ptbins[i] << "_" << ptbins[i+1] << ".pdf";
            dyp->Rebin(2);
            dybbmp->Rebin(2);
            ttbarp->Rebin(2);
            compare_shapes({ttbarp, dyp, dybbmp}, {"tt", "dy", "dybbm"}, name.str());
        }
            
        auto dyx = dyfrag2d->ProjectionX();
        auto dybbmx = dybbmfrag2d->ProjectionX();
        auto ttx = ttfrag2d->ProjectionX();
        compare_shapes({ttx, dyx, dybbmx}, {"tt", "dy", "dybbm"}, outputdir + hname + "_ptj.pdf");
    }
    
    auto dydr_bd = dy->get_histogram("all", "bfrag2_dr_mcbd");
    auto dydr_bb = dy->get_histogram("all", "bfrag2_dr_mcbb");
    
    TH2D* dydr_bd_2d = dynamic_cast<TH2D*>(dydr_bd.histo.get());
    TH2D* dydr_bb_2d = dynamic_cast<TH2D*>(dydr_bb.histo.get());
    TH1D* h_dydr_bd = dydr_bd_2d->ProjectionY();
    TH1D* h_dydr_bb_02 = dydr_bb_2d->ProjectionY("_py02", 1, dydr_bb_2d->GetXaxis()->FindBin(0.2));
    TH1D* h_dydr_bb = dydr_bb_2d->ProjectionY();
    h_dydr_bd->Rebin(4);
    h_dydr_bb->Rebin(4);
    h_dydr_bb_02->Rebin(4);
    compare_shapes({h_dydr_bd, h_dydr_bb, h_dydr_bb_02}, {"bd", "bb", "bb02"}, outputdir + "bb_bd.pdf");
}

