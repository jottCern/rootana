#include "TROOT.h"
#include "plot/include/utils.hpp"

#include <iostream>

using namespace std;
using namespace ra;

namespace {
string outputdir = "plot_single_efficiency_out/";
string inputdir = "/nfs/dust/cms/user/ottjoc/omega-out/ntuple-latest/single_ivf_efficiency/";
const double pt_bins[] = {0., 8., 10., 12., 14., 16., 18., 20., 22., 24., 26., 28., 30., 32., 34., 36., 38., 40., 42., 44., 46., 48., 50., 52., 54., 56., 58., 60., 62., 64., 66., 68., 70., 72., 74., 76., 78., 80., 84., 88., 92., 96., 100., 104., 108., 112., 116., 120., 130., 140., 160., 200.};
}

Histogram get_eff_hist(const shared_ptr<ProcessHistograms> & ph, const string & selection, const string & num, const string & denom, const Formatters & format){
    Histogram hnum = ph->get_histogram(selection, num);
    Histogram hden = ph->get_histogram(selection, denom);
    format(hnum);
    format(hden);
    hnum.histo->Divide(hden.histo.get());
    return move(hnum);
}

void compare_eff_processes(const vector<shared_ptr<ProcessHistograms> > & phs, const string & selection, const string & num, const string & denom, const Formatters & format, const string & name){
    vector<Histogram> histos;
    for(const auto & ph : phs){
        histos.emplace_back(get_eff_hist(ph, selection, num, denom, format));
    }
    string outfilename = outputdir + name + histos[0].hname + "_effp.pdf";
    draw_histos(histos, outfilename);
}

int main(){
    shared_ptr<ProcessHistograms> ttbar(new ProcessHistogramsTFile(inputdir+"ttbar.root", "ttbar"));
    shared_ptr<ProcessHistograms> dy(new ProcessHistogramsTFile({inputdir+"dy1jets.root", inputdir+"dy2jets.root", inputdir+"dy3jets.root", inputdir+"dy4jets.root", inputdir+"dy.root"}, "dy"));
    
    Formatters formatters_x;
    formatters_x.add<SetOption>
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
       ("dR_mc_reco_diff", "draw_ratio", "")
       ("dR_mc_reco_diff", "xtext", "#Delta #Delta R (B_{reco}, B_{true})")
       ("mcb_bcand_pt_ratio", "xtext", "p_{T}(B_{reco})/p_{T}(B_{true})")
       ("mcbs_pt", "xtext", "p_{T}(B_{true}) [GeV]")
       ("mcbs_eta", "xtext", "#eta(B_{true}) [GeV]")
       ("mcb_bcand_dr", "xtext", "#Delta R(B_{true}, B_{reco})")
       ("matched_mcb_pt", "xtext", "p_{T}(B_{true}) [GeV]")
       ("mcb_bcand_d*", "xmin", "0.0")
       ("mcb_bcand_dr", "xmax", "0.1")
       ("mcb_bcand_dphi", "xtext", "#Delta #phi(B_{true}, B_{reco})")
       ("mcb_bcand_dphi", "xmax", "0.05")
       ("mcb_bcand_deta", "xtext", "#Delta #eta(B_{true}, B_{reco})")
       ("mcb_bcand_deta", "xmax", "0.05")
       ("ptz", "xtext", "p_{T}(ll) [GeV]");
    
       
    // 1. shape plots for all histograms, comparing processes:
    Formatters formatters(formatters_x);
    //formatters.add("*", SetLineColor(1));
    //formatters.add<SetFillColor>("ttbar:", 810) ("dy:", kBlue);
    formatters.add<SetLineColor>("ttbar:", 810) ("dy:", 414) ("dyexcl:", kMagenta);
    formatters.add<SetLineWidth>("*", 2);
    formatters.add<SetLegends>("ttbar:", "t#bar{t}+Jets", "$\\ttbar$") ("dy:", "Z/#gamma*+Jets (MG)", "$Z/\\gamma^*$+Jets");
    formatters.add<SetOption>("*", "ytext", "events");
    Plotter p1(outputdir, {ttbar, dy}, formatters);
    p1.shapeplots({"ttbar", "dy"});
    
    
    // 2. efficiency comparisons between different processes:
    Formatters format_eff(formatters_x);
    format_eff.add<SetLineColor>("ttbar:", 810) ("dy:", 414) ("dyexcl:", kMagenta);
    format_eff.add<SetLegends>("ttbar:", "t#bar{t}+Jets", "$\\ttbar$") ("dy:", "Z/#gamma*+Jets (MG)", "$Z/\\gamma^*$+Jets");
    format_eff.add<RebinVariable> ("matched_mcb_pt", 52, pt_bins) ("mcbs_pt", 52, pt_bins) ("double_matchedb_lower_pt", 52, pt_bins) ("double_matchedb_higher_pt", 52, pt_bins) ("double_mcb_lower_pt", 52, pt_bins) ("double_mcb_higher_pt", 52, pt_bins);
    format_eff.add<SetOption>("*", "ytext", "efficiency") ("*", "use_errors", "1");
    //format_eff.add<SetFillColor>("*", 0);
    
    compare_eff_processes({ttbar, dy}, "all", "matched_mcb_pt", "mcbs_pt", format_eff, "all/");
    compare_eff_processes({ttbar, dy}, "mcb_forward", "matched_mcb_pt", "mcbs_pt", format_eff, "mcb_forward/");
    compare_eff_processes({ttbar, dy}, "mcb_central", "matched_mcb_pt", "mcbs_pt", format_eff, "mcb_central/");
    
    compare_eff_processes({ttbar, dy}, "mcb_pt20", "matched_mcb_eta", "mcbs_eta", format_eff, "mcb_pt20/");
    
    // 3. compare different selections for same process:
    Formatters formatters_selcomp(formatters_x);
    formatters_selcomp.add<SetLineWidth>("*", 2);
    formatters_selcomp.add<SetLineColor>(":all/", 810) (":mcb_pt10/", 414) (":mcb_pt20/", kMagenta) (":mcb_central/", kBlue)(":mcb_forward/", kGreen);
    formatters_selcomp.add<SetLegends>(":mcb_pt20/", "p_{T}(B) > 20 GeV")(":mcb_pt10/", "p_{T}(B) > 10 GeV")(":all/", "all") (":mcb_central/", "|#eta(B)| < 1")
        (":mcb_forward/", "|#eta(B)| > 1");
        
    Plotter p2(outputdir, {ttbar, dy}, formatters_selcomp);
    p2.selcomp_plots({"all", "mcb_pt10", "mcb_pt20"}, {"mcbs_pt", "mcbs_eta"}, "selcomp");
    p2.selcomp_plots({"mcb_central", "mcb_forward"}, {"mcb_bcand_deta", "mcb_bcand_dphi"}, "selcomp_eta");
    p2.selcomp_plots({"all", "mcb_pt10", "mcb_pt20"}, {"mcb_bcand_deta", "mcb_bcand_dphi"}, "selcomp_pt");
}

