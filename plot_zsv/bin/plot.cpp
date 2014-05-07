#include "TROOT.h"
#include "plot/include/utils.hpp"

#include <iostream>

using namespace std;
using namespace ra;


namespace {
string outputdir = "eps/full_more_eff0";
string inputdir = "../zsvanalysis/rootfiles/full_more_eff/";
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
    shared_ptr<ProcessHistograms> ttbar(new ProcessHistogramsTFile(inputdir+"ttbar.root", "ttbar"));
    shared_ptr<ProcessHistograms> dy(new ProcessHistogramsTFile({inputdir+"dy1jets.root", inputdir+"dy2jets.root", inputdir+"dy3jets.root", inputdir+"dy4jets.root", inputdir+"dy.root"}, "dy"));
    shared_ptr<ProcessHistograms> data(new ProcessHistogramsTFile({inputdir+"dmu_runa.root", inputdir+"dmu_runb.root", inputdir+"dmu_runc.root", inputdir+"dmu_rund.root"}, "data"));
    
    Formatters formatters;
    formatters.add("*", SetLineColor(1));
    formatters.add<SetFillColor>("ttbar:", 810)  ("dymix:", 414) ("dyexcl:", kMagenta) ("dy:", kBlue);
    //formatters.add<SetLineColor>("ttbar:", 810)  ("dy:", 414) ("dyexcl:", kMagenta);
    formatters.add<SetLegends>("ttbar:", "t#bar{t}+Jets", "$\\ttbar$") ("dy:", "Z/#gamma*+Jets (MG)", "$Z/\\gamma^*$+Jets") ("data:", "Data") ("dyexcl:", "Z/#gamma*+Jets (excl)")("dymix:", "Z/#gamma*+Jets");
    formatters.add<SetOption>
       ("*", "title_ur", "19.7 fb^{-1}, #sqrt{s}=8TeV")
       ("*", "draw_ratio", "1")
       ("*", "ratio_ymax", "2")
       ("*", "ratio_ymin", "0.5")
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
       ("DPhi_BB", "xtext", "#Delta #phi(B,B)")
       ("DR_BB", "ratio_ymin", "0.5")
       ("DR_BB", "ratio_ymax", "1.5")
       ;
    formatters.add<RebinFactor>("mll", 4)("ptz", 4)("Bpt",4)("Beta",4)("DPhi_BB", 2)("DR_BB", 2)("m_BB", 4);

    
    plot_eff(*ttbar, "presel/matched_bcands_pt", "presel/mcbs_pt", formatters);
    plot_eff(*ttbar, "presel/matched_bcands_eta", "presel/mcbs_eta", formatters);
    plot_eff(*ttbar, "presel/double_matchedb_lower_pt", "presel/double_mcb_lower_pt", formatters);
    plot_eff(*ttbar, "presel/double_matchedb_higher_pt", "presel/double_mcb_higher_pt", formatters);
    plot_eff(*ttbar, "presel/double_matchedb_dPhi", "presel/double_mcb_dPhi", formatters);
    plot_eff(*ttbar, "presel/double_matchedb_dR", "presel/double_mcb_dR", formatters);
    plot_eff(*dy, "presel/matched_bcands_pt", "presel/mcbs_pt", formatters);
    plot_eff(*dy, "presel/matched_bcands_eta", "presel/mcbs_eta", formatters);
    plot_eff(*dy, "presel/double_matchedb_lower_pt", "presel/double_mcb_lower_pt", formatters);
    plot_eff(*dy, "presel/double_matchedb_higher_pt", "presel/double_mcb_higher_pt", formatters);
    plot_eff(*dy, "presel/double_matchedb_dPhi", "presel/double_mcb_dPhi", formatters);
    plot_eff(*dy, "presel/double_matchedb_dR", "presel/double_mcb_dR", formatters);
    
    plot_eff(*ttbar, "final/matched_bcands_pt", "final/mcbs_pt", formatters);
    plot_eff(*ttbar, "final/matched_bcands_eta", "final/mcbs_eta", formatters);
    plot_eff(*ttbar, "final/double_matchedb_lower_pt", "final/double_mcb_lower_pt", formatters);
    plot_eff(*ttbar, "final/double_matchedb_higher_pt", "final/double_mcb_higher_pt", formatters);
    plot_eff(*ttbar, "final/double_matchedb_dPhi", "final/double_mcb_dPhi", formatters);
    plot_eff(*ttbar, "final/double_matchedb_dR", "final/double_mcb_dR", formatters);
    plot_eff(*dy, "final/matched_bcands_pt", "final/mcbs_pt", formatters);
    plot_eff(*dy, "final/matched_bcands_eta", "final/mcbs_eta", formatters);
    plot_eff(*dy, "final/double_matchedb_lower_pt", "final/double_mcb_lower_pt", formatters);
    plot_eff(*dy, "final/double_matchedb_higher_pt", "final/double_mcb_higher_pt", formatters);
    plot_eff(*dy, "final/double_matchedb_dPhi", "final/double_mcb_dPhi", formatters);
    plot_eff(*dy, "final/double_matchedb_dR", "final/double_mcb_dR", formatters);
    
    // scale final selection by 0.92**2 (IVF data/MC eficiency factor):
    formatters.add<Scale>("dy:final/", 0.92 * 0.92);
    formatters.add<Scale>("ttbar:final/", 0.92 * 0.92);
    
     Plotter p(outputdir, {ttbar, dy, data}, formatters);
     
     p.stackplots({"dy", "ttbar"});
     p.cutflow("cutflow", "cutflow.tex");
    
    //p.shapeplots({"dy", "dyexcl"});
    
    //Plotter p("eps", {dy, dyexcl}, formatters);
    //p.stackplots({});
}
