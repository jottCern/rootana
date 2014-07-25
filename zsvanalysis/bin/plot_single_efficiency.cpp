#include "TROOT.h"
#include "plot/include/utils.hpp"

#include <iostream>

using namespace std;
using namespace ra;


namespace {
string outputdir = "eps/";
string inputdir = "/nfs/dust/cms/user/ottjoc/omega-out/single_ivf_efficiency/";
const double pt_bins[] = {0., 8., 10., 12., 14., 16., 18., 20., 22., 24., 26., 28., 30., 32., 34., 36., 38., 40., 42., 44., 46., 48., 50., 52., 54., 56., 58., 60., 62., 64., 66., 68., 70., 72., 74., 76., 78., 80., 84., 88., 92., 96., 100., 104., 108., 112., 116., 120., 130., 140., 160., 200.};
}

void plot_eff(const vector<shared_ptr<ProcessHistograms> > & histos, const string & numerator, const string & denom, const Formatters & format){
    Histogram num = histos[0]->get_histogram(numerator);
    Histogram den = histos[0]->get_histogram(denom);
    std::string plot_name = "_" + histos[0]->id().name() + "_";
    /*for (unsigned int i = 1; i < histos.size(); ++i) {
        plot_name += histos[i]->id().name() + "_";
        const auto & n = histos[i]->get_histogram(numerator);
        const auto & d = histos[i]->get_histogram(denom);
        num.histo->Add(n.histo.get());
        den.histo->Add(d.histo.get());
    }*/
    format(num);
    format(den);
    num.histo->Divide(den.histo.get());
//     num.options["draw_ratio"] = "";
    num.options["use_errors"] = "true";
    num.options["ytext"] = "#epsilon";
    vector<Histogram> eff_hist;
    eff_hist.emplace_back(move(num));
    string outfilename = outputdir + "/" + numerator + plot_name + "eff.pdf";
    create_dir(outfilename);
    draw_histos(eff_hist, outfilename);
}


