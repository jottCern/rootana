#include "TROOT.h"
#include "utils.hpp"

#include <iostream>
#include <sstream>

using namespace std;
using namespace ra;


void add_default_formatters(Formatters & formatters){
    formatters.add<SetLineWidth>("*", 2.0);
    formatters.add<RebinFactor>("drbb", 2);
    formatters.add<SetOption>
       ("*", "title_ur", "L = 1/fb")
       ("*", "ytext", "events")
       ("mll", "xtext", "m_{ee} [GeV]")
       ("ptll", "xtext", "p_{T,ee} [GeV]")
       ("drbb", "xtext", "#Delta R(b,b)")
       ("dphibb", "xtext", "#Delta #phi(b,b)")
       ("ptb0*", "xtext", "p_{T}(b_{0}) [GeV]")
       ("ptb1*", "xtext", "p_{T}(b_{1}) [GeV]")
       ("ptj0", "xtext", "p_{T}(j_{0}) [GeV]")
       ("ptj1", "xtext", "p_{T}(j_{1}) [GeV]")
       ("etaj0", "xtext", "#eta(j_{0})")
       ("etaj1", "xtext", "#eta(j_{1})")
       ("*", "xlabel_factor", "1.1");
}

void comparison_plot(const string & name, const std::vector<string> & names, const std::vector<string> & legends, const string & ratio = ""){
    if(legends.size() != names.size()) throw invalid_argument("legends.size() != names.size()");
    
    const string path = "/afs/desy.de/user/o/ottjoc/xxl-af-cms/code/lhe-analysis/";
    vector<shared_ptr<ProcessHistograms>> phs;
    Formatters formatters;
    add_default_formatters(formatters);
    size_t i=0;
       /*("*", "draw_ratio", "madgraph / alpgen")
       ("*", "ratio_ymin", "0.8")
       ("*", "ratio_ymax", "1.2")*/
    const int colors[] = {810, 414, kBlue, kYellow+1, kMagenta+1, kGray, kBlack};
    for(auto sname : names){
        phs.emplace_back(new ProcessHistogramsTFile(path + sname + ".root", sname));
        formatters.add<SetLegends>(sname + ":", legends[i]);
        formatters.add<SetLineColor>(sname + ":", colors[i]);
        ++i;
    }
    Plotter p(name, phs, formatters);
    p.stackplots({});
    p.print_integrals("nj", name + "/yields.txt", "yields", false);
}

int main(){
    comparison_plot("0j-ptb15", {"madgraph0-noas-ptb15", "madgraph0-asfl5-ptb15", "madgraph0-asfl4-ptb15", "alpgen0-ickkw-ptb15", "alpgen0-noickkw-ptb15", "alpgen0-ickkw-mt-patched6-ptb15"},
                                {"MadGraph no asrwgt", "MadGraph asrwgtfl=5","MadGraph asrwgtfl=4", "Alpgen ickkw=1", "Alpgen ickkw=0", "Alpgen ickkw=1, patched"});
    
    comparison_plot("1j-ptb15", {"madgraph1-noas-ptb15", "madgraph1-asfl5-ptb15", "madgraph1-asfl4-ptb15", "alpgen1-ickkw-ptb15", "alpgen1-noickkw-ptb15", "alpgen1-ickkw-patched6-ptb15"},
                                {"MadGraph no asrwgt", "MadGraph asrwgtfl=5","MadGraph asrwgtfl=4", "Alpgen ickkw=1", "Alpgen ickkw=0", "Alpgen ickkw=1, patched"});
    
    /*comparison_plot("ag1j-ptb15", {"alpgen1-ickkw-ptb15", "alpgen1-ickkw-mt-ptb15", "alpgen1-noickkw-ptb15"},
                                {"Alpgen ickkw=1 as(pt)", "Alpgen ickkw=1 as(mt),", "Alpgen ickkw=0"});*/
    //comparison_plot("2j-ptb15", {"madgraph2-asfl5-ptb15", "madgraph2-asfl4-ptb15", "alpgen2-ickkw-ptb15"}, {"MadGraph asrwgtfl=5","MadGraph asrwgtfl=4", "Alpgen ickkw=1"});
}

