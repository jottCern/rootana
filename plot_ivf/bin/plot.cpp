#include "TROOT.h"
#include "plot/include/utils.hpp"

#include <iostream>

using namespace std;
using namespace ra;


namespace {
string outputdir = "eps/ivf_hist_mcttbar_new_11_with_ivfsels_full_selcomp6";
string inputdir = "../ivf_treeAnalysis/rootfiles/";
}

void plot_eff(ProcessHistograms & input, const string & numerator, const string & denom, const Formatters & format){
    auto n = input.get_histogram(numerator);
    auto d = input.get_histogram(denom);
    format(d);
    format(n);
    n.histo->Divide(d.histo.get());
    n.options["draw_ratio"] = "";
    n.options["use_errors"] = "true";
    n.options["ytext"] = "#epsilon";
    vector<Histogram> histos;
    histos.emplace_back(move(n));
    string outfilename = outputdir + "/" + numerator + "_" + input.id().name() + "_eff.pdf";
    create_dir(outfilename);
    draw_histos(histos, outfilename);
}


int main(){
    shared_ptr<ProcessHistograms> mcttbar_full(new ProcessHistogramsTFile(inputdir+"ivf_hist_mcttbar_new_9_addhists_test.root", "mcttbar_full"));
    
    Formatters formatters;
    formatters.add("*", SetLineColor(1));
    formatters.add<SetFillColor>("AllProcess/", kBlack) ("AnyDecay05/", kBlue) ("GeantHadronic05/", kRed) ("GeantConversionsc05/", kGreen) ("BDecay/", kBlue) ("CDecay/", kOrange) ("PrimaryDecay/", kGreen);
//     formatters.add<SetLineColor>("mcttbar_full:", 810);
    formatters.add<SetLegends>("AllProcess/", "All Processes")("AnyDecay05/", "Decays (0.5)") ("GeantHadronic05/", "Hadronic Processes, NI (0.5)") ("GeantConversionsc05/", "Conversion Processes (0.5)") ("BDecay/", "B Decays") ("CDecay/", "C Decays") ("PrimaryDecay/", "Primary Fakes");
    formatters.add<SetOption>("VertMass", "xmin", "0") ("VertMass", "xmax", "10") ("Rho1D", "ylog", "1") ("VertMass", "ylog", "1") ("*", "ytext", "Normalized Units") ("VertMass", "xtext", "Mass [GeV]") ("Rho1D", "xtext", "Rho [cm]") ("TrackMult", "xtext", "Number Tracks");
//     formatters.add<SetOption>
//        ("*", "title_ur", "19.7 fb^{-1}, #sqrt{s}=8TeV")
//        ("*", "draw_ratio", "1")
//        ("*", "ratio_ymax", "2")
//        ("*", "ratio_ymin", "0.5")
//        ("*", "ytext", "events")
//        ("mll", "xtext", "m_{ll} [GeV]")
//        ("DR_ZB", "xtext", "#Delta R(Z,B)")
//        ("DR_BB", "xtext", "#Delta R(B,B)")
//        ("DR_VV", "xtext", "#Delta R(V,V)")
//        ("A_ZBB", "xtext", "A_{ZBB}")
//        ("met", "xtext", "E_{T}^{miss} [GeV]")
//        ("m_BB", "xtext", "m_{BB} [GeV]")
//        ("Beta", "xtext", "#eta_B")
//        ("Bpt", "xtext", "p_{T,B} [GeV]")
//        ("min_DR_ZB", "xtext", "min #Delta R(Z,B)")
//        ("DPhi_BB", "xtext", "#Delta #phi(B,B)")
//        ("DR_BB", "ratio_ymin", "0.5")
//        ("DR_BB", "ratio_ymax", "1.5")
//        ;
//     formatters.add<RebinFactor>("mll", 4)("ptz", 4)("Bpt",4)("Beta",4)("DPhi_BB", 2)("DR_BB", 2)("m_BB", 4);
    
    // scale final selection by 0.92**2 (IVF data/MC eficiency factor):
    /*formatters.add<Scale>("final/dy:", 0.92 * 0.92);
    formatters.add<Scale>("final/mcttbar_full:", 0.92 * 0.92);*/
    
     Plotter p(outputdir, {mcttbar_full}, formatters);
     
     p.stackplots({"mcttbar_full"});
     p.selcomp_plots({"AllProcess", "AnyDecay05", "GeantHadronic05", "GeantConversionsc05"}, {"Rho1D", "VertMass", "TrackMult"}, "geantonly_selcomp");
     p.selcomp_plots({"GeantHadronic05", "BDecay", "CDecay", "PrimaryDecay"}, {"Rho1D", "VertMass", "TrackMult"}, "geantdec_selcomp");
//      p.cutflow("cutflow", "cutflow.tex");
    
    //p.shapeplots({"dy", "dyexcl"});
    
    //Plotter p("eps", {dy, dyexcl}, formatters);
    //p.stackplots({});
}
