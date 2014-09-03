#include "utils.hpp"

#include "TStyle.h"
#include "TH1.h"
#include "TKey.h"
#include "TROOT.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TLegendEntry.h"
#include "TLatex.h"
#include "TLine.h"

#include <stdexcept>
#include <fstream>
#include <stdlib.h> // for strtol
#include <iomanip>

#include <list>
#include <set>

#include <boost/lexical_cast.hpp>

#include "base/include/utils.hpp"

using namespace std;
using namespace ra;

namespace{
    
bool draw_init2() {
    gROOT->SetStyle("Plain");
    gStyle->SetPalette(1,0);
    gStyle->SetOptStat(0);
    gStyle->SetNdivisions(510, "x");
    gStyle->SetNdivisions(505, "y");

    gStyle->SetOptTitle(0);
    gStyle->SetPadTickX(0);
    gStyle->SetPadTickY(0);
    
    gStyle->SetTextSize(0.03);
    
    gStyle->SetTitleOffset(1.9, "y");

    gStyle->SetPadTopMargin(0.08);
    gStyle->SetPadBottomMargin(0.15);
    gStyle->SetPadLeftMargin(0.15);
    gStyle->SetPadRightMargin(0.05);
    
    gStyle->SetTitleFont(42);
    gStyle->SetTitleFont(42, "XYZ");
    gStyle->SetLabelFont(42, "XYZ");
    gStyle->SetTextFont(42);
    //gStyle->SetLegendFont(42);
    
    //gStyle->SetHatchesLineWidth(2);

    /*gStyle->SetLineStyleString(2, "[24 24]");
    gStyle->SetLineStyleString(3, "[8 16]");
    gStyle->SetLineStyleString(4, "[24 30 8 30]");
    gStyle->SetLineStyleString(5, "[48 20 8 20]");
    gStyle->SetLineStyleString(6, "[8 16 8 16 24 24]");
    gStyle->SetLineStyleString(7, "[24 24 24 24 8 16]");*/
    return true;
}
   
bool abc = draw_init2();

long int string2int(const std::string & s){
    const char * c = s.c_str();
    char * end;
    long int result = strtol(c, &end, 10);
    if(end != c + s.size()){
        throw runtime_error("could not convert '" + s + "' to int.");
    }
    return result;
}

//Will add such that the histos should be drawn in the order given
// here, i.e., first histo will be the sum of all, and so on ...
void makestack(vector<TH1*> & histos){
  for(size_t i=0; i<histos.size(); ++i){
    for(size_t j=i+1; j<histos.size(); ++j){
      histos[i]->Add(histos[j]);
    }
  }
}

template<typename T>
T get_option(const map<string, string> & options, const string & key, const T def){
    auto it = options.find(key);
    if(it==options.end()) return def;
    return boost::lexical_cast<T>(it->second);
}

void trim(string & s){
    while(!s.empty() && s[0]==' ') s = s.substr(1);
    while(!s.empty() && s.back() == ' ') s = s.substr(0, s.size()-1);
}

}


