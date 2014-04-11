#include "TROOT.h"
#include "utils.hpp"

#include <iostream>

using namespace std;
using namespace ra;

int main(){
    shared_ptr<ProcessHistograms> ttbar(new ProcessHistogramsTFile("../ttbar.root", "ttbar"));
    shared_ptr<ProcessHistograms> dy(new ProcessHistogramsTFile({"../dy1jets.root", "../dy2jets.root", "../dy3jets.root", "../dy4jets.root", "../dy.root"}, "dy"));
    shared_ptr<ProcessHistograms> data(new ProcessHistogramsTFile({"../dmu_runa.root", "../dmu_runb.root", "../dmu_runc.root", "../dmu_rund.root"}, "data"));
    
    Formatters formatters;
    formatters.add("*", SetLineColor(1));
    formatters.add<SetFillColor>("ttbar:", 810)  ("dymix:", 414) ("dyexcl:", kMagenta) ("dy:", kBlue);
    //formatters.add<SetLineColor>("ttbar:", 810)  ("dy:", 414) ("dyexcl:", kMagenta);
    formatters.add<SetLegends>("ttbar:", "t#bar{t}+Jets", "$\\ttbar$") ("dy:", "Z/#gamma*+Jets (incl)", "$Z/\\gamma^*$+Jets") ("data:", "Data") ("dyexcl:", "Z/#gamma*+Jets (excl)")("dymix:", "Z/#gamma*+Jets");
    formatters.add<SetOption>
       ("*", "title_ur", "19.7 fb^{-1}, #sqrt{s}=8TeV")
       ("*", "draw_ratio", "1")
       ("*", "ytext", "events")
       ("mll", "xtext", "m_{ll} [GeV]")
       ("DR_ZB", "xtext", "#Delta R(Z,B)")
       ("DR_BB", "xtext", "#Delta R(B,B)")
       ("DR_VV", "xtext", "#Delta R(V,V)")
       ("A_ZBB", "xtext", "A_{ZBB}")
       ("met", "xtext", "E_{T}^{miss} [GeV]")
       ("m_BB", "xtext", "m_{BB} [GeV]")
       ("Beta", "xtext", "#eta_B")
       ("Bpt", "xtext", "p_{T,B} [GeV]")
       ("min_DR_ZB", "xtext", "min #Delta R(Z,B)")
       ("DPhi_BB", "xtext", "#Delta #phi(B,B)");
    formatters.add<RebinFactor>("mll", 4)("ptz", 4)("Bpt",4)("Beta",4)("DPhi_BB", 2);
    //formatters.add<Ignore>("")("all/mc_n_me_finalstate", false);
    //formatters.add<Scale>("dyexcl:", 1.197);
    
    Plotter p("eps", {ttbar, dy, data}, formatters);
    //Plotter p("eps", {data, data_old}, formatters);
    
    p.stackplots({"dy", "ttbar"});
    p.cutflow("cutflow", "cutflow.tex");
    
    //p.shapeplots({"dy", "dyexcl"});
    
    //Plotter p("eps", {dy, dyexcl}, formatters);
    //p.stackplots({});
}
