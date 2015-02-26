#include "plot/include/utils.hpp"

#include "TH2D.h"
#include "TGraphErrors.h"
#include "TCanvas.h"

#include <iostream>

using namespace std;
using namespace ra;


namespace {
string outputdir = "plot_reco_out/";
}

void print_sfs(const shared_ptr<ProcessHistograms> & ph, const string & selection){
    auto h = ph->get_histogram(selection, "musf");
    cout << "musf: " << h.histo->GetMean() << endl;
    h = ph->get_histogram(selection, "elesf");
    cout << "elesf: " << h.histo->GetMean() << endl;
    h = ph->get_histogram(selection, "pileupsf");
    cout << "pileup: " << h.histo->GetMean() << endl;
}

void print_trigger_effs(const shared_ptr<ProcessHistograms> & ph, const string & selection){
    auto h = ph->get_histogram(selection, "lepton_trigger");
    cout << "lepton trigger efficiency (w.r.t. offline lepton selection): " << h.histo->GetBinContent(2) /  (h.histo->GetBinContent(1) + h.histo->GetBinContent(2)) << endl;
    h = ph->get_histogram(selection, "lepton_triggermatch");
    cout << "lepton trigger matching efficiency (after offline selection and trigger): " << h.histo->GetBinContent(2) /  (h.histo->GetBinContent(1) + h.histo->GetBinContent(2))<< endl;
}

void dump_histo(const shared_ptr<ProcessHistograms> & ph, const string & selection, const string & hname){
    auto h = ph->get_histogram(selection, hname);
    for(int i=1; i<=h.histo->GetNbinsX(); ++i){
        cout << i << " (" << h.histo->GetXaxis()->GetBinLowEdge(i) << "--" << h.histo->GetXaxis()->GetBinUpEdge(i) << ")" << h.histo->GetBinContent(i) << endl;
    }
}


void compare_meanpt_vs_dr(const vector<shared_ptr<ProcessHistograms>> & phs, const string & sel, const string & outname_prefix, const Formatters & f){
    vector<unique_ptr<TH2D>> hists;
    size_t idata = -1;
    for(const auto & ph : phs){
        auto h = ph->get_histogram(sel, "bcand1pt_drbb");
        f(h);
        hists.emplace_back(dynamic_cast<TH2D*>((TH2D*)h.histo->Clone())); // x = DR, y = pt
        if(h.process == "data"){
            idata = hists.size() - 1;
        }
    }
    
    if(idata >= hists.size()){
        throw runtime_error("compare_meanpt_vs_dr: did not find data histogram");
    }
    
    // for each x bin, calculate mean in y direction and compare data vs. MC
    vector<double> x, ydata, yedata, ymc, yemc;
    for(int ix=1; ix<=hists[0]->GetNbinsX(); ++ix){
        unique_ptr<TH1D> mcs, data;
        for(size_t i=0; i<hists.size(); ++i){
            TH1D * proj = hists[i]->ProjectionY("_py", ix, ix);
            if(i == idata){
                data.reset(proj);
            }
            else{
                if(!mcs){
                    mcs.reset(proj);
                }
                else{
                    mcs->Add(proj);
                    delete proj;
                }
            }
        }
        x.push_back(hists[0]->GetXaxis()->GetBinCenter(ix));
        ydata.push_back(data->GetMean());
        yedata.push_back(data->GetMeanError());
        ymc.push_back(mcs->GetMean());
        yemc.push_back(mcs->GetMeanError());
    }
    
    unique_ptr<TGraph> g_data(new TGraphErrors(x.size(), &x[0], &ydata[0], 0, &yedata[0]));
    unique_ptr<TGraph> g_mc(new TGraphErrors(x.size(), &x[0], &ymc[0], 0, &yemc[0]));
    
    g_mc->SetLineColor(kBlue);
    
    TCanvas * c = new TCanvas("c", "c", 600, 600);
    g_data->Draw("AL");
    g_mc->Draw("L");
    c->Print((outname_prefix + sel + "_bpt1.pdf").c_str());
    delete c;
}