// Draw the given histograms in the order they are arranged in the vector.
//
// histos[0].options control many aspects of drawing:
// * "title_ul": additional title placed on the upper left, e.g. "CMS preliminary sqrt(s) = XTeV"
// * "title_ul1": upper left lower title, within the plot, line 1
// * "title_ul2": upper left lower title, within the plot, line 2
// * "title_ul3": upper left title, line 3
// * "title_ur": additional title placed on the upper right, e.g. for lumi
//
// * "legend_align": either "left" (in this case, do not use title_ul1, title_ul2) or "right" (default)
// * "legend_xshift": double, shift of legend from default position
// * "legend_fontsize" (default: 0.03) font size for the legend
// * "legend_linespread" (default: 1.1) vertical distance of the lines in the legend (in units of the font size)
//
// * "xlabel_factor" and "ylabel_factor": values > 1.0 increase the distance of axis / axis legend (default: 1.0)
// * "xmin", "xmax" to override the x range of histos[0] which is drawn. Note that this only works if both, xmin and xmax, are set.
// * "ymin", "ymax" to override the y range of the histogram
// * "more_ymax": set higher maximum for y axis to increase space in y direction for legend and titles, as fraction of pad height (default: 0.0)
// * "ylog": if non-empty, logarithmic y axis
//
// * "auto_linestyles": if non-empty, use different line styles for all non-filled histograms for better b/w readability
// * "as_function": if non-empty, draws histograms as TGraphs with same linecolor / style
// * "labelsize"/"titlesize": if non-empty and positive, the size of the x/y labels/titles
//
// * "draw_total_error": string. If non-empty, draw the total error (=the one from histos[0], which is usually the complete background in a stackplot) as hatched area
// * "draw_ratio": if non-empty, should be set to "proc1 / proc2" with process names proc1 and proc2; defaults to "data / bkg". In this case, further options are
//   - "ratio_ytext": y axis label for ratio plot
//   - "ratio_ymin" and "ratio_ymax": minimum and maximum y value
//   - "ratio_line1": if non-empoty draw a line at ratio=1
// * "vertical_line": a x value where to draw a vertical line
//
// All of the above options are taken from histos[0].options.
//
// Additional interpreted options per histogram are:
// * "use_errors": plot the errors
// * "fill": fill the histogram. If false, only draw a line
void draw_histos(const vector<Histogram> & histos, const string & filename) {
    if(histos.size() == 0){
        throw runtime_error("draw_histos: empty histos vector given");
    }
    const map<string, string> & options = histos[0].options;
    bool debug = get_option<string>(options, "debug", "").size();
    bool ylog = get_option<string>(options, "ylog", "").size() > 0;
    bool auto_linestyles = get_option<string>(options, "auto_linestyles", "").size() > 0;
    bool draw_total_error = get_option<string>(options, "draw_total_error", "").size() > 0;
    bool draw_ratio = get_option<string>(options, "draw_ratio", "").size() > 0;
    double labelsize = get_option<double>(options, "labelsize", -1.0);
    double titlesize = get_option<double>(options, "titlesize", -1.0);
    string ytext = get_option<string>(options, "ytext", histos[0].histo->GetYaxis()->GetTitle());
    string xtext = get_option<string>(options, "xtext", histos[0].histo->GetXaxis()->GetTitle());
    double xmin = get_option<double>(options, "xmin", NAN);
    double xmax = get_option<double>(options, "xmax", NAN);
    double options_ymin = get_option<double>(options, "ymin", NAN);
    double options_ymax = get_option<double>(options, "ymax", NAN);
    if(!std::isnan(xmin) && !std::isnan(xmax)){
        histos[0].histo->GetXaxis()->SetRangeUser(xmin, xmax);
    }
    const int n_linestyles = 10;
    const int linestyles[n_linestyles] = {1,2,3,4,5,6,7,8,9,10};
    int i_linestyle = 0;
    
    // basic sanity checks:
    int n = -1;
    for(const auto & h : histos){
        if(!h.histo) throw logic_error("draw_histos: passed null ptr in histos vector for process '" + nameof(h.process)
            + "', selection '" + nameof(h.selection) + "', hname '" + nameof(h.hname) + "'");
        if(n==-1){
            n = h.histo->GetNbinsX();
        }
        else{
            if(n != h.histo->GetNbinsX()){
                throw logic_error("draw_histos: inconsistent binning for process '" +
                    nameof(h.process) + "', selection '" + nameof(h.selection) + "', hname '" + nameof(h.hname) + "'");
            }
        }
    }
    
    
    if(auto_linestyles){
        for(size_t i=0; i<histos.size(); ++i){
            if(histos[i].histo->GetFillColor()!=0) continue; // ignore filled histos
            histos[i].histo->SetLineStyle(linestyles[i_linestyle]);
            i_linestyle = (i_linestyle + 1) % n_linestyles;
        }
    }
    
    std::string ratio_num, ratio_denom;
    
    if(draw_ratio){
        string draw_ratio_option = get_option<string>(options, "draw_ratio", "data/bkg");
        size_t p = draw_ratio_option.find('/');
        if(p!=string::npos){
            ratio_num = draw_ratio_option.substr(0, p);
            ratio_denom = draw_ratio_option.substr(p+1);
            trim(ratio_num);
            trim(ratio_denom);
        }
        else{
            ratio_num = "data";
            ratio_denom = "bkg";
        }
    }

    std::unique_ptr<TCanvas> c(new TCanvas("c", "c", 600, 600));
    c->cd();
    const double mainpad_height = 0.67;
    if(draw_ratio){
        c->Divide(1, 2);
        c->cd(1);
        gPad->SetPad(0.0, 0.33, 1.0, 1.0);
        gPad->SetBottomMargin(0.02);
        c->cd(2);
        // make the bottom pad smaller
        gPad->SetPad(0.0, 0.0, 1.0, 0.33);
        gPad->SetTopMargin(0.03); // note: 0 is problematic, as centered y axis labels will then be truncated.
        gPad->SetBottomMargin(0.3);
        
        c->cd(1);
    }
    
    
    // *** axis:
    
    if(ylog){
        gPad->SetLogy();
    }
    double ymax=-1;
    double ymin = numeric_limits<double>::infinity();
    for (size_t i=0; i<histos.size(); i++) {
        // NOTE: pay attention to range set ...
        int ibin0 = histos[0].histo->GetXaxis()->GetFirst();
        int ibin1 = histos[0].histo->GetXaxis()->GetLast();
        for(int k=ibin0; k<=ibin1; ++k){
            double c = histos[i].histo->GetBinContent(k);
            ymax = max(ymax, c);
            if(c > 0){
                ymin = min(ymin, c);
            }
        }
    }
    std::unique_ptr<TH1> h((TH1*)histos[0].histo->Clone());
    h->GetXaxis()->SetTitle(xtext.c_str());
    h->GetYaxis()->SetTitle(ytext.c_str());
    double xlabel_factor = get_option<double>(options, "xlabel_factor", 1.0);
    double ylabel_factor = get_option<double>(options, "ylabel_factor", 1.0);
    h->GetYaxis()->SetTitleOffset(1.5 * ylabel_factor);
    h->GetXaxis()->SetTitleOffset(xlabel_factor);
    if(labelsize > 0){
        h->GetXaxis()->SetLabelSize(labelsize);
        h->GetYaxis()->SetLabelSize(labelsize);
    }
    if(titlesize > 0){
        h->GetXaxis()->SetTitleSize(titlesize);
        h->GetYaxis()->SetTitleSize(titlesize);
    }
    
    if(draw_ratio){
        // hide x axis labels by making them white:
        h->GetXaxis()->SetLabelColor(kWhite);
        h->GetXaxis()->SetTitleColor(kWhite);
    }

    
    if(ylog){
        h->SetMinimum(ymin);
        h->SetMaximum(ymin + exp(1.2 * log(ymax/ymin)));
    }
    else{
        double more_ymax = get_option<double>(options, "more_ymax", 0.0);
        double histomax = ymax * (1.1 + 0.05 * histos.size() + more_ymax);
        h->SetMaximum(histomax); //make room for legend
        h->SetMinimum(0.0);
    }
    if(!std::isnan(options_ymin)){
        h->SetMinimum(options_ymin);
    }
    if(!std::isnan(options_ymax)){
        h->SetMaximum(options_ymax);
    }

    h->Draw("AXIS");
    
    
    // *** histograms:
    std::vector<std::unique_ptr<TObject>> tmp_objects; // will be destroyed upon return ...
    TH1* total_error_hist=0;
    static process_type data("data");
    for (size_t i=0; i<histos.size(); i++) {
        process_type process = histos[i].process;
        if(process==data){
            if(draw_total_error){ // just before drawing data, draw the total error
                //cout << "draw total error" << endl;
                // the E3 option from root connects the *end* of the error bars which is not
                // what we want; we want the whole bin hatched. Therefore, make a new histogram
                // with bin centers at the original bin ends and fill in the error there.
                
                /*TH1* tmp = (TH1*)histos[0]->Clone();
                tmp->SetFillColor(kBlack);
                tmp->SetFillStyle(3004);
                tmp->Draw("SAME E3");
                tmp_objects.push_back(tmp);*/
                vector<double> bins;
                for(int ibin=1; ibin<=histos[0].histo->GetNbinsX(); ++ibin){
                    bins.push_back(histos[0].histo->GetBinLowEdge(ibin));
                    bins.push_back(histos[0].histo->GetBinLowEdge(ibin));
                    bins.push_back(histos[0].histo->GetXaxis()->GetBinUpEdge(ibin));
                    bins.push_back(histos[0].histo->GetXaxis()->GetBinUpEdge(ibin));
                }
                TH1D * tmp = new TH1D("tmp", "tmp", bins.size()-1, &(bins[0]));
                total_error_hist = tmp;
                tmp->SetDirectory(0);
                tmp_objects.emplace_back(tmp);
                int ibin_new = 1;
                for(int ibin=1; ibin<=histos[0].histo->GetNbinsX(); ++ibin){
                    //1. fill the three new bins corresponding to the one old bin ibin:
                    tmp->SetBinContent(ibin_new, histos[0].histo->GetBinContent(ibin));
                    tmp->SetBinError(ibin_new, histos[0].histo->GetBinError(ibin));
                    ++ibin_new;
                    tmp->SetBinContent(ibin_new, histos[0].histo->GetBinContent(ibin));
                    tmp->SetBinError(ibin_new, histos[0].histo->GetBinError(ibin));
                    ++ibin_new;
                    tmp->SetBinContent(ibin_new, histos[0].histo->GetBinContent(ibin));
                    tmp->SetBinError(ibin_new, histos[0].histo->GetBinError(ibin));
                    ++ibin_new;
                    // fill the "gap bin":
                    if(ibin < histos[0].histo->GetNbinsX()){
                        tmp->SetBinContent(ibin_new, histos[0].histo->GetBinContent(ibin));
                        tmp->SetBinError(ibin_new, histos[0].histo->GetBinError(ibin));
                        ++ibin_new;
                    }
                }
                tmp->SetFillColor(kBlack);
                tmp->SetFillStyle(3354);
                tmp->Draw("SAME E3");
            }
            histos[i].histo->SetMarkerStyle(kFullCircle);
            histos[i].histo->SetMarkerSize(1.0);
            histos[i].histo->Draw("SAME E");
        }
        else{
            if(!get_option<string>(histos[i].options, "use_errors", "").empty()){
                histos[i].histo->Draw("SAME E");
            }
            else{
                histos[i].histo->Draw("SAME HIST");
            }
        }
    }

    
    // *** Legend:
    double legend_xshift = get_option<double>(options, "legend_xshift", 0.0);
    string legend_align = get_option<string>(options, "legend_align", "right");
    
    double legend_fontsize = get_option<double>(options, "legend_fontsize", 0.03);
    double legend_linespread = get_option<double>(options, "legend_linespread", 1.1);
    
    const double legend_space_h = 0.02;
    const double legend_space_v = 0.02;
    
    // determine max legend width:
    double legend_maxwidth = 0.0;
    for(auto & h : histos){
        TLatex l(0.0, 0.0, h.legend.c_str());
        l.SetTextSize(legend_fontsize);
        l.SetTextFont(gStyle->GetTextFont());
        //cout << "Width of '" << h.legend << "': " << l.GetXsize() << endl;
        legend_maxwidth = max(legend_maxwidth, l.GetXsize());
    }
    
    
    double legend_x0, legend_x1;
    if(legend_align=="left"){
        legend_x0 = gPad->GetLeftMargin() + legend_space_h + legend_xshift;
        legend_x1 = legend_x0 + legend_maxwidth + 0.1;
    }
    else{
        legend_x0 = 1.0 - gPad->GetRightMargin() - 0.1 - legend_maxwidth - legend_space_h;
        legend_x1 = legend_x0 + legend_maxwidth + 0.1;
    }
    
    unique_ptr<TLegend> leg1(new TLegend(legend_x0,
                                1.0 - gPad->GetTopMargin() - legend_space_v - histos.size() * legend_fontsize  * legend_linespread,
                                legend_x1,
                                1.0 - gPad->GetTopMargin() - legend_space_v,"","brNDC"));
    leg1->SetBorderSize(0);
    leg1->SetTextSize(legend_fontsize);
    leg1->SetTextFont(gStyle->GetTextFont());
    leg1->SetFillColor(10);
    leg1->SetLineColor(1);
    TLegendEntry* entries[histos.size()];
    TLegendEntry * total_error_legentry = 0;
    for (size_t i=0; i<histos.size(); i++) {
        string options = "L";
        if(!get_option<string>(histos[i].options, "fill", "").empty()){
            options = "F";
        }
        else if(histos[i].process == data || !get_option<string>(histos[i].options, "use_errors", "").empty()){ // data or MC with errors
            options = "PL";
            if(total_error_legentry==0 && draw_total_error){
                total_error_legentry = leg1->AddEntry(total_error_hist, "Uncertainty", "F");
            }
        }
        entries[i]=leg1->AddEntry(histos[i].histo.get(), histos[i].legend.c_str(), options.c_str());
        entries[i]->SetLineColor(1);
        //entries[i]->SetLineStyle(1);
        entries[i]->SetLineWidth(3);
        entries[i]->SetTextColor(1);
    }
    leg1->Draw();

    TLatex title_ul;
    title_ul.SetNDC();
    title_ul.SetTextSize(0.035);
    title_ul.DrawLatex(gPad->GetLeftMargin(), 0.93, get_option<string>(options, "title_ul", "").c_str());
    
    double title_l_x0 = gPad->GetLeftMargin() + 0.03;
    if(legend_align=="left") title_l_x0 = 0.6;
    TLatex title_ul1;
    title_ul1.SetNDC();
    title_ul1.SetTextSize(0.035);
    title_ul1.DrawLatex(title_l_x0, 0.88, get_option<string>(options, "title_ul1", "").c_str());
    
    TLatex title_ul2;
    title_ul2.SetNDC();
    title_ul2.SetTextSize(0.035);
    title_ul2.DrawLatex(title_l_x0, 0.84, get_option<string>(options, "title_ul2", "").c_str());
    
    TLatex title_ul3;
    title_ul3.SetNDC();
    title_ul3.SetTextSize(0.035);
    title_ul3.DrawLatex(title_l_x0, 0.79, get_option<string>(options, "title_ul3", "").c_str());
    
    TLatex title_ur;
    title_ur.SetNDC();
    title_ur.SetTextSize(0.035);
    title_ur.SetText(0.0, 0.93, get_option<string>(options, "title_ur", "").c_str());
    title_ur.SetX(1.0 - gPad->GetRightMargin() - title_ur.GetXsize());
    title_ur.Draw();
    
    double xline = get_option<double>(options, "vertical_line", NAN);
    TLine line;
    if(!std::isnan(xline)){
       line.SetLineWidth(3.0);
       line.SetLineColor(kBlack);
       line.DrawLine(xline, ymin, xline, ymax*1.01);
    }
    
    if(debug){
        c->Print(("debug3_"+filename).c_str());
    }
    
    unique_ptr<TH1D> ratio;
    if(draw_ratio){
        c->cd(2);
        ratio.reset(new TH1D("ratio", "ratio", h->GetNbinsX(), h->GetXaxis()->GetXmin(), h->GetXaxis()->GetXmax()));
        string ytext = get_option<string>(options, "ratio_ytext", (ratio_num + " / " + ratio_denom));
        ratio->GetYaxis()->SetTitle(ytext.c_str());
        ratio->GetXaxis()->SetTitle(h->GetXaxis()->GetTitle());
                
        double font_factor = mainpad_height / (1 - mainpad_height);
        ratio->GetYaxis()->SetTitleSize(h->GetYaxis()->GetTitleSize() * font_factor);
        ratio->GetYaxis()->SetLabelSize(h->GetYaxis()->GetLabelSize() * font_factor);
        ratio->GetXaxis()->SetTitleSize(h->GetXaxis()->GetTitleSize() * font_factor);
        ratio->GetXaxis()->SetLabelSize(h->GetXaxis()->GetLabelSize() * font_factor);
        ratio->GetYaxis()->SetTitleOffset(0.5);
        
        // find the indices for the num and denom histos:
        int inum=-1, idenom=-1;
        for(size_t i=0; i<histos.size(); ++i){
            if(ratio_num == nameof(histos[i].process) || (ratio_num == "bkg" && i==0)) inum = i;
            if(ratio_denom == nameof(histos[i].process) || (ratio_denom == "bkg" && i==0)) idenom = i;
        }
        
        const double inf = numeric_limits<double>::infinity();
        double minentry = inf, maxentry = -inf;
        for(int ibin=1; ibin <= ratio->GetNbinsX(); ++ibin){
            double entry = 1.0, entry_error = 1.0;
            
            if(inum >= 0 and idenom >= 0){
                double delta_num = histos[inum].histo->GetBinError(ibin);
                double num = histos[inum].histo->GetBinContent(ibin);
            
                double delta_denom = histos[idenom].histo->GetBinError(ibin);
                double denom = histos[idenom].histo->GetBinContent(ibin);
            
                entry = num / denom;
                // the relative error on the ratio is the quadratic sum of the relative errors of numerator and denominator:
                entry_error = sqrt(pow(delta_num / num, 2) + pow(delta_denom / denom, 2)) * entry;
            
                if(!std::isfinite(entry) || !std::isfinite(entry_error)){
                    entry = 1.0;
                    entry_error = 0.0;
                }
            }
            
            ratio->SetBinContent(ibin, entry);
            ratio->SetBinError(ibin, entry_error);
            minentry = min(minentry, entry - 1.2 * entry_error);
            maxentry = max(maxentry, entry + 1.2 * entry_error);
        }
        // make range a bit larger by rounding to the next 0.1:
        minentry = int(minentry * 10) * 0.1;
        maxentry = int(maxentry * 10 + 1) * 0.1;
        
        double ratio_ymin = get_option<double>(options, "ratio_ymin", NAN);
        double ratio_ymax = get_option<double>(options, "ratio_ymax", NAN);
        double ymin = minentry;
        double ymax = maxentry;
        if(!std::isnan(ratio_ymin)) ymin = ratio_ymin;
        if(!std::isnan(ratio_ymax)) ymax = ratio_ymax;
        ratio->SetMinimum(ymin);
        ratio->SetMaximum(ymax);
        ratio->GetXaxis()->SetNdivisions(h->GetXaxis()->GetNdivisions());
        ratio->Draw("AXIS");
        
        TLine line;
        if(!get_option<string>(options, "ratio_line1", "").empty()){
            line.SetLineWidth(2.0);
            line.SetLineColor(kGray);
            line.DrawLine(ratio->GetXaxis()->GetXmin(), 1.0, ratio->GetXaxis()->GetXmax(), 1.0);
        }
        
        ratio->Draw("ESAME");
    }
    c->Print(filename.c_str());
}


