#include "TROOT.h"
#include "plot/include/utils.hpp"

#include <iostream>

using namespace std;
using namespace ra;


namespace {
string outputdir = "eps/ivf_hist_mcttbar_new_18_with_rhocut";
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
    shared_ptr<ProcessHistograms> mcttbar_full(new ProcessHistogramsTFile(inputdir+"ivf_hist_mcttbar_new_18_with_rhocut.root", "mcttbar_full"));
    
    Formatters formatters;
    formatters.add("*", SetLineColor(1));
    formatters.add<SetFillColor>("AllProcess/", kBlack)
        ("AnyDecay05/", kBlue) ("GeantHadronic05/", kRed) ("GeantConversions05/", kGreen) ("BDecay/", kBlue) ("KsDecay/", kOrange) ("PrimaryDecay/", kGreen) ("RestDecay/", kMagenta)
        ("AnyDecay05Eta2/", kBlue) ("GeantHadronic05Eta2/", kRed) ("GeantConversions05Eta2/", kGreen) ("BDecayEta2/", kBlue) ("KsDecayEta2/", kOrange) ("PrimaryDecayEta2/", kGreen) ("RestDecayEta2/", kMagenta)
        ("AnyDecay05Nt3/", kBlue) ("GeantHadronic05Nt3/", kRed) ("GeantConversions05Nt3/", kGreen) ("BDecayNt3/", kBlue) ("KsDecayNt3/", kOrange) ("PrimaryDecayNt3/", kGreen) ("RestDecayNt3/", kMagenta)
        ("AnyDecay05Pt8/", kBlue) ("GeantHadronic05Pt8/", kRed) ("GeantConversions05Pt8/", kGreen) ("BDecayPt8/", kBlue) ("KsDecayPt8/", kOrange) ("PrimaryDecayPt8/", kGreen) ("RestDecayPt8/", kMagenta)
        ("AnyDecay05Mass/", kBlue) ("GeantHadronic05Mass/", kRed) ("GeantConversions05Mass/", kGreen) ("BDecayMass/", kBlue) ("KsDecayMass/", kOrange) ("PrimaryDecayMass/", kGreen) ("RestDecayMass/", kMagenta)
        ("AnyDecay05Sig3dFIN/", kBlue) ("GeantHadronic05Sig3dFIN/", kRed) ("GeantConversions05Sig3dFIN/", kGreen) ("BDecaySig3dFIN/", kBlue) ("KsDecaySig3dFIN/", kOrange) ("PrimaryDecaySig3dFIN/", kGreen) ("RestDecaySig3dFIN/", kMagenta)
        ("AnyDecay05Rho28/", kBlue) ("GeantHadronic05Rho28/", kRed) ("GeantConversions05Rho28/", kGreen) ("BDecayRho28/", kBlue) ("KsDecayRho28/", kOrange) ("PrimaryDecayRho28/", kGreen) ("RestDecayRho28/", kMagenta)
        ;
    formatters.add<SetLegends>
        ("AllProcess/", "All Processes")
        ("AnyDecay05/", "Decays (0.5)") ("GeantHadronic05/", "Hadronic Processes, NI (0.5)") ("GeantConversions05/", "Conversion Processes (0.5)") ("BDecay/", "B Decays") ("KsDecay/", "Kshort") ("PrimaryDecay/", "Primary Fakes") ("RestDecay/", "Rest Decays")
        ("AnyDecay05Eta2/", "Decays (0.5)") ("GeantHadronic05Eta2/", "Hadronic Processes, NI (0.5)") ("GeantConversions05Eta2/", "Conversion Processes (0.5)") ("BDecayEta2/", "B Decays") ("KsDecayEta2/", "Kshort") ("PrimaryDecayEta2/", "Primary Fakes") ("RestDecayEta2/", "Rest Decays")
        ("AnyDecay05Nt3/", "Decays (0.5)") ("GeantHadronic05Nt3/", "Hadronic Processes, NI (0.5)") ("GeantConversions05Nt3/", "Conversion Processes (0.5)") ("BDecayNt3/", "B Decays") ("KsDecayNt3/", "Kshort") ("PrimaryDecayNt3/", "Primary Fakes") ("RestDecayNt3/", "Rest Decays")
        ("AnyDecay05Pt8/", "Decays (0.5)") ("GeantHadronic05Pt8/", "Hadronic Processes, NI (0.5)") ("GeantConversions05Pt8/", "Conversion Processes (0.5)") ("BDecayPt8/", "B Decays") ("KsDecayPt8/", "Kshort") ("PrimaryDecayPt8/", "Primary Fakes") ("RestDecayPt8/", "Rest Decays")
        ("AnyDecay05Mass/", "Decays (0.5)") ("GeantHadronic05Mass/", "Hadronic Processes, NI (0.5)") ("GeantConversions05Mass/", "Conversion Processes (0.5)") ("BDecayMass/", "B Decays") ("KsDecayMass/", "Kshort") ("PrimaryDecayMass/", "Primary Fakes") ("RestDecayMassFIN/", "Rest Decays")
        ("AnyDecay05Sig3dFIN/", "Decays (0.5)") ("GeantHadronic05Sig3dFIN/", "Hadronic Processes, NI (0.5)") ("GeantConversions05Sig3dFIN/", "Conversion Processes (0.5)") ("BDecaySig3dFIN/", "B Decays") ("KsDecaySig3dFIN/", "Kshort") ("PrimaryDecaySig3dFIN/", "Primary Fakes") ("RestDecaySig3dFIN/", "Rest Decays")
        ("AnyDecay05Rho28/", "Decays (0.5)") ("GeantHadronic05Rho28/", "Hadronic Processes, NI (0.5)") ("GeantConversions05Rho28/", "Conversion Processes (0.5)") ("BDecayRho28/", "B Decays") ("KsDecayRho28/", "Kshort") ("PrimaryDecayRho28/", "Primary Fakes") ("RestDecayRho28/", "Rest Decays")
        ;
    formatters.add<SetOption>/*("Rho1D", "use_yrange", "1") ("VertMass", "use_yrange", "1") ("FlightDirSig3d", "use_yrange", "1")*/ ("VertMass", "xmin", "0") ("VertMass", "xmax", "10") ("Rho1D", "ylog", "1") ("VertMass", "ylog", "1") ("*", "ytext", "#Vertices") ("VertMass", "xtext", "Mass [GeV]") ("Rho1D", "xtext", "Rho [cm]") ("TrackMult", "xtext", "Number Tracks") ("VertPt", "xtext", "p_{T} [GeV]") ("FlightDirSig3d", "xtext", "flight dir sig3D") ("FlightDirSig3d", "ylog", "1");
    
