#include "plot/include/utils.hpp"

#include <iostream>

using namespace std;
using namespace ra;

int main(){
    const string outputdir = "plot_gen_out/";
    const string inputdir = "/nfs/dust/cms/user/ottjoc/omega-out/ntuple-latest/genplots/";
    shared_ptr<ProcessHistograms> dybbm(new ProcessHistogramsTFile(inputdir+"dybbm.root", "dybbm"));
    shared_ptr<ProcessHistograms> dybb4f(new ProcessHistogramsTFile(inputdir+"dybb4f.root", "dybb4f"));
    
    /*shared_ptr<ProcessHistograms> dy0(new ProcessHistogramsTFile(inputdir+"dy.root", "dy0"));
    shared_ptr<ProcessHistograms> dy1(new ProcessHistogramsTFile(inputdir+"dy1jets.root", "dy1"));
    shared_ptr<ProcessHistograms> dy2(new ProcessHistogramsTFile(inputdir+"dy2jets.root", "dy2"));
    shared_ptr<ProcessHistograms> dy3(new ProcessHistogramsTFile(inputdir+"dy3jets.root", "dy3"));
    shared_ptr<ProcessHistograms> dy4(new ProcessHistogramsTFile(inputdir+"dy4jets.root", "dy4"));*/
    
    shared_ptr<ProcessHistograms> dy(new ProcessHistogramsTFile({inputdir+"dy[1-4]jets.root", inputdir+"dy.root"}, "dy"));
    
    Formatters formatters_x;
    formatters_x.add<SetOption>
       ("*", "title_ur", "scaled to L=19.7/fb")
       (":all/", "title_ul", "no cuts")
       (":gen/", "title_ul", "N_{B} = 2 (p_{T}(B) > 15 GeV, |#eta(B)| < 2)")
       ("drbb", "xtext", "#Delta R(B,B)")
       ("dphibb", "xtext", "#Delta #phi(B,B)")
       ("ptb0", "xtext", "p_{T}(B_{0}) [GeV]")
       ("ptb1", "xtext", "p_{T}(B_{1}) [GeV]")
       ("etab0", "xtext", "#eta (B_{0})")
       ("etab1", "xtext", "#eta (B_{1})")
       ("nb", "ylog", "1")
       ;
    
       
    // 1. shape plots for all histograms, comparing processes:
    Formatters formatters(formatters_x);
    formatters.add<RebinFactor>("drbb", 5)("dphibb", 4)("pt*", 4)("eta*",4);
    formatters.add<SetFillColor>("dy0:", 415)("dy1:", 416)("dy2:", 417)("dy3:", 418)("dy4:", 419);
    formatters.add<SetLineColor>("*", 1)("dybbm:", 810) ("dy:", 414) ("dybb4f:", kBlue) ;
    formatters.add<SetLineWidth>("*", 2);
    formatters.add<SetLegends>("dy:", "Z/#gamma*+Jets inclusive", "$Z/\\gamma^*$+Jets")("dybbm:", "Z/#gamma*+bb+Jets, massive-b") ("dybb4f:", "Z/#gamma*+bb+Jets, 4FS");
    formatters.add<SetOption>("*", "ytext", "events")("*", "ylabel_factor", "1.1");
    
    Plotter p1(outputdir, {dybbm, dybb4f, dy}, formatters);
    //p1.shapeplots({"ttbar", "dy"});
    p1.stackplots({});
    
    //Plotter p2(outputdir, {dybbm, dybb4f, dy0, dy1, dy2, dy3, dy4}, formatters);
    //p2.stackplots({"dy0", "dy1", "dy2", "dy3", "dy4"}, "dyex");
}