void create_dir(const string & filename){
    size_t p = filename.rfind('/');
    if(p==string::npos) return;
    string path = filename.substr(0, p);
    int res = system(("mkdir -v -p " + path).c_str());
    if(res < 0){
        throw runtime_error("Error executing 'mkdir -p " + path + "'");
    }
}


ProcessHistograms::~ProcessHistograms(){}

ProcessHistogramsTFile::ProcessHistogramsTFile(const std::string & filename, const process_type & pid): process_(pid){
    init_files({filename});
}

ProcessHistogramsTFile::ProcessHistogramsTFile(const std::initializer_list<std::string> & filenames, const process_type & pid): process_(pid){
    init_files(filenames);
}


void ProcessHistogramsTFile::init_files(const std::initializer_list<std::string> & filenames){
    for(const auto & pattern : filenames){
        auto filenames_matched = glob(pattern, false);
        if(filenames_matched.size() != 1) cout << "Note in ProcessHistogramsTFile: pattern '" << pattern << "' matched " << filenames_matched.size() << " files." << endl;
        for(const auto & filename : filenames_matched){
            TFile * file = new TFile(filename.c_str(), "read");
            if(!file->IsOpen()){
                throw runtime_error("could not open file '" + filename + "'");
            }
            files.push_back(file);
        }
    }
    if(files.empty()){
        throw runtime_error("ProcessHistogramsTFile: no files given");
    }
}