     Plotter p(outputdir, {mcttbar_full}, formatters);
     
//      p.stackplots({"mcttbar_full"});
     p.selcomp_plots({"AllProcess", "AnyDecay05", "GeantHadronic05", "GeantConversions05", "RestDecay"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantonly_selcomp");
     p.selcomp_plots({"GeantHadronic05", "BDecay", "KsDecay", "PrimaryDecay"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantdec_selcomp");
     
     p.selcomp_plots({"AllProcess", "AnyDecay05Eta2", "GeantHadronic05Eta2", "GeantConversions05Eta2", "RestDecayEta2"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantonly_selcomp_Eta2");
     p.selcomp_plots({"GeantHadronic05Eta2", "BDecayEta2", "KsDecayEta2", "PrimaryDecayEta2"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantdec_selcomp_Eta2");
     
     p.selcomp_plots({"AllProcess", "AnyDecay05Nt3", "GeantHadronic05Nt3", "GeantConversions05Nt3", "RestDecayNt3"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantonly_selcomp_Nt3");
     p.selcomp_plots({"GeantHadronic05Nt3", "BDecayNt3", "KsDecayNt3", "PrimaryDecayNt3"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantdec_selcomp_Nt3");
     
     p.selcomp_plots({"AllProcess", "AnyDecay05Pt8", "GeantHadronic05Pt8", "GeantConversions05Pt8", "RestDecayPt8"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantonly_selcomp_Pt8");
     p.selcomp_plots({"GeantHadronic05Pt8", "BDecayPt8", "KsDecayPt8", "PrimaryDecayPt8"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantdec_selcomp_Pt8");
     
     p.selcomp_plots({"AllProcess", "AnyDecay05Mass", "GeantHadronic05Mass", "GeantConversions05Mass", "RestDecayMass"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantonly_selcomp_Mass");
     p.selcomp_plots({"GeantHadronic05Mass", "BDecayMass", "KsDecayMass", "PrimaryDecayMass"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantdec_selcomp_Mass");
     
     p.selcomp_plots({"AllProcess", "AnyDecay05Sig3dFIN", "GeantHadronic05Sig3dFIN", "GeantConversions05Sig3dFIN", "RestDecaySig3dFIN"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantonly_selcomp_Sig3dFIN");
     p.selcomp_plots({"GeantHadronic05Sig3dFIN", "BDecaySig3dFIN", "KsDecaySig3dFIN", "PrimaryDecaySig3dFIN"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantdec_selcomp_Sig3dFIN");
     
     p.selcomp_plots({"AllProcess", "AnyDecay05Rho28", "GeantHadronic05Rho28", "GeantConversions05Rho28", "RestDecayRho28"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantonly_selcomp_Rho28");
     p.selcomp_plots({"GeantHadronic05Rho28", "BDecayRho28", "KsDecayRho28", "PrimaryDecayRho28"}, {"Rho1D", "VertMass", "TrackMult", "VertPt", "FlightDirSig3d"}, "ZZ_geantdec_selcomp_Rho28");
//      p.cutflow("cutflow", "cutflow.tex");
    
    //p.shapeplots({"dy", "dyexcl"}); 
    
    //Plotter p("eps", {dy, dyexcl}, formatters);
    //p.stackplots({});
}
