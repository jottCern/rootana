#include "TROOT.h"
#include "utils.hpp"

#include <iostream>

using namespace std;
using namespace ra;


int main(){
    string path = "/afs/desy.de/user/o/ottjoc/xxl-af-cms/zsv-cmssw/CMSSW_5_3_14_patch2/src/ZSVAnalysis/GenStudies/";
    shared_ptr<ProcessHistograms> mgz2j(new ProcessHistogramsTFile(path + "mg5-z2j-plots*.root", "mgz2j"));
    shared_ptr<ProcessHistograms> mg4fs(new ProcessHistogramsTFile(path + "mg5-zbb-plots*.root", "mg4fs"));
    shared_ptr<ProcessHistograms> amcatnlo(new ProcessHistogramsTFile(path + "amcatnlo.root", "amcatnlo"));
    
    Formatters formatters;
    //formatters.add<SetLineColor>("", SetLineColor(1));
    formatters.add<SetLineColor>("mgz2j:", 810)  ("mg4fs:", 414);
    //formatters.add<SetLineColor>("ttbar:", 810)  ("dy:", 414) ("dyexcl:", kMagenta);
    formatters.add<SetLegends>("mgz2j:", "MadGraph Z2j") ("mg4fs:", "MadGraph Zbb 4FS")("amcatnlo:", "aMC@NLO Zbb 4FS");
    formatters.add<SetOption>
       ("*", "title_ur", "L = 1/fb")
       ("*", "ytext", "events")
       //("*", "draw_ratio", "mg4fs / mgz2j")
       ("*", "ratio_ymin", "0.0")
       ("*", "ratio_ymax", "1.5")
       ("bb_lepm12", "xtext", "m_{ee} [GeV]")
       //("bb_lepm12", "ylabel_factor", "1.3")
       ("*", "xlabel_factor", "1.1")
       ("bb_lepm12", "ylog", "1")
       ("bb_*", "title_ul", "BB phase space")
       ("zbb_*", "title_ul", "ZBB phase space")
       ("zbbvis_*", "title_ul", "ZBB_vis phase space")
       ("zbbjet_*", "title_ul", "ZBB_jet phase space")
       ("*_bh_pt1", "xtext", "p_{T}^{B1} [GeV]")
       ("*_bh_pt2", "xtext", "p_{T}^{B2} [GeV]")
       ("*_bh_eta1", "xtext", "#eta^{B1}")
       ("*_bh_eta2", "xtext", "#eta^{B2}")
       ;
       
       
    formatters.add<RebinFactor>("zbbvis_*", 4);
    
    formatters.add<Scale>("mgz2j:", 0.9635)("mg4fs:", 0.0294)("amcatnlo:", 0.001);
    
    Plotter p("eps", {mgz2j, mg4fs, amcatnlo}, formatters);
    
    //p.stackplots({});
    
    p.print_integrals("plot/bb_bh_mult", "yields.txt", "BB", false);
    p.print_integrals("plot/zbb_bh_mult", "yields.txt", "ZBB");
    p.print_integrals("plot/zbbvis_bh_mult", "yields.txt", "ZBB_vis");
    p.print_integrals("plot/zbbjet_bh_mult", "yields.txt", "ZBB_jet");
    
    p.shapeplots({"mgz2j", "mg4fs", "amcatnlo"});
    
    //Plotter p("eps", {dy, dyexcl}, formatters);
    //p.stackplots({});
}