// get all names of the objects in the TDirectory dir and fill them into result.
// if 'recursive' is true, will follow into subdirectories and report names
//  there as well (as 'subdir/name')
//
// T must be constructible with a std string, so result can be vector<string>.
template<typename T>
void get_names_of_type(std::vector<T> & result, TDirectory * dir, const char * type, bool recursive, const string & prefix){
    TList * keys = dir->GetListOfKeys();
    TObjLink *lnk = keys->FirstLink();
    while (lnk) {
        TKey * key = static_cast<TKey*>(lnk->GetObject());
        TObject * obj = key->ReadObj();
        if(obj->InheritsFrom(type)){
            result.emplace_back(prefix + key->GetName());
        }
        TDirectory* subdir = dynamic_cast<TDirectory*>(obj);
        if(subdir && recursive){
            get_names_of_type(result, subdir, type, true, prefix + subdir->GetName() + "/");
        }
        lnk = lnk->Next();
    }
}

template
void get_names_of_type<string>(std::vector<string> & result, TDirectory * dir, const char * type, bool recursive, const string & prefix);

std::vector<selection_type> ProcessHistogramsTFile::get_selections(){
    std::vector<selection_type> result;
    get_names_of_type(result, files[0], "TDirectory");
    return result;
}


std::vector<hname_type> ProcessHistogramsTFile::get_hnames(const selection_type & selection){
    TDirectory * sel_dir = files[0]->GetDirectory(nameof(selection).c_str()); // also works with empty selection = toplevel
    if(!sel_dir){
        throw runtime_error("did not find selection '" + nameof(selection) + "' in file '" + files[0]->GetName() + "'");
    }
    std::vector<hname_type> result;
    get_names_of_type(result, sel_dir, "TH1");
    return result;
}


