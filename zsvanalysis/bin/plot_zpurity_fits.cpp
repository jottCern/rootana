#include "plot/include/utils.hpp"

#include "TH2D.h"
#include "TGraphErrors.h"
#include "TCanvas.h"

#include <iostream>
#include <fstream>
#include <tuple>

using namespace std;
using namespace ra;


namespace {

string read_last_workdir(){
    ifstream in(".mllfit_workdir");
    if(!in){
        throw runtime_error("Could not read last workdir from .mllfir_workdir file");
    }
    string res;
    getline(in, res);
    return res;
}

string workdir = read_last_workdir();
string outputdir = workdir + "plots/";
string recoplots = "/nfs/dust/cms/user/ottjoc/omega-out/ntuplev7/recoplots/";
}

void plot_ttbar_bernstein_fit(const std::string & channel){
    // treat ttbar like data:
    auto ttbar = make_shared<ProcessHistogramsTFile>(workdir + "theta.root", "data");
    auto h_ttbar = ttbar->get_histogram("", channel + "_ttbar__DATA");
    
    auto fitted = make_shared<ProcessHistogramsTFile>(workdir + "ttbar-bernstein-fit-" + channel + ".root", "fit");
    auto h_fitted = fitted->get_histogram("", channel + "_dr0__ttbar");
    
    h_ttbar.histo->Scale(1.0 / h_ttbar.histo->Integral());
    h_fitted.histo->Scale(1.0 / h_fitted.histo->Integral());
    
    h_ttbar.options["xtext"] = "m_{ll} [GeV]";
    h_ttbar.options["ytext"] = "arb.";
    h_ttbar.options["ylabel_factor"] = "1.3";
    //h_ttbar.options["title_ul1"] = channel;
    h_ttbar.legend = "MC";
    h_fitted.legend = "Fit (Berstein of deg. 3)";
    
    h_ttbar.histo->SetLineColor(kBlack);
    
    h_fitted.histo->SetLineWidth(2);
    h_fitted.histo->SetLineColor(kRed);
    
    vector<Histogram> hs;
    hs.emplace_back(h_ttbar.copy_shallow());
    hs.emplace_back(h_fitted.copy_shallow());
    
    draw_histos(hs, outputdir + "ttbar_bernstein_fit_" + channel + ".pdf");
}

void plot_data_zfits(const std::string & channel){
    auto hfile = make_shared<ProcessHistogramsTFile>(workdir + "bwshape-" + channel + ".root", "hf");
    
    auto h_bwshape = hfile->get_histogram("", "bwshape");
    auto h_bkg = hfile->get_histogram("", "ttbar");
    auto h_data = hfile->get_histogram("", "data");
    
    h_data.legend = "Data";
    h_data.process = "data";
    
    h_bkg.legend = "t #bar{t}";
    h_bwshape.legend = "Z shape (BW + Gauss)";
    
    h_bwshape.options["xtext"] = "m_{ll} [GeV]";
    h_bwshape.options["ytext"] = "events";
    h_bwshape.options["draw_ratio"] = "1";
    h_bwshape.options["ratio_ymin"] = "0.5";
    h_bwshape.options["ratio_ymax"] = "1.5";
    h_bwshape.options["ratio_ytext"] = "MC / data";
    
    h_bwshape.options["fill"] = "1";
    h_bkg.options["fill"] = "1";
    //h_bwshape.options["title_ul1"] = "CMS";
    
    
    vector<Histogram> hs;
    hs.emplace_back(h_bwshape.copy_shallow());
    hs.emplace_back(h_bkg.copy_shallow());
    hs.emplace_back(h_data.copy_shallow());
    
    h_bwshape.histo->SetFillColor(kRed);
    h_bkg.histo->SetFillColor(kGreen);
    
    makestack({h_bwshape.histo.get(), h_bkg.histo.get()});
    
    draw_histos(hs, outputdir + "bwshape_fit_" + channel + ".pdf");
}