int main(){
    shared_ptr<ProcessHistograms> ttbar(new ProcessHistogramsTFile(inputdir+"ttbar.root", "ttbar"));
    shared_ptr<ProcessHistograms> dy(new ProcessHistogramsTFile({inputdir+"dy1jets.root", inputdir+"dy2jets.root", inputdir+"dy3jets.root", inputdir+"dy4jets.root", inputdir+"dy.root"}, "dy"));
    
    Formatters formatters;
    formatters.add("*", SetLineColor(1));
    formatters.add<SetFillColor>("ttbar:", 810) ("dy:", kBlue);
    formatters.add<SetLineColor>("ttbar:", 810) ("dy:", 414) ("dyexcl:", kMagenta);
    formatters.add<SetLegends>("ttbar:", "t#bar{t}+Jets", "$\\ttbar$") ("dy:", "Z/#gamma*+Jets (MG)", "$Z/\\gamma^*$+Jets");
    formatters.add<SetOption>
       /*("*", "draw_ratio", "1")
       ("*", "ratio_ymax", "2")
       ("*", "ratio_ymin", "0.5")*/
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
       ("dR_mc_reco_diff", "draw_ratio", "")
       ("dR_mc_reco_diff", "xtext", "#Delta #Delta R (B_{reco}, B_{true})")
       ("mcb_bcand_pt_ratio", "xtext", "p_{T}(B_{reco})/p_{T}(B_{true})")
       ("mcbs_pt", "xtext", "p_{T}(B_{true})")
       ("mcbs_eta", "xtext", "#eta(B_{true})")
       ("mcb_bcand_dr", "xtext", "#Delta R(B_{true}, B_{reco})")
       ("mcb_bcand_dr", "xmin", "0.0")
       ("mcb_bcand_dr", "xmax", "0.1")
       ("ptz", "xtext", "p_{T}(ll)")
       ;
    //formatters.add<RebinFactor>("mll", 4)("ptz", 4)("Bpt",4)("Beta",4)("DPhi_BB", 2)("DR_BB", 2)("m_BB", 4)("m_all", 16) ("met", 4) ("ptz", 2);
    //formatters.add<SetOption>("matched*", "draw_ratio", "") ("mcb*", "draw_ratio", "") ("double*", "draw_ratio", "");
    
    /*Formatters format_eff;
    format_eff.add<RebinVariable> ("matched_mcb_pt", 52, pt_bins) ("mcbs_pt", 52, pt_bins) ("double_matchedb_lower_pt", 52, pt_bins) ("double_matchedb_higher_pt", 52, pt_bins) ("double_mcb_lower_pt", 52, pt_bins) ("double_mcb_higher_pt", 52, pt_bins);
    format_eff.add<SetOption>
        ("*", "title_ur", "") ("matched_mcb_pt", "xtext", "p_{T}(Gen-B)") ("matched_mcb_eta", "xtext", "#eta(Gen-B)")
        ("double_matchedb_lower_pt", "xtext", "lower p_{T}(Gen-B)") ("double_matchedb_higher_pt", "xtext", "higher p_{T}(Gen-B)")
        ("double_matchedb_dPhi", "xtext", "#Delta #phi(Gen-(B,B))") ("double_matchedb_dR", "xtext", "#Delta R(Gen-(B,B))");
    format_eff.add<SetOption> ("*", "draw_ratio", "");
    format_eff.add<SetLegends>("*", "Efficiency, Gen-B p_{T} > 30");
    format_eff.add<RebinFactor>("double_matchedb_dR", 2) ("double_mcb_dR", 2);
//     format_eff.add<SetOption>
//        ("matched_bcands_pt", "xmin", "0.")
//        ("matched_bcands_pt", "xmax", "200.")
//        ("mcbs_pt", "xmin", "0.")
//        ("mcbs_pt", "xmax", "200.");
    
    plot_eff({ttbar}, "all/matched_mcb_pt", "all/mcb_pt", format_eff);
    
    plot_eff({ttbar, dy}, "presel/matched_bcands_pt", "presel/mcbs_pt", format_eff);
    plot_eff({ttbar, dy}, "presel/matched_bcands_eta", "presel/mcbs_eta", format_eff);
    plot_eff({ttbar, dy}, "presel/double_matchedb_lower_pt", "presel/double_mcb_lower_pt", format_eff);
    plot_eff({ttbar, dy}, "presel/double_matchedb_higher_pt", "presel/double_mcb_higher_pt", format_eff);
    plot_eff({ttbar, dy}, "presel/double_matchedb_dPhi", "presel/double_mcb_dPhi", format_eff);
    plot_eff({ttbar, dy}, "presel/double_matchedb_dR", "presel/double_mcb_dR", format_eff);
    plot_eff({ttbar}, "presel/double_matchedb_lower_pt", "presel/double_mcb_lower_pt", format_eff);
    plot_eff({ttbar}, "presel/double_matchedb_higher_pt", "presel/double_mcb_higher_pt", format_eff);
    plot_eff({ttbar}, "presel/double_matchedb_dPhi", "presel/double_mcb_dPhi", format_eff);
    plot_eff({ttbar}, "presel/double_matchedb_dR", "presel/double_mcb_dR", format_eff);
    plot_eff({dy}, "presel/double_matchedb_lower_pt", "presel/double_mcb_lower_pt", format_eff);
    plot_eff({dy}, "presel/double_matchedb_higher_pt", "presel/double_mcb_higher_pt", format_eff);
    plot_eff({dy}, "presel/double_matchedb_dPhi", "presel/double_mcb_dPhi", format_eff);
    plot_eff({dy}, "presel/double_matchedb_dR", "presel/double_mcb_dR", format_eff);
    
    plot_eff({ttbar, dy}, "no_bcand_rest/matched_bcands_pt", "no_bcand_rest/mcbs_pt", format_eff);
    plot_eff({ttbar, dy}, "no_bcand_rest/matched_bcands_eta", "no_bcand_rest/mcbs_eta", format_eff);
    plot_eff({ttbar, dy}, "no_bcand_rest/double_matchedb_lower_pt", "no_bcand_rest/double_mcb_lower_pt", format_eff);
    plot_eff({ttbar, dy}, "no_bcand_rest/double_matchedb_higher_pt", "no_bcand_rest/double_mcb_higher_pt", format_eff);
    plot_eff({ttbar, dy}, "no_bcand_rest/double_matchedb_dPhi", "no_bcand_rest/double_mcb_dPhi", format_eff);
    plot_eff({ttbar, dy}, "no_bcand_rest/double_matchedb_dR", "no_bcand_rest/double_mcb_dR", format_eff);
    
    plot_eff({ttbar, dy}, "one_bcand_bef_mll/matched_bcands_pt", "one_bcand_bef_mll/mcbs_pt", format_eff);
    plot_eff({ttbar, dy}, "one_bcand_bef_mll/matched_bcands_eta", "one_bcand_bef_mll/mcbs_eta", format_eff);
    plot_eff({ttbar, dy}, "one_bcand_bef_mll/double_matchedb_lower_pt", "one_bcand_bef_mll/double_mcb_lower_pt", format_eff);
    plot_eff({ttbar, dy}, "one_bcand_bef_mll/double_matchedb_higher_pt", "one_bcand_bef_mll/double_mcb_higher_pt", format_eff);
    plot_eff({ttbar, dy}, "one_bcand_bef_mll/double_matchedb_dPhi", "one_bcand_bef_mll/double_mcb_dPhi", format_eff);
    plot_eff({ttbar, dy}, "one_bcand_bef_mll/double_matchedb_dR", "one_bcand_bef_mll/double_mcb_dR", format_eff);*/
    
    // scale final selection by 0.92**2 (IVF data/MC eficiency factor):
//     formatters.add<Scale>("dy:final/", 0.92 * 0.92) ("dy:final_mllinvert_metinvert/", 0.92 * 0.92);
//     formatters.add<Scale>("ttbar:final/", 0.92 * 0.92) ("ttbar:final_mllinvert_metinvert/", 0.92 * 0.92);
    
    
    Plotter p(outputdir, {ttbar}, formatters);
     
    p.shapeplots({"ttbar"});
    //p.stackplots({});
    //p.stackplots({"dy", "ttbar"});
    //p.cutflow("cutflow", "cutflow.tex");
    
    //p.shapeplots({"dy", "dyexcl"});
    
    //Plotter p("eps", {dy, dyexcl}, formatters);
    //p.stackplots({});
}