namespace {
    
// returns the name of the last directory, e.g.
// lastdirname("a/b/c/d") is "c"
// returns an empty string if no such directory name exists, e.g.
// lastdirname("b") is ""
string lastdirname(const string & fullname){
    auto lastsl = fullname.rfind('/');
    if(lastsl == string::npos || lastsl == 0) return "";
    auto prelast = fullname.rfind('/', lastsl-1);
    if(prelast == string::npos){
        return fullname.substr(0, lastsl);
    }
    else{
        assert(prelast < lastsl);
        return fullname.substr(prelast+1, lastsl - prelast - 1);
    }
}

}

Histogram ProcessHistogramsTFile::get_histogram(const selection_type & selection, const hname_type & hname){
    Histogram result;
    string name = nameof(selection);
    if(!name.empty()) name += '/';
    name += nameof(hname);
    for(size_t i=0; i<files.size(); ++i){
        TH1* histo = dynamic_cast<TH1*>(files[i]->Get(name.c_str()));
        if(!histo){
            throw runtime_error("Did not find histogram '" + name + "' in file '" + files[i]->GetName() + "'");
        }
        if(!result.histo){
            result.process = process_;
            result.selection = lastdirname(name);
            result.hname = histo->GetName();
            result.histo.reset((TH1*)histo->Clone());
            result.histo->SetDirectory(0);
        }
        else{
            result.histo->Add(histo);
        }
    }
    return move(result);
}

