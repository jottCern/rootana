#include "TROOT.h"
#include "utils.hpp"

#include <iostream>

using namespace std;
using namespace ra;

int main(){
    shared_ptr<ProcessHistograms> ttbar(new ProcessHistogramsTFile("../ttbar.root", "ttbar"));
    shared_ptr<ProcessHistograms> dy(new ProcessHistogramsTFile("../dy.root", "dy"));
    shared_ptr<ProcessHistograms> data(new ProcessHistogramsTFile("../dmu.root", "data"));
    
    Formatters formatters;
    formatters.add("*", SetLineColor(1));
    formatters.add<SetFillColor>("ttbar:", 810)  ("dy:", 414);
    formatters.add<SetLegends>("ttbar:", "t#bar{t}+Jets", "$\\ttbar$") ("dy:", "Z/#gamma*+Jets", "$Z/\\gamma^*$+Jets") ("data:", "Data");
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
    formatters.add<RebinFactor>("mll", 4);
    formatters.add<Ignore>("bcand2_mll/");
    
    Plotter p("eps", {ttbar, dy, data}, formatters);
    
    //std::map<identifier, double> factors;
    //factors["ttbar"] = 1.1;
    //factors["dy"] = 1.1;
    //shared_ptr<Formatter> apply_factors(new Factors(scales));
    
    //std::map<std::string, tuple<int, double, double> > binning;
    //binning["mll"] = make_tuple(30, 60, 120);
    //shared_ptr<Formatter> apply_binning(new Binning(binning));
    
    //std::map<identifier, std::string> titles = {{"ttbar", "TTbar"}, {"dy", "DY+Jets"}, {}};
    
    p.stackplots({"dy", "ttbar"});
    //p.cutflow("cutflow", "cutflow.tex");
}
