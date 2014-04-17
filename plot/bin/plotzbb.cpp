#include "TROOT.h"
#include "utils.hpp"

#include <iostream>

using namespace std;
using namespace ra;


int main(){
    string path = "/afs/desy.de/user/o/ottjoc/xxl-af-cms/zsv-cmssw/CMSSW_5_3_14_patch2/src/ZSVAnalysis/GenStudies/";
    shared_ptr<ProcessHistograms> mgz2j(new ProcessHistogramsTFile(path + "mg5-z2j-plots*.root", "mgz2j"));
    shared_ptr<ProcessHistograms> mg4fs(new ProcessHistogramsTFile(path + "mg5-zbb-plots*.root", "mg4fs"));
    
    Formatters formatters;
    //formatters.add<SetLineColor>("", SetLineColor(1));
    formatters.add<SetLineColor>("mgz2j:", 810)  ("mg4fs:", 414);
    //formatters.add<SetLineColor>("ttbar:", 810)  ("dy:", 414) ("dyexcl:", kMagenta);
    formatters.add<SetLegends>("mgz2j:", "MadGraph Z2j") ("mg4fs:", "MadGraph Z+bb 4FS");
    formatters.add<SetOption>
       ("*", "title_ur", "L = 1/fb")
       ("*", "ytext", "events")
       ("*", "draw_ratio", "mg4fs / mgz2j")
       ("*", "ratio_ymin", "0.0")
       ("*", "ratio_ymax", "1.5")
       ("bb_lepm12", "xtext", "m_{ee} [GeV]")
       //("bb_lepm12", "ylabel_factor", "1.3")
       ("bb_lepm12", "ylog", "1")
       ("bb_*", "title_ul", "BB phase space")
       ;
    //formatters.add<RebinFactor>("mll", 4)("ptz", 4)("Bpt",4)("Beta",4)("DPhi_BB", 2)("DR_BB", 2)("m_BB", 4);
    
    formatters.add<Scale>("mgz2j:", 0.9635);
    formatters.add<Scale>("mg4fs:", 0.0294);
    
    Plotter p("eps", {mgz2j, mg4fs}, formatters);
    
    p.stackplots({});
    
    p.print_integrals("plot/bb_bh_mult", "yields.txt", "BB", false);
    p.print_integrals("plot/zbb_bh_mult", "yields.txt", "ZBB");
    p.print_integrals("plot/zbbvis_bh_mult", "yields.txt", "ZBB_vis");
    p.print_integrals("plot/zbbjet_bh_mult", "yields.txt", "ZBB_jet");
    
    //p.shapeplots({"dy", "dyexcl"});
    
    //Plotter p("eps", {dy, dyexcl}, formatters);
    //p.stackplots({});
}