ProcessHistogramsTFile::~ProcessHistogramsTFile(){
    for(auto file : files){
        file->Close();
        delete file;
    }
}

Formatters::Formatters(): all_processes("*"), all_selections("*"){
}

void Formatters::add(const std::string & histos_, const formatter_type & formatter){
    process_type proc = "*";
    selection_type sel = "*";
    std::string histos(histos_);
    // try to interpret string as
    // proc:sel/hname
    // allow that any part is missing or empty.
    size_t pcol = histos.find(':');
    if(pcol!=string::npos){
        if(pcol!=0){
            proc = histos.substr(0, pcol);
        } // otherwise: leave it at proc = all
        histos = histos.substr(pcol + 1);
    }
    size_t psl = histos.find('/');
    if(psl!=string::npos){
        if(psl!=0){
            sel = histos.substr(0, psl);
        } // itherwise, leave it at sel = all
        histos = histos.substr(psl + 1);
    }
    fspec::e_matchmode hname_mode = fspec::mm_any;
    string hname;
    if(!histos.empty()){
        if(histos[histos.size()-1] == '*' && histos.size() != 1){
            // match prefix:
            hname_mode = fspec::mm_prefix;
            hname = histos.substr(0, histos.size()-1);
        }
        else if(histos[0]=='*' && histos.size() != 1){
            // match suffix
            hname_mode = fspec::mm_suffix;
            hname = histos.substr(1);
        }
        else if(histos[0]=='*' && histos.size() == 1){
            // do nothing: leave at match any
        }
        else{ // match full name
            hname_mode = fspec::mm_full;
            hname = histos;
        }
    }
    formatters.emplace_back(proc, sel, hname_mode, hname, formatter);
}


void Formatters::operator()(Histogram & h) const{
    for(const auto & f : formatters){
        if(f.proc != all_processes and f.proc != h.process) continue;
        if(f.sel != all_selections and f.sel != h.selection) continue;
        bool matches = true;
        string hname;
        switch(f.hname_mm){
            case fspec::mm_any:
                matches = true;
                break;
            case fspec::mm_full:
                matches = h.hname == f.hname_full;
                break;
            case fspec::mm_prefix:
                matches = f.hname_substr.compare(0, f.hname_substr.size(), nameof(h.hname), 0, f.hname_substr.size()) == 0;
                break;
            case fspec::mm_suffix:
                hname = nameof(h.hname);
                matches = hname.size() >= f.hname_substr.size() &&
                    // compare all of "this"=f.hname_substr to last part of "that" = hname
                    f.hname_substr.compare(0, f.hname_substr.size(), hname, hname.size() - f.hname_substr.size(), f.hname_substr.size()) == 0;
        }
        if(!matches) continue;
        f.formatter(h);
    }
}

void RebinRange::operator()(Histogram & h){
    if(xmin < h.histo->GetXaxis()->GetXmin() || xmax > h.histo->GetXaxis()->GetXmax()) throw runtime_error("tried to rebin beyond input histogram '" + nameof(h.hname) + "'");
    const double binwidth = (xmax - xmin) / nbins;
    double new_contents[nbins];
    double new_errors[nbins];
    for(int i=0; i<nbins; ++i){
        double bin_xmin = xmin + i * binwidth;
        double bin_xmax = xmin + (i + 1) * binwidth;
        // integrate that range in the current histogram:
        int ibin_low = h.histo->FindBin(bin_xmin);
        int ibin_high = h.histo->FindBin(bin_xmax);
        double c = 0;
        double err2 = 0;
        for(int j=ibin_low; j<=ibin_high; ++j){
            // fraction of this bin to integrate:
            double f = 1.0;
            if(j==ibin_low){
                if(j==ibin_high){ // this means xmin and xmax are both in the same bin ...
                    f = (xmax - xmin) / h.histo->GetBinWidth(j);
                }
                else{
                    f = (h.histo->GetXaxis()->GetBinUpEdge(j) - xmin) / h.histo->GetBinWidth(j);
                }
            }
            else if(j==ibin_high){
                f = (xmax - h.histo->GetXaxis()->GetBinLowEdge(j)) / h.histo->GetBinWidth(j);
            }
            c += f * h.histo->GetBinContent(j);
            err2 += pow(f * h.histo->GetBinError(j), 2);
        }
        new_contents[i] = c;
        new_errors[i] = sqrt(err2);
    }
    // now reset the binning of h->histo:
    h.histo->SetBins(nbins, xmin, xmax);
    for(int i=0; i<nbins; ++i){
        h.histo->SetBinContent(i+1, new_contents[i]);
        h.histo->SetBinError(i+1, new_errors[i]);
    }
}