int main(){
    const string inputdir = "/nfs/dust/cms/user/ottjoc/omega-out/ntuplev7/recoplots/";
    //const string inputdir = "/nfs/dust/cms/user/ottjoc/omega-out/ntuple-latest/debug_me/";
    
    //shared_ptr<ProcessHistograms> dy_all(new ProcessHistogramsTFile(inputdir + "dy[0-4]jets*.root", "dy_all"));
    
    shared_ptr<ProcessHistograms> dybbvis(new ProcessHistogramsTFile(inputdir + "dy{[0-4]jets,}bbvis.root", "dybbvis"));
    shared_ptr<ProcessHistograms> dybbother(new ProcessHistogramsTFile(inputdir + "dy{[0-4]jets,}bbother.root", "dybbother"));
    shared_ptr<ProcessHistograms> dybbany(new ProcessHistogramsTFile(inputdir + "dy{[0-4]jets,}bb{vis,other}.root", "dybbany"));
    shared_ptr<ProcessHistograms> dycc(new ProcessHistogramsTFile(inputdir + "dy{[0-4]jets,}cc.root", "dycc"));
    shared_ptr<ProcessHistograms> dylight(new ProcessHistogramsTFile(inputdir + "dy{[0-4]jets,}other.root", "dylight"));
    
    shared_ptr<ProcessHistograms> dybb4f_bbvis(new ProcessHistogramsTFile(inputdir + "dybb4fbbvis.root", "dybb4f_bbvis"));
    shared_ptr<ProcessHistograms> dybbm_bbvis(new ProcessHistogramsTFile(inputdir + "dybbmbbvis.root", "dybbm_bbvis"));
    
    shared_ptr<ProcessHistograms> dybb4f_bbother(new ProcessHistogramsTFile(inputdir + "dybb4fbbother.root", "dybb4f_bbother"));
    shared_ptr<ProcessHistograms> dybbm_bbother(new ProcessHistogramsTFile(inputdir + "dybbmbbother.root", "dybbm_bbother"));
    
    //shared_ptr<ProcessHistograms> dybb4f_bbany(new ProcessHistogramsTFile(inputdir + "dybb4f-zbb*.root", "dybb4f_bbany"));
    //shared_ptr<ProcessHistograms> dybbm_bbany(new ProcessHistogramsTFile(inputdir + "dybbm-zbb*.root", "dybbm_bbany"));
    
    //shared_ptr<ProcessHistograms> top(new ProcessHistogramsTFile({inputdir + "ttbar.root", inputdir + "st*.root"}, "top"));
    
    shared_ptr<ProcessHistograms> ttbar2b(new ProcessHistogramsTFile({inputdir + "ttbar2b.root"}, "ttbar2b"));
    shared_ptr<ProcessHistograms> ttbar4b(new ProcessHistogramsTFile({inputdir + "ttbar4b.root"}, "ttbar4b"));
    shared_ptr<ProcessHistograms> ttbar(new AddProcesses({ttbar2b, ttbar4b}, "ttbar"));
    
    shared_ptr<ProcessHistograms> st(new ProcessHistogramsTFile(inputdir + "st*.root", "st"));
    
    shared_ptr<ProcessHistograms> top(new AddProcesses({ttbar, st}, "top"));
    
    shared_ptr<ProcessHistograms> diboson(new ProcessHistogramsTFile({inputdir + "ww.root", inputdir + "wz.root", inputdir + "zz.root"}, "diboson"));
    
    // diboson separately:
    shared_ptr<ProcessHistograms> ww(new ProcessHistogramsTFile({inputdir + "ww.root"}, "ww"));
    shared_ptr<ProcessHistograms> wz(new ProcessHistogramsTFile({inputdir + "wz.root"}, "wz"));
    shared_ptr<ProcessHistograms> zz(new ProcessHistogramsTFile({inputdir + "zz.root"}, "zz"));
    
    shared_ptr<ProcessHistograms> mm_data(new ProcessHistogramsTFile(inputdir + "dmu*.root", "data"));
    shared_ptr<ProcessHistograms> ee_data(new ProcessHistogramsTFile(inputdir + "dele*.root", "data"));
    shared_ptr<ProcessHistograms> me_data(new ProcessHistogramsTFile(inputdir + "mue*.root", "data"));
    
    /*shared_ptr<ProcessHistograms> me_data_a(new ProcessHistogramsTFile(inputdir + "mue_runa.root", "data"));
    shared_ptr<ProcessHistograms> me_data_b(new ProcessHistogramsTFile(inputdir + "mue_runb.root", "data"));
    shared_ptr<ProcessHistograms> me_data_c(new ProcessHistogramsTFile(inputdir + "mue_runc.root", "data"));
    shared_ptr<ProcessHistograms> me_data_d(new ProcessHistogramsTFile(inputdir + "mue_rund.root", "data"));*/
    
    Formatters formatters_legends;
    
    formatters_legends.add<SetLegends>("top:", "t#bar{t}, tW")
        ("ttbar:", "t#bar{t}")
        ("ttbar2b:", "t#bar{t}")
        ("ttbar4b:", "t#bar{t} + bb")
        ("st:", "tW")
        ("diboson:", "diboson (WW, ZZ, WZ)")
        ("zz:", "ZZ")
        ("wz:", "WZ")
        ("ww:", "WW")
        ("dybbvis:", "Z/#gamma*+bb (5FS, mb=0)")
        ("dybbany:", "Z/#gamma*+bb (any)")
        ("dybbother:", "Z/#gamma*+bb (invis)")
        ("dynonb:", "Z/#gamma*+c/light")
        ("dycc:", "Z/#gamma*+Jets (cc)")
        ("dylight:", "Z/#gamma*+Jets (light)")
        ("data:", "Data")
        ("dybbm_bbvis:", "Z/#gamma*+bb (5FS, mb > 0)")
        ("dybb4f_bbvis:", "Z/#gamma*+bb (4FS, mb > 0)")
        ("dybbm_bbother:", "Z/#gamma*+bb+Jets mb (bb invis)")
        ("dybb4f_bbother:", "Z/#gamma*+bb+Jets 4FS (bb invis)")
        ("dybbm_bbany:", "Z/#gamma*+bb+Jets mb (bb)")
        ("dybb4f_bbany:", "Z/#gamma*+bb+Jets 4FS (bb)")
        ;
    formatters_legends.add<SetOption>
       ("*", "title_ur", "19.7 fb^{-1}, #sqrt{s}=8TeV")
       //(":fs_mm_bc2_mlli/", "title_ul", "N_{B}=2, Z veto")
       (":fs_bc2_mll_met/", "title_ul1", "final (N_{B}=2, Z, MET<50GeV)")
       (":presel_mm/", "title_ul1", "#mu#mu channel")
       (":mm_ee/", "title_ul1", "#mu#mu + ee channels")
       //("*", "draw_ratio", "dybbvis; dybbm_bbvis; dybb4f_bbvis / data")
       ("*", "ratio_ytext", "MC / data")
       ("*", "ratio_denom_error", "data")
       //("*", "draw_ratio", "data / bkg")
       ("*", "ratio_ymax", "2")
       ("*", "ratio_ymin", "0")
       //("*", "legend_fontsize", "0.04")
       ("*", "more_ymax", "0.2")
       //("*", "canvas_width", "1.5")
       ("DR_BB", "more_ymax", "0.25")
       ("*", "ytext", "events")
       //("*", "ylabel_factor", "1.3")
       ("mll", "xtext", "m_{ll} [GeV]")
       ("DR_ZB", "xtext", "#Delta R(Z,B)")
       ("DR_BB", "xtext", "#Delta R(B,B)")
       ("A_ZBB", "xtext", "A_{ZBB}")
       ("met", "xtext", "E_{T}^{miss} [GeV]")
       ("m_BB", "xtext", "m_{BB} [GeV]")
       ("Beta", "xtext", "#eta_{B}")
       ("Bpt", "xtext", "p_{T}(B) [GeV]")
       ("ptz", "xtext", "p_{T}(Z) [GeV]")
       ("min_DR_ZB", "xtext", "min #Delta R(Z,B)")
       ("DPhi_BB", "xtext", "#Delta #phi(B,B)")
       ("nbh", "xtext", "N_{B}")
       ("cutflow*", "ylog", "1")
       ("nbcands", "ylog", "1")
       ("nbcands", "xtext", "N_{B}")
       ("nbcands", "xmin", "0")
       ("nbcands", "xmax", "4")
       //("nbh", "ymax", "300")
       ;
    
    Formatters formatters(formatters_legends);
    formatters.add<SetLineColor>("*", 1)
        ("dybbm_bbvis:", kCyan + 2) ("dybb4f_bbvis:", kYellow + 2) ("dybbvis:", kGray + 2)
        ("dybbm_bbother:", kCyan + 2) ("dybb4f_bbother:", kYellow + 2) ("dybbother:", kGray + 2)
        ("dybbm_bbany:", kCyan + 2) ("dybb4f_bbany:", kYellow + 2) ("dybbany:", kGray + 2);
    formatters.add<SetLineWidth>("dybbm_bbvis:", 2)("dybb4f_bbvis:", 2)("dybbvis:", 2);
    formatters.add<SetFillColor>("top:", kRed)("ttbar:", kRed)
          ("ttbar2b:", kRed) ("ttbar4b:", kYellow)
          ("st:", kRed - 6)("diboson:", kGreen + 3)
          ("dybbother:", kBlue - 7) ("dycc:", kBlue + 1) ("dylight:", kBlue-1)("dynonb:", kBlue-1);
       
       
    formatters.add<RebinFactor>("mll", 4)("ptz", 4)("Bpt",4)("Beta",4)("DPhi_BB", 4)("DR_BB", 8)("m_BB", 4)("bc_pt_over_bj_pt", 5)("met", 4)("Bpt_over_jetpt", 4);
    formatters.add<SetOption>("DR_BB", "xmin", "0")("DR_BB", "xmax", "4");
    
    // add DY+Jets NLO k-factor. NOTE: only for the special Dy+bb samples; factor already applied for includsive madgraph Z+4j sample
    formatters.add<Scale> //("dybbvis:", 1.197)("dybbany:", 1.197)("dybbother:", 1.197)("dycc:", 1.197)("dylight:", 1.197)
        ("dybbm_bbvis:", 1.197)
        ("dybb4f_bbvis:", 1.197)
        ("dybbm_bbany:", 1.197)
        ("dybb4f_bbany:", 1.197);
    
    // scale final selection by IVF data/MC eficiency factor:
    double b2sf = 0.92 * 0.92;
    formatters.add<Scale>(":fs_mm_bc2_mll_met/", b2sf)(":fs_mm_bc2_mll/", b2sf)(":fs_mm_bc2_mlli/", b2sf)
                         (":fs_ee_bc2_mll_met/", b2sf)(":fs_ee_bc2_mll/", b2sf)(":fs_ee_bc2_mlli/", b2sf)
                         (":fs_me_bc2/", b2sf)(":fs_me_bc2_mll/", b2sf)(":fs_me_bc2_mlli/", b2sf)(":fs_me_bc2_mll_met/", b2sf)
                         (":mm_ee/", b2sf);
    
     //Plotter p(outputdir, {top, diboson, dylight, dycc, dybbany, dybbm_bbany, dybb4f_bbany, data}, formatters);
     //p.stackplots({"dycc", "dylight", "diboson", "top"}, "", true);
    
    //dump_histo(ttbar, "presel_me", "nbcands");
    //dump_histo(me_data, "presel_me", "nbcands");
     
    
    /*cout << "ttbar sfs presel me:" << endl;
    print_sfs(ttbar, "presel_me");
    
    cout << "ttbar sfs presel mm:" << endl;
    print_sfs(ttbar, "presel_mm");
    
    cout << "ttbar sfs presel ee:" << endl;
    print_sfs(ttbar, "presel_ee");
    
    
    cout << "ttbar sfs fs_bc2:" << endl;
    print_sfs(ttbar, "fs_me_bc2");
    
    cout << "ttbar sfs presel mm:" << endl;
    print_sfs(ttbar, "fs_mm_bc2_mll");
    
    cout << "ttbar sfs presel ee:" << endl;
    print_sfs(ttbar, "fs_ee_bc2_mll");
    
    cout << endl;
    
    
    cout << "dylight sfs me:" << endl;
    print_sfs(dylight, "presel_me");
    
    cout << "dylight sfs mm:" << endl;
    print_sfs(dylight, "presel_mm");
    
    cout << "dylight sfs ee:" << endl;
    print_sfs(dylight, "presel_ee");
        
    cout << endl;
    
    
    cout << "dylight trigger effs mm:" << endl;
    print_trigger_effs(dylight, "mm");
    
    cout << "dylight trigger effs ee:" << endl;
    print_trigger_effs(dylight, "ee");
    
    cout << "dylight trigger effs me:" << endl;
    print_trigger_effs(dylight, "me");*/
    
    
    
    //return 0;
    
    /*
    // compare different ttbar selections:
    {
        Formatters formatters_selcomp;
        formatters_selcomp.add<SetLineColor>(":presel_mm/", kRed)(":presel_me/", kGreen)(":presel_ee/", kBlue);
        formatters_selcomp.add<SetLegends>(":presel_mm/", "mm")(":presel_ee/", "ee")(":presel_me/", "me")
            (":fs_mm_bc2_mll/", "mm")(":fs_ee_bc2_mll/", "ee")(":fs_me_bc2/", "me");
        Plotter p(outputdir, {ttbar}, formatters_selcomp);
        p.selcomp_plots({"presel_mm", "presel_ee", "presel_me"}, {"mcbs_pt", "ttdec"}, "out-presel");
        p.selcomp_plots({"fs_mm_bc2_mll", "fs_ee_bc2_mll", "fs_me_bc2"}, {"mcbs_pt", "ttdec"}, "out-fs");
    }*/
    
    //return 0;
    
     // me selections:
     {
        Plotter p(outputdir, {ttbar2b, ttbar4b, st, diboson, dylight, dycc, dybbany, me_data}, formatters);
        //Plotter p(outputdir, {ttbar, st, diboson, dylight, dycc, dybbother, dybbvis, dybbm_bbvis, dybb4f_bbvis, me_data}, formatters);
        p.set_histogram_filter(RegexFilter(".*", ".*_me_.*|presel_me", ".*"));
        p.set_option("add_nonstacked_to_stack", "1");
        //p.stackplots({"ttbar", "st", "diboson", "dylight", "dycc", "dybbother" });
        p.stackplots({"ttbar2b", "ttbar4b", "st", "diboson", "dylight", "dycc", "dybbany" });
     }
     
     
     // mm selection:
     /*{
        Plotter p(outputdir, {ttbar, st, diboson, dylight, dycc, dybbother, dybbvis, dybbm_bbvis, dybb4f_bbvis, mm_data}, formatters);
        p.set_histogram_filter(RegexFilter(".*", "fs_mm_.*|presel_mm", ".*"));
        p.set_option("add_nonstacked_to_stack", "1");
        p.stackplots({"ttbar", "st", "diboson", "dylight", "dycc", "dybbother" });
        
        p.set_histogram_filter(RegexFilter(".*", "", "cutflow_mm.*"));
        p.stackplots({"ttbar", "st", "diboson", "dylight", "dycc", "dybbother" });
     }*/
     
     // ee selection:
     /*{
        Plotter p(outputdir, {ttbar, st, diboson, dylight, dycc, dybbother, dybbvis, dybbm_bbvis, dybb4f_bbvis, ee_data}, formatters);
        p.set_histogram_filter(RegexFilter(".*", "fs_ee_.*|presel_ee", ".*"));
        p.set_option("add_nonstacked_to_stack", "1");
        //p.stackplots({"ttbar", "st", "diboson", "dylight", "dycc", "dybbother" });
        
        // top-level (control) histos:
        p.set_histogram_filter(RegexFilter(".*", "", "cutflow_ee.*"));
        p.stackplots({"ttbar", "st", "diboson", "dylight", "dycc", "dybbother" });
     }*/
     
     // mm + ee:
     /*{
         //shared_ptr<ProcessHistograms> ttbar_added(new AddSelections(ttbar, "mm_ee", {"fs_mm_bc2_mll_met", "fs_ee_bc2_mll_met"}));
         //shared_ptr<ProcessHistograms> st_added(new AddSelections(st, "mm_ee", {"fs_mm_bc2_mll_met", "fs_ee_bc2_mll_met"}));
         
         auto mmsel = "fs_mm_bc2_mll";
         auto eesel = "fs_ee_bc2_mll";
         
         shared_ptr<ProcessHistograms> top_added(new AddSelections(top, "mm_ee", {mmsel, eesel}));
         shared_ptr<ProcessHistograms> diboson_added(new AddSelections(diboson, "mm_ee", {mmsel, eesel}));
         
         shared_ptr<ProcessHistograms> dylight_added(new AddSelections(dylight, "mm_ee", {mmsel, eesel}));
         shared_ptr<ProcessHistograms> dycc_added(new AddSelections(dycc, "mm_ee", {mmsel, eesel}));
         shared_ptr<ProcessHistograms> dybbother_added(new AddSelections(dybbother, "mm_ee", {mmsel, eesel}));
         
         shared_ptr<ProcessHistograms> dy_nonsignal(new AddProcesses({dylight_added, dycc_added, dybbother_added}, "dynonb"));
         
         shared_ptr<ProcessHistograms> dybbvis_added(new AddSelections(dybbvis, "mm_ee", {mmsel, eesel}));
         shared_ptr<ProcessHistograms> dybbm_bbvis_added(new AddSelections(dybbm_bbvis, "mm_ee", {mmsel, eesel}));
         shared_ptr<ProcessHistograms> dybb4f_bbvis_added(new AddSelections(dybb4f_bbvis, "mm_ee", {mmsel, eesel}));
         
         shared_ptr<ProcessHistograms> data_added(new AddSelections({mm_data, ee_data}, "mm_ee", {mmsel, eesel}));
         
         //compare_meanpt_vs_dr({ttbar_added, st_added, diboson_added, dylight_added, dycc_added, dybbother_added, dybbvis_added, data_added}, "mm_ee", "dy_", formatters);
         //compare_meanpt_vs_dr({ttbar_added, st_added, diboson_added, dylight_added, dycc_added, dybbother_added, dybbm_bbvis_added, data_added}, "mm_ee", "dybbm_", formatters);
         //compare_meanpt_vs_dr({ttbar_added, st_added, diboson_added, dylight_added, dycc_added, dybbother_added, dybb4f_bbvis_added, data_added}, "mm_ee", "dybb4f_", formatters);
         
         Plotter p(outputdir, {top_added, diboson_added, dy_nonsignal, dybbvis_added, dybbm_bbvis_added, dybb4f_bbvis_added, data_added}, formatters);
         p.set_histogram_filter(RegexFilter(".*", "mm_ee", ".*"));
         p.set_option("add_nonstacked_to_stack", "1");
         p.stackplots({"top", "diboson", "dynonb"});
     }*/
     
     // yields:
     /*{
        Plotter p(outputdir, {ttbar2b, ttbar4b, st, ww, wz, zz, dylight, dycc, dybbother, dybbvis, dybbm_bbvis, dybb4f_bbvis, dybbm_bbother, dybb4f_bbother, mm_data, ee_data, me_data}, formatters);
        // mm
        p.print_integrals("presel_mm", "mll", "mm_yields.txt", "presel", false);
        p.print_integrals("fs_mm_bc2_mll", "mll", "mm_yields.txt", "bc2_mll");
        p.print_integrals("fs_mm_bc2_mll_met", "mll", "mm_yields.txt", "bc2_mll_met");
        p.print_integrals("fs_mm_bc2_mlli", "mll", "mm_yields.txt", "mll sideband (bc2_mlli)");
        
        // ee
        p.print_integrals("presel_ee", "mll", "ee_yields.txt", "presel", false);
        p.print_integrals("fs_ee_bc2_mll", "mll", "ee_yields.txt", "bc2_mll");
        p.print_integrals("fs_ee_bc2_mll_met", "mll", "ee_yields.txt", "bc2_mll_met");
        p.print_integrals("fs_ee_bc2_mlli", "mll", "ee_yields.txt", "mll sideband (bc2_mlli)");
        
        // me
        p.print_integrals("presel_me", "mll", "me_yields.txt", "presel", false);
        p.print_integrals("fs_me_bc1", "mll", "me_yields.txt", "N_b = 1");
        p.print_integrals("fs_me_bc2", "mll", "me_yields.txt", "N_b = 2");
        p.print_integrals("fs_me_bc2_mll", "mll", "me_yields.txt", "N_b = 2, mll cut");
        p.print_integrals("fs_me_bc2_mlli", "mll", "me_yields.txt", "N_b = 2, mll sideband (down to mll=30)");
        
        p.print_integrals("presel_me", "tp2_ptp", "me_tp2.txt", "total probes", false);
        p.print_integrals("presel_me", "tp2_ptp_bcb", "me_tp2.txt", "probes with 2B cands");
        p.print_integrals("presel_me", "tp2_ptp_mcb", "me_tp2.txt", "probes with matching true Bs");
     }*/
     
     // debug_me yields:
     /*{
        Plotter p(outputdir, {ttbar, st, ww, wz, zz, dylight, dycc, dybbother, dybbvis, me_data_a, me_data_b, me_data_c, me_data_d}, formatters);
        p.print_integrals("presel_me", "mll", "debug_me_yields.txt", "presel", false);
        p.print_integrals("fs_me_mll50", "mll", "debug_me_yields.txt", "mll > 50", false);
        p.print_integrals("fs_me_nb1", "mll", "debug_me_yields.txt", "N_bjets >= 1");
        p.print_integrals("fs_me_nb2", "mll", "debug_me_yields.txt", "N_bjets >= 2");
     }*/
}
