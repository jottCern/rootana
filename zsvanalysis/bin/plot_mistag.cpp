#include "TROOT.h"
#include "plot/include/utils.hpp"

#include <iostream>

using namespace std;
using namespace ra;

namespace {
string outputdir = "plot_mistag_out/";
string inputdir = "/nfs/dust/cms/user/ottjoc/omega-out/mistag/";
}

Histogram get_eff_hist(const shared_ptr<ProcessHistograms> & ph, const string & num, const string & denom, const Formatters & format){
    Histogram hnum = ph->get_histogram(num);
    Histogram hden = ph->get_histogram(denom);
    format(hnum);
    format(hden);
    hnum.histo->Divide(hden.histo.get());
    return move(hnum);
}

void compare_eff_processes(const vector<shared_ptr<ProcessHistograms> > & phs, const string & num, const string & denom, const Formatters & format, const string & name){
    vector<Histogram> histos;
    for(const auto & ph : phs){
        histos.emplace_back(get_eff_hist(ph, num, denom, format));
    }
    string outfilename = outputdir + name + histos[0].hname.name() + "_effp.pdf";
    draw_histos(histos, outfilename);
}


int main(){
    shared_ptr<ProcessHistograms> ttbar(new ProcessHistogramsTFile(inputdir+"ttbar.root", "ttbar"));
    shared_ptr<ProcessHistograms> dy(new ProcessHistogramsTFile({inputdir+"dy1jets.root", inputdir+"dy2jets.root", inputdir+"dy3jets.root", inputdir+"dy4jets.root", inputdir+"dy.root"}, "dy"));
    
    Formatters formatters_x;
    formatters_x.add<SetOption>
       ("*tag_bpt_over_jetpt", "xtext", "p_{T}(B_{reco}) / p_{T}(jet)")
       ("*tag_jetpt", "xtext", "p_{T}(jet) [GeV]")
       ("*tag_matched_dr", "xtext", "#Delta R(B_{reco}, jet)");
    
       
    // 1. shape plots for all histograms, comparing processes:
    Formatters formatters(formatters_x);
    //formatters.add("*", SetLineColor(1));
    //formatters.add<SetFillColor>("ttbar:", 810) ("dy:", kBlue);
    formatters.add<SetLineColor>("ttbar:", 810) ("dy:", 414);
    formatters.add<SetLineWidth>("*", 2);
    formatters.add<SetLegends>("ttbar:", "t#bar{t}+Jets", "$\\ttbar$") ("dy:", "Z/#gamma*+Jets (MG)", "$Z/\\gamma^*$+Jets");
    formatters.add<SetOption>("*", "ytext", "events");
    
    Plotter p1(outputdir, {ttbar, dy}, formatters);
    p1.shapeplots({"ttbar", "dy"});
    
    
    // 2. some efficiency comparisons
    Formatters format_eff(formatters_x);
    format_eff.add<SetLineColor>("ttbar:", 810) ("dy:", 414) ("dyexcl:", kMagenta);
    format_eff.add<SetLegends>("ttbar:", "t#bar{t}+Jets", "$\\ttbar$") ("dy:", "Z/#gamma*+Jets (MG)", "$Z/\\gamma^*$+Jets");
    format_eff.add<SetOption>("*", "ytext", "efficiency") ("*", "use_errors", "1");
    //format_eff.add<SetFillColor>("*", 0);
    
    compare_eff_processes({ttbar, dy}, "all/jetpt_l_tagged", "all/jetpt_l", format_eff, "all/");
    compare_eff_processes({ttbar, dy}, "all/jetpt_c_tagged", "all/jetpt_c", format_eff, "all/");
    compare_eff_processes({ttbar, dy}, "all/jetpt_b_tagged", "all/jetpt_b", format_eff, "all/");
    
    compare_eff_processes({ttbar, dy}, "all/jeteta_l_tagged", "all/jeteta_l", format_eff, "all/");
    compare_eff_processes({ttbar, dy}, "all/jeteta_c_tagged", "all/jeteta_c", format_eff, "all/");
    compare_eff_processes({ttbar, dy}, "all/jeteta_b_tagged", "all/jeteta_b", format_eff, "all/");
    
    /*// 3. compare different selections for same process:
    Formatters formatters_selcomp(formatters_x);
    formatters_selcomp.add<SetLineWidth>("*", 2);
    formatters_selcomp.add<SetLineColor>(":all/", 810) (":mcb_pt10/", 414) (":mcb_pt20/", kMagenta) (":mcb_central/", kBlue)(":mcb_forward/", kGreen);
    formatters_selcomp.add<SetLegends>(":mcb_pt20/", "p_{T}(B) > 20 GeV")(":mcb_pt10/", "p_{T}(B) > 10 GeV")(":all/", "all") (":mcb_central/", "|#eta(B)| < 1")
        (":mcb_forward/", "|#eta(B)| > 1");
        
    Plotter p2(outputdir, {ttbar, dy}, formatters_selcomp);
    p2.selcomp_plots({"all", "mcb_pt10", "mcb_pt20"}, {"mcbs_pt", "mcbs_eta"}, "selcomp");
    p2.selcomp_plots({"mcb_central", "mcb_forward"}, {"mcb_bcand_deta", "mcb_bcand_dphi"}, "selcomp_eta");
    p2.selcomp_plots({"all", "mcb_pt10", "mcb_pt20"}, {"mcb_bcand_deta", "mcb_bcand_dphi"}, "selcomp_pt");*/
}