void RebinVariable::operator() (Histogram & h) {
    new_name = nameof(h.hname)+"_var_bin";
    TH1* rebin_histo = h.histo->Rebin(nbins, new_name.c_str(), bin_arr);
    h.histo->SetBins(nbins, bin_arr);
    for(int i=0; i<nbins; ++i){
        h.histo->SetBinContent(i+1, rebin_histo->GetBinContent(i+1));
        h.histo->SetBinError(i+1, rebin_histo->GetBinError(i+1));
    }
    double xmin = h.histo->GetXaxis()->GetBinLowEdge(1);
    double xmax = h.histo->GetXaxis()->GetBinLowEdge(nbins);
    h.histo->GetXaxis()->SetRangeUser(xmin, xmax);
}


std::vector<Histogram> Plotter::get_formatted_histograms(const std::vector<std::shared_ptr<ProcessHistograms> > & hsources, const selection_type & selection,
                                                        const hname_type & hname){
    vector<Histogram> histograms;
    for(auto & p : hsources){
        Histogram p_histo;
        try{
            if(filter && !(*filter)(p->get_process(), selection, hname)){
                continue;
            }
            p_histo = p->get_histogram(selection, hname);
        }
        catch(std::runtime_error & ex){
            cout << "Warning: Error reading histogram '" << nameof(hname) << "' for process '" <<
                    nameof(p->get_process()) << "' selection '" << nameof(selection) << "': " << ex.what() << endl;
            continue;
        }
        formatters(p_histo);
        histograms.emplace_back(move(p_histo));
        assert(histograms.back().histo);
    }
    return histograms;
}

std::vector<Histogram> Plotter::get_selection_histogram(const std::shared_ptr<ProcessHistograms> & hsource,
                                                        const std::initializer_list<selection_type> & selections_to_compare, const hname_type & hname){
    vector<Histogram> histograms;
    for (const auto & selection : selections_to_compare) {
        Histogram p_histo;
        try{
            if(filter && !(*filter)(hsource->get_process(), selection, hname)){
                continue;
            }
            p_histo = hsource->get_histogram(selection, hname);
        }
        catch(std::runtime_error & ex){
            cout << "Warning: Error reading histogram '" << nameof(hname) << "' for process '" <<
                    nameof(hsource->get_process()) << "' selection '" << nameof(selection) << "': " << ex.what() << endl;
            continue;
        }
        formatters(p_histo);
        histograms.emplace_back(move(p_histo));
        assert(histograms.back().histo);
    }
    return histograms;
}


void Plotter::stackplots(const std::initializer_list<process_type> & processes_to_stack, const std::string & filenamesuffix){
    // get list of histograms:
    static process_type data("data");
    bool add_nonstacked_to_stack = not get_option<string>(options, "add_nonstacked_to_stack", "").empty();
    auto selections = histos[0]->get_selections();
    selections.push_back(""); // toplevel plots like cutflow, control histos
    for(const auto selection : selections){
        auto hnames = histos[0]->get_hnames(selection);
        string s_selection = nameof(selection);
        for(const auto & hname : hnames){
            auto histograms = get_formatted_histograms(histos, selection, hname);
            if(histograms.empty()) continue;
            vector<Histogram> histos_for_draw;
            vector<TH1*> histos_for_stack;
            // build up histos_for_stack to point into histograms[i].histo, in the order given by processes_to_stack
            // and put them into histos_for_draw.
            for(auto & proc : processes_to_stack){
                for(auto & h : histograms){
                    if(h.process==proc){
                        histos_for_stack.push_back(h.histo.get());
                        if(h.histo->GetFillColor()==0){
                            h.histo->SetFillColor(h.histo->GetLineColor());
                        }
                        histos_for_draw.emplace_back(move(h));
                        histos_for_draw.back().options["fill"] = "1";
                        break;
                    }
                }
            }
            makestack(histos_for_stack);
            
            // now append everything not in the stack to the end of histos_to_draw.
            for(auto & h : histograms){
                // h.histo is empty if it was already moved into histos_for_draw above, so skip those:
                if(!h.histo) continue;
                if(add_nonstacked_to_stack && h.process != data){
                    h.histo->Add(histos_for_draw[0].histo.get());
                }
                histos_for_draw.emplace_back(move(h));
                assert(histos_for_draw.back().histo);
            }
            string outfilename = outdir;
            if(!s_selection.empty()){
                outfilename += s_selection + '/';
            }
            outfilename += hname + filenamesuffix + "_stacked.pdf";
            create_dir(outfilename);
            draw_histos(histos_for_draw, outfilename);
        }
    }
}