void check_independence_drbb_mll_ttbar(const std::string & channel, int imin){
    auto ttbar_mc = make_shared<ProcessHistogramsTFile>(recoplots + "ttbar*.root", "ttbar");
    
    int colors[] = {kRed, kBlue, kGreen + 2};
    int ncolors = sizeof(colors) / sizeof(int);
    
    auto h = ttbar_mc->get_histogram("fs_" + channel + "_bc2", "mll_drbb");
    vector<Histogram> hs;
    int nrebin_dr = 6;
    double binwidth_dr = 0.1;
    stringstream hnames;
    for(int i=imin; i<imin + 3; ++i){
        Histogram hproj;
        hproj.histo.reset(((TH2*)h.histo.get())->ProjectionY("py", i*nrebin_dr + 1, (i+1)*nrebin_dr+1));
        hproj.histo->Rebin(5);
        hproj.histo->GetXaxis()->SetRangeUser(60., 150.0);
        hproj.histo->Scale(1.0 / hproj.histo->Integral());
        hproj.histo->SetLineColor(colors[i % ncolors]);
        stringstream ss;
        ss << "#Delta R in [" << (binwidth_dr * i * nrebin_dr) << ", " << (binwidth_dr * nrebin_dr * (i+1)) << "]";
        hproj.legend = ss.str();
        ss.str("");
        ss << "dr" << i;
        hnames << ";" << ss.str();
        hproj.process = ss.str();
        hs.emplace_back(move(hproj));
        hs.back().histo->SetLineWidth(2);
    }
    // get the parameterized background, as fitted in the N_b = 1 sideband:
    auto ttbar_par = make_shared<ProcessHistogramsTFile>(workdir + "ttbar-bernstein-fit-" + channel + ".root", "par");
    auto h_par = ttbar_par->get_histogram("", channel + "_dr0__ttbar");
    h_par.histo->Scale(1.0 / h_par.histo->Integral());
    h_par.histo->Rebin(5);
    for(int i=1; i <= h_par.histo->GetNbinsX(); ++i){
        h_par.histo->SetBinError(i, 0.0);
    }
    h_par.legend = "parameterized";
    hs.emplace_back(move(h_par));
    hs[0].options["draw_ratio"] = hnames.str().substr(1) + " / par";
    hs[0].options["xtext"] = "m_{ll} [GeV]";
    hs[0].options["ytext"] = "arb.";
    hs[0].options["ratio_ymin"] = "0.5";
    hs[0].options["ratio_ymax"] = "1.5";
    hs[0].options["ratio_ytext"] = "MC / par";
    stringstream fname;
    fname << outputdir << "mll_drbb" << imin << "_ttbar_" << channel << ".pdf";
    draw_histos(hs, fname.str());
}

void plot_dr_yields(const std::string & channel){
    auto hfile = make_shared<ProcessHistogramsTFile>(workdir + "result-" + channel + ".root", "r");
    
    auto h_sig = hfile->get_histogram("", "sig");
    h_sig.histo->SetLineWidth(2);
    h_sig.options["use_errors"] = "1";
    h_sig.options["xtext"] = "#Delta R(b,b)";
    h_sig.options["ytext"] = "Z yield";
    
    vector<Histogram> hs;
    hs.emplace_back(h_sig.copy_shallow());
    draw_histos(hs, outputdir + "dr_yield_" + channel + ".pdf");
    
    auto h_chi2 = hfile->get_histogram("", "chi2_nd");
    h_chi2.options["xtext"] = "#Delta R(b,b)";
    h_chi2.options["ytext"] = "#chi^2 / nd";
    hs.clear();
    hs.emplace_back(h_chi2.copy_shallow());
    draw_histos(hs, outputdir + "chi2_dr_" + channel + ".pdf");
    
    // plot fit result vs. MC truth:
    string resulthname, filepattern, legend, shortname;
    tuple<string, string, string, string> ttbar_vars{"bkg", "ttbar*.root", "t#bar{t}", "ttbar"};
    tuple<string, string, string, string> dy_vars{"sig", "dy*jets*.root", "DY", "dy"};
    for(int i=0; i<2; ++i){
        tie(resulthname, filepattern, legend, shortname) = i==0 ? ttbar_vars : dy_vars;
        auto h_fitted = hfile->get_histogram("", resulthname);
        auto mc = make_shared<ProcessHistogramsTFile>(recoplots + filepattern, shortname);
        auto h_mc = mc->get_histogram("fs_" + channel + "_bc2_mll", "DR_BB");
        h_mc.histo->Rebin(8);
        h_mc.legend = legend + " MC";
        h_fitted.legend = legend + " from fit";
        h_fitted.options["use_errors"] = "1";
        h_mc.histo->SetLineColor(kRed);
        hs.clear();
        hs.emplace_back(h_fitted.copy_shallow());
        hs.emplace_back(h_mc.copy_shallow());
        draw_histos(hs, outputdir + shortname + "_fitvsmc_dr_" + channel + ".pdf");
    }
}

