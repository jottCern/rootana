#include "plot/include/utils.hpp"

#include <iostream>

using namespace std;
using namespace ra;


namespace {
string outputdir = "plot_reco_out/";
}

int main(){
    //const string inputdir = "/nfs/dust/cms/user/ottjoc/omega-out/ntuple-latest/recoplots/";
    const string inputdir = "/nfs/dust/cms/user/ottjoc/omega-out/ntuple-latest/plot_me/";
    
    //shared_ptr<ProcessHistograms> dy_all(new ProcessHistogramsTFile(inputdir + "dy[0-4]jets*.root", "dy_all"));
    
    shared_ptr<ProcessHistograms> dybbvis(new ProcessHistogramsTFile(inputdir + "dy[0-4]jets-zbbvis.root", "dybbvis"));
    shared_ptr<ProcessHistograms> dybbother(new ProcessHistogramsTFile(inputdir + "dy[0-4]jets-zbbother.root", "dybbother"));
    shared_ptr<ProcessHistograms> dybbany(new ProcessHistogramsTFile(inputdir + "dy[0-4]jets-zbb*.root", "dybbany"));
    shared_ptr<ProcessHistograms> dycc(new ProcessHistogramsTFile(inputdir + "dy[0-4]jets-zcc*.root", "dycc"));
    shared_ptr<ProcessHistograms> dylight(new ProcessHistogramsTFile(inputdir + "dy[0-4]jets-zother*.root", "dylight"));
    
    shared_ptr<ProcessHistograms> dybb4f_bbvis(new ProcessHistogramsTFile(inputdir + "dybb4f-zbbvis.root", "dybb4f_bbvis"));
    shared_ptr<ProcessHistograms> dybbm_bbvis(new ProcessHistogramsTFile(inputdir + "dybbm-zbbvis.root", "dybbm_bbvis"));
    
    shared_ptr<ProcessHistograms> dybb4f_bbother(new ProcessHistogramsTFile(inputdir + "dybb4f-zbbother.root", "dybb4f_bbother"));
    shared_ptr<ProcessHistograms> dybbm_bbother(new ProcessHistogramsTFile(inputdir + "dybbm-zbbother.root", "dybbm_bbother"));
    
    //shared_ptr<ProcessHistograms> dybb4f_bbany(new ProcessHistogramsTFile(inputdir + "dybb4f-zbb*.root", "dybb4f_bbany"));
    //shared_ptr<ProcessHistograms> dybbm_bbany(new ProcessHistogramsTFile(inputdir + "dybbm-zbb*.root", "dybbm_bbany"));
    
    //shared_ptr<ProcessHistograms> top(new ProcessHistogramsTFile({inputdir + "ttbar.root", inputdir + "st*.root"}, "top"));
    shared_ptr<ProcessHistograms> ttbar(new ProcessHistogramsTFile({inputdir + "ttbar.root"}, "ttbar"));
    shared_ptr<ProcessHistograms> st(new ProcessHistogramsTFile(inputdir + "st*.root", "st"));
    shared_ptr<ProcessHistograms> diboson(new ProcessHistogramsTFile({inputdir + "ww.root", inputdir + "wz.root", inputdir + "zz.root"}, "diboson"));
    
    //shared_ptr<ProcessHistograms> mm_data(new ProcessHistogramsTFile(inputdir + "dmu*.root", "data"));
    //shared_ptr<ProcessHistograms> ee_data(new ProcessHistogramsTFile(inputdir + "dele*.root", "data"));
    shared_ptr<ProcessHistograms> me_data(new ProcessHistogramsTFile(inputdir + "mue*.root", "data"));
    
    Formatters formatters;
    formatters.add<SetLineColor>("*", 0)
        ("dybbm_bbvis:", kCyan + 2) ("dybb4f_bbvis:", kYellow + 2) ("dybbvis:", kGray + 2)
        ("dybbm_bbother:", kCyan + 2) ("dybb4f_bbother:", kYellow + 2) ("dybbother:", kGray + 2)
        ("dybbm_bbany:", kCyan + 2) ("dybb4f_bbany:", kYellow + 2) ("dybbany:", kGray + 2);
    formatters.add<SetLineWidth>("*bbvis:", 3);
    formatters.add<SetFillColor>("top:", kRed)("ttbar:", kRed)("st:", kRed - 6)("diboson:", kGreen + 3)
          ("dybbother:", kBlue - 7) ("dycc:", kBlue + 1) ("dylight:", kBlue-1);
    formatters.add<SetLegends>("top:", "top (t#bar{t}, tW)")
        ("ttbar:", "t#bar{t}")
        ("st:", "tW")
        ("diboson:", "diboson (WW, ZZ, WZ)")
        ("dybbvis:", "Z/#gamma*+Jets (bbvis)")
        ("dybbany:", "Z/#gamma*+Jets (bb)")
        ("dybbother:", "Z/#gamma*+Jets (bb invis)")
        ("dycc:", "Z/#gamma*+Jets (cc)")
        ("dylight:", "Z/#gamma*+Jets (light)")
        ("data:", "Data")
        ("dybbm_bbvis:", "Z/#gamma*+bb+Jets mb (bbvis)")
        ("dybb4f_bbvis:", "Z/#gamma*+bb+Jets 4FS (bbvis)")
        ("dybbm_bbother:", "Z/#gamma*+bb+Jets mb (bb invis)")
        ("dybb4f_bbother:", "Z/#gamma*+bb+Jets 4FS (bb invis)")
        ("dybbm_bbany:", "Z/#gamma*+bb+Jets mb (bb)")
        ("dybb4f_bbany:", "Z/#gamma*+bb+Jets 4FS (bb)")
        ;
    formatters.add<SetOption>
       ("*", "title_ur", "19.7 fb^{-1}, #sqrt{s}=8TeV")
       (":fs_bc2_mlli/", "title_ul", "N_{B}=2, Z veto")
       (":fs_bc1/", "title_ul", "N_{B}=1")
       (":fs_bc2_mll_met/", "title_ul", "final (N_{B}=2, Z, MET<50GeV)")
       ("mll", "draw_ratio", "1")
       ("*", "ratio_ymax", "1.5")
       ("*", "ratio_ymin", "0.5")
       ("*", "legend_fontsize", "0.025")
       ("*", "more_ymax", "0.3")
       ("*", "ytext", "events")
       ("*", "ylabel_factor", "1.3")
       ("mll", "xtext", "m_{ll} [GeV]")
       ("DR_ZB", "xtext", "#Delta R(Z,B)")
       ("DR_BB", "xtext", "#Delta R(B,B)")
       ("A_ZBB", "xtext", "A_{ZBB}")
       ("met", "xtext", "E_{T}^{miss} [GeV]")
       ("m_BB", "xtext", "m_{BB} [GeV]")
       ("Beta", "xtext", "#eta_{B}")
       ("Bpt", "xtext", "p_{T,B} [GeV]")
       ("min_DR_ZB", "xtext", "min #Delta R(Z,B)")
       ("DPhi_BB", "xtext", "#Delta #phi(B,B)")
       ("nbh", "xtext", "N_{B}")
       ("DR_BB", "ratio_ymin", "0.5")
       ("DR_BB", "ratio_ymax", "1.5")
       ("cutflow*", "ylog", "1")
       //("nbh", "ymax", "300")
       ;
    formatters.add<RebinFactor>("mll", 4)("ptz", 4)("Bpt",4)("Beta",4)("DPhi_BB", 2)("DR_BB", 2)("m_BB", 4)("bc_pt_over_bj_pt", 4);
    
    // add DY+Jets NLO k-factor:
    formatters.add<Scale>("dybbvis:", 1.197)("dybbany:", 1.197)("dybbother:", 1.197)("dycc:", 1.197)("dylight:", 1.197)
        ("dybbm_bbvis:", 1.197)
        ("dybb4f_bbvis:", 1.197)
        ("dybbm_bbany:", 1.197)
        ("dybb4f_bbany:", 1.197);
    
    // scale final selection by 0.92**2 (IVF data/MC eficiency factor):
    //formatters.add<Scale>(":fs_bc1/", 0.92)(":fs_bc1_mlli/", 0.92)(":fs_bc2_mll_met/", 0.92 * 0.92)(":fs_bc2_mlli/", 0.92 * 0.92)
    //                     ("data:fs_bc1/", 1/0.92)("data:fs_bc1_mlli/", 1/0.92)("data:fs_bc2_mll_met/", 1/0.92/0.92)("data:fs_bc2_mlli/", 1/0.92/0.92);
    
     //Plotter p(outputdir, {top, diboson, dylight, dycc, dybbany, dybbm_bbany, dybb4f_bbany, data}, formatters);
     //p.stackplots({"dycc", "dylight", "diboson", "top"}, "", true);
     
    
     // separate visible and invisible
     //Plotter p(outputdir, {top, diboson, dylight, dycc, dybbother, dybbm_bbvis, dybb4f_bbvis, data}, formatters);
     //p.set_option("add_nonstacked_to_stack", "1");
     //p.stackplots({"dybbother", "dycc", "dylight", "diboson", "top"}, "bbsep");
     
     // plot me selections:
     {
        Plotter p(outputdir, {ttbar, st, diboson, dylight, dycc, dybbany, me_data}, formatters);
        //p.set_histogram_filter(RegexFilter(".*", ".*_me_.*|presel_me", ".*"));
        //p.print_integrals("presel_me", "mll", "yields.txt", "presel me", false);
        p.stackplots({"ttbar", "st", "diboson", "dylight", "dycc", "dybbany" });
     }
     
     // mm selection:
     /*{
        Plotter p(outputdir, {top, diboson, dylight, dycc, dybbany, mm_data}, formatters);
        p.set_histogram_filter(RegexFilter(".*", "fs_mm_.*|presel_mm", ".*"));
        p.stackplots({"top", "diboson", "dylight", "dycc", "dybbany" });
     }*/
     
     // ee selection:
     /*{
        Plotter p(outputdir, {top, diboson, dylight, dycc, dybbany, ee_data}, formatters);
        p.set_histogram_filter(RegexFilter(".*", "fs_ee_.*|presel_ee", ".*"));
        p.stackplots({"top", "diboson", "dylight", "dycc", "dybbany" });
        p.set_histogram_filter(RegexFilter(".*", "", ".*"));
        p.stackplots({"top", "diboson", "dylight", "dycc", "dybbany" });
     }*/
     
     // compare the invisible component of different samples
     /*Plotter p3(outputdir, {dybbother, dybbm_bbother, dybb4f_bbother}, formatters);
     p3.stackplots({}, "bbother");*/
     
     //p.cutflow("cutflow", "cutflow.tex");
}