void Plotter::print_integrals(const selection_type & selection, const hname_type & hname, const std::string & filename, const std::string & title, bool append){
    string filename_full = outdir + filename;
    ofstream out(filename_full.c_str(), append ? ios_base::app : ios_base::trunc);
    if(!out){
        throw runtime_error("print_integrals: could not open '" + filename_full + "'");
    }
    out << title << endl;
    auto histograms = get_formatted_histograms(histos, selection, hname);
    for(auto & h : histograms){
        out << h.latex_legend << ": " << setprecision(10) << h.histo->Integral(0, h.histo->GetNbinsX() + 1) << "  (N_unweighted: " << h.histo->GetEntries() << ")" << endl;
    }
    out << endl;
}

void Plotter::shapeplots(const std::initializer_list<process_type> & processes_to_compare, const string & suffix){
    std::vector<std::shared_ptr<ProcessHistograms> > selected_histos;
    for(auto process : processes_to_compare){
        for(auto & psource : histos){
            if(psource->get_process()==process) selected_histos.push_back(psource);
        }
    }
    auto selections = histos[0]->get_selections();
    for(const auto & selection: selections){
        auto hnames = histos[0]->get_hnames(selection);
        string s_selection = nameof(selection);
        for(const auto & hname : hnames){
            auto histograms = get_formatted_histograms(selected_histos, selection, hname);
            if(histograms.empty()) continue;
            // normalize all and do not fill:
            for(auto & h: histograms){
                h.histo->Scale(1.0 / h.histo->Integral());
                h.histo->SetFillColor(0);
            }
            string outfilename = outdir;
            if(!s_selection.empty()){
                outfilename += s_selection + '/';
            }
            outfilename += hname + suffix + "_shape.pdf";
            create_dir(outfilename);
            draw_histos(histograms, outfilename);
        }
    }
}

void Plotter::cutflow(const hname_type & cutflow_hname, const string & outname){
    ofstream out(outdir + outname);
    // rows are the processes, columns are the selections
    bool first_histo = true;
    for(auto & p : histos){
        Histogram h(p->get_histogram("", cutflow_hname));
        formatters(h);
        if(first_histo){
            // write out header of the table:
            out << "\\begin{tabular}{l}{";
            for(int ibin=1; ibin <= h.histo->GetNbinsX(); ++ibin){
                out << "c";
            }
            out << "}" << endl;
            out << "Process";
            for(int ibin=1; ibin <= h.histo->GetNbinsX(); ++ibin){
                out << " & " << h.histo->GetXaxis()->GetBinLabel(ibin);
            }
            out << "\\\\ \\hline" << endl;
            first_histo = false;
        }
        out << h.latex_legend;
        for(int ibin=1; ibin <= h.histo->GetNbinsX(); ++ibin){
            out << " & " << h.histo->GetBinContent(ibin);
        }
        out << "\\\\" << endl;
    }
}


Plotter::Plotter(const std::string & outdir_, const std::vector<std::shared_ptr<ProcessHistograms> > & histograms, const Formatters & formatters_):
   outdir(outdir_), histos(histograms), formatters(formatters_){
    if(!outdir.empty()){
        if(outdir[outdir.size()-1]!='/'){
            outdir += '/';
        }
    }
    if(histos.empty()) throw runtime_error("histograms are empty");
}


void Plotter::selcomp_plots(const std::initializer_list<selection_type> & selections_to_compare,
                            const std::initializer_list<hname_type> & plots_to_compare, const std::string & outputname){
    if(selections_to_compare.size()==0){
        throw runtime_error("selcomp_plots: empty selections given");
    }
    
    std::vector<hname_type> hnames;
    if (!plots_to_compare.size()){
        hnames = histos[0]->get_hnames(*selections_to_compare.begin());
    }else{
        hnames = plots_to_compare;
    }

    for(const auto & histo : histos){
        for (const auto & hname : hnames){
            auto histograms1 = get_selection_histogram(histo, selections_to_compare, hname);
            auto histograms2 = get_selection_histogram(histo, selections_to_compare, hname);
            if(histograms1.empty()) throw std::runtime_error("no histograms1 for Plotter::selcomp_plots found!");
            if(histograms2.empty()) throw std::runtime_error("no histograms2 for Plotter::selcomp_plots found!");
            // normalize all and set line color to fill color:
            for(auto & h: histograms1){
                h.histo->Scale(1.0 / h.histo->Integral());
            }
            string outfilename1 = outdir + outputname + "/" + nameof(histo->get_process()) + "_" + nameof(hname) + "_selcomp_norm.pdf";
            string outfilename2 = outdir + outputname + "/" + nameof(histo->get_process()) + "_" + nameof(hname) + "_selcomp_abs.pdf";
            create_dir(outfilename1);
            draw_histos(histograms1, outfilename1);
            draw_histos(histograms2, outfilename2);
        }
    }
}


RegexFilter::RegexFilter(const std::string & p_regex, const std::string & s_regex, const std::string & h_regex): p_r(p_regex), s_r(s_regex), h_r(h_regex){
}
    
bool RegexFilter::operator()(const process_type & p, const selection_type & s, const hname_type & h) const{
    using boost::regex_match;
    using boost::match_any;
    return regex_match(p, p_r, match_any ) && regex_match(s, s_r, match_any) && regex_match(h, h_r, match_any );
}