namespace {
    
    
class ResultHistograms: public ProcessHistograms {
public:
    virtual string get_process(){
        return "data";
    }
    
    virtual std::vector<string> get_selections(){
        return {"mm_ee"};
    }
    
    virtual std::vector<string> get_hnames(const string & selection){
        return {"DR_BB"};
    }
    
    virtual Histogram get_histogram(const string & selection, const string & hname){
        auto result = hfile_ee->get_histogram("", "sig");
        result.histo->Add(hfile_mm->get_histogram("", "sig").histo.get());
        return move(result);
    }
    
    ResultHistograms(){
        hfile_ee = make_shared<ProcessHistogramsTFile>(workdir + "result-ee.root", "data");
        hfile_mm = make_shared<ProcessHistogramsTFile>(workdir + "result-mm.root", "data");
    }
    
private:
    shared_ptr<ProcessHistograms> hfile_ee, hfile_mm;
};


}

void plot_combined_dr_yields(){
    //auto hfile_ee = make_shared<ProcessHistogramsTFile>("result-ee.root", "r");
    //auto hfile_mm = make_shared<ProcessHistogramsTFile>("result-mm.root", "r");
    
    string workdir = "/nfs/dust/cms/user/ottjoc/omega-out/ntuplev7/recoplots/";
    
    shared_ptr<ProcessHistograms> dybbvis(new ProcessHistogramsTFile(workdir + "dy{[0-4]jets,}bbvis.root", "dybbvis"));
    shared_ptr<ProcessHistograms> dybbother(new ProcessHistogramsTFile(workdir + "dy{[0-4]jets,}bbother.root", "dybbother"));
    shared_ptr<ProcessHistograms> dycc(new ProcessHistogramsTFile(workdir + "dy{[0-4]jets,}cc.root", "dycc"));
    shared_ptr<ProcessHistograms> dylight(new ProcessHistogramsTFile(workdir + "dy{[0-4]jets,}other.root", "dylight"));
    
    shared_ptr<ProcessHistograms> dybb4f_bbvis(new ProcessHistogramsTFile(workdir + "dybb4fbbvis.root", "dybb4f_bbvis"));
    shared_ptr<ProcessHistograms> dybbm_bbvis(new ProcessHistogramsTFile(workdir + "dybbmbbvis.root", "dybbm_bbvis"));
    
    auto mmsel = "fs_mm_bc2_mll";
    auto eesel = "fs_ee_bc2_mll";
    
    shared_ptr<ProcessHistograms> dylight_added(new AddSelections(dylight, "mm_ee", {mmsel, eesel}));
    shared_ptr<ProcessHistograms> dycc_added(new AddSelections(dycc, "mm_ee", {mmsel, eesel}));
    shared_ptr<ProcessHistograms> dybbother_added(new AddSelections(dybbother, "mm_ee", {mmsel, eesel}));
    
    shared_ptr<ProcessHistograms> dy_nonsignal(new AddProcesses({dylight_added, dycc_added, dybbother_added}, "dynonb"));
    
    shared_ptr<ProcessHistograms> dybbvis_added(new AddSelections(dybbvis, "mm_ee", {mmsel, eesel}));
    shared_ptr<ProcessHistograms> dybbm_bbvis_added(new AddSelections(dybbm_bbvis, "mm_ee", {mmsel, eesel}));
    shared_ptr<ProcessHistograms> dybb4f_bbvis_added(new AddSelections(dybb4f_bbvis, "mm_ee", {mmsel, eesel}));
    
    shared_ptr<ProcessHistograms> result(new ResultHistograms());
    
    
    Formatters formatters;
    formatters.add<SetLineColor>("*", 1)
        ("dybbm_bbvis:", kCyan + 2) ("dybb4f_bbvis:", kYellow + 2) ("dybbvis:", kGray + 2)
        ("dybbm_bbother:", kCyan + 2) ("dybb4f_bbother:", kYellow + 2) ("dybbother:", kGray + 2)
        ("dybbm_bbany:", kCyan + 2) ("dybb4f_bbany:", kYellow + 2) ("dybbany:", kGray + 2);
    formatters.add<SetLineWidth>("dybbm_bbvis:", 2)("dybb4f_bbvis:", 2)("dybbvis:", 2);
    formatters.add<SetFillColor>("top:", kRed)("ttbar:", kRed)
          ("ttbar2b:", kRed) ("ttbar4b:", kYellow)
          ("st:", kRed - 6)("diboson:", kGreen + 3)
          ("dybbother:", kBlue - 7) ("dycc:", kBlue + 1) ("dylight:", kBlue-1)("dynonb:", kBlue-1);
    
    formatters.add<SetLegends>
        ("dybbvis:", "Z/#gamma*+bb (5FS, mb=0)")
        ("dybbany:", "Z/#gamma*+bb (any)")
        ("dybbother:", "Z/#gamma*+bb (invis)")
        ("dynonb:", "Z/#gamma*+c/light")
        ("dycc:", "Z/#gamma*+Jets (cc)")
        ("dylight:", "Z/#gamma*+Jets (light)")
        ("data:", "Data (fit)")
        ("dybbm_bbvis:", "Z/#gamma*+bb (5FS, mb > 0)")
        ("dybb4f_bbvis:", "Z/#gamma*+bb (4FS, mb > 0)");
        
    formatters.add<RebinFactor>("dynonb:DR_BB", 8)("dybb4f_bbvis:DR_BB", 8)("dybbm_bbvis:DR_BB", 8)("dybbvis:DR_BB", 8);
    
    formatters.add<SetOption>("DR_BB", "xmin", "0.0")("DR_BB", "xmax", "4.0")("DR_BB", "more_ymax", "0.25")("DR_BB", "xtext", "#Delta R(B,B)")("*", "ytext", "events");
    
    Plotter p("plot_zpurity_fits_out", {result, dy_nonsignal, dybbvis_added, dybbm_bbvis_added, dybb4f_bbvis_added}, formatters);
    p.set_histogram_filter(RegexFilter(".*", "mm_ee", ".*"));
    p.set_option("add_nonstacked_to_stack", "1");
    p.stackplots({"dynonb"});
}


int main(){
    cout << "Using input dir '" << workdir << "'" << endl;
    
    plot_ttbar_bernstein_fit("mm");
    plot_ttbar_bernstein_fit("ee");
    
    plot_data_zfits("mm");
    plot_data_zfits("ee");
    
    plot_dr_yields("mm");
    plot_dr_yields("ee");
    
    //plot_combined_dr_yields();
    
    //check_independence_drbb_mll_ttbar("mm", 0);
    //check_independence_drbb_mll_ttbar("mm", 3);
}

