#include "plot/include/utils.hpp"

#include "TF1.h"
#include "TFitResult.h"

using namespace std;

// make the ratio histograms by taking the sum over all phs and dividing the two histograms
// hf is applied on the summed histograms to allow re-binning, etc.
Histogram make_ratio(const vector<shared_ptr<ProcessHistograms>> & phs, const string & selection, const string & num, const string & denom, const
  std::function<void (Histogram& )> & hf){
    auto n = phs[0]->get_histogram(selection, num);
    auto d = phs[0]->get_histogram(selection, denom);
    for(size_t j=1; j<phs.size(); ++j){
        n.histo->Add(phs[j]->get_histogram(selection, num).histo.get());
        d.histo->Add(phs[j]->get_histogram(selection, denom).histo.get());
    }
    hf(n);
    hf(d);
    n.histo->Divide(n.histo.get(), d.histo.get(), 1.0, 1.0, "B"); // note: use this form of divide to get binomial errors
    return move(n);
}

void apply_csvl_sfb(Histogram & h){
    if(h.hname.find("csv") == string::npos) return; // apply reweighting only on numerator (hname=tp_ptp_csvb), not on denominator (hname=tp_ptp).
    for(int i=1; i<=h.histo->GetNbinsX(); ++i){
        double x = h.histo->GetXaxis()->GetBinCenter(i);
        // see: https://twiki.cern.ch/twiki/pub/CMS/BtagRecommendation53XReReco/SFb-pt_WITHttbar_payload_EPS13.txt
        double sf = 0.997942*((1.+(0.00923753*x))/(1.+(0.0096119*x)));
        h.histo->SetBinContent(i, h.histo->GetBinContent(i) * sf);
    }
}

int main(){
    const string inputdir = "/nfs/dust/cms/user/ottjoc/omega-out/ntuple-latest/recoplots/";
    shared_ptr<ProcessHistograms> top(new ProcessHistogramsTFile({inputdir + "ttbar*.root", inputdir + "st*.root"}, "top"));
    shared_ptr<ProcessHistograms> mue(new ProcessHistogramsTFile({inputdir + "mue*.root"}, "mue"));
    shared_ptr<ProcessHistograms> ee(new ProcessHistogramsTFile({inputdir + "dele*.root"}, "ee"));
    shared_ptr<ProcessHistograms> mm(new ProcessHistogramsTFile({inputdir + "dmu*.root"}, "mm"));

    typedef pair<string, shared_ptr<ProcessHistograms>> sd;
    vector<sd> selection_data = {sd{"presel_me", mue}};
    
    for(const auto & seld: selection_data){
        const auto & selection = seld.first;
        const auto & data = seld.second;
        vector<Histogram> histos;
        histos.emplace_back(make_ratio({top}, selection, "tp_ptp_mcb", "tp_ptp", RebinFactor(2)));
        histos.back().legend = "MC";
        histos.back().options["xtext"] = "p_{T}(probe) [GeV]";
        histos.back().options["ytext"] = "B purity";
        draw_histos(histos, selection + "_probe_purity.pdf");
    
        // use a data-driven way to cross-check b-jet purity in the sample
        histos.clear();
        histos.emplace_back(make_ratio({top}, selection, "tp_ptp_csvb", "tp_ptp", ApplyAll(apply_csvl_sfb, RebinFactor(2)))); // note: apply sfb only to MC
        histos.emplace_back(make_ratio({data}, selection, "tp_ptp_csvb", "tp_ptp", RebinFactor(2)));
        histos[0].process = "mc";
        histos[1].process = "data";
        histos[0].legend = "MC";
        histos[1].legend = "data";
        histos[0].histo->SetLineColor(kRed);
        histos[0].options["use_errors"] = "1";
        histos[1].options["use_errors"] = "1";
        histos[0].options["xtext"] = "p_{T}(probe) [GeV]";
        histos[0].options["ytext"] = "tag rate";
        histos[0].options["draw_ratio"] = "data / mc";
        histos[0].options["ratio_ymin"] = "0.7";
        histos[0].options["ratio_ymax"] = "1.3";
        histos[0].options["ratio_line1"] = "1";
        draw_histos(histos, selection + "_probe_purity_dd.pdf");
        
        Histogram eff_mc, eff_data;
        eff_mc = make_ratio({top}, selection, "tp_ptp_bcb", "tp_ptp", RebinFactor(2));
        eff_data = make_ratio({data}, selection, "tp_ptp_bcb", "tp_ptp", RebinFactor(2));
        eff_mc.process = "mc";
        eff_data.process = "data";
        eff_mc.legend = "MC";
        eff_data.legend = "data";
        eff_mc.histo->SetLineColor(kRed);
        /*eff_mc.options["draw_ratio"] = "data / mc";
        eff_mc.options["ratio_ymin"] = "0.7";
        eff_mc.options["ratio_ymax"] = "1.3";*/
        eff_mc.options["use_errors"] = "1";
        eff_mc.options["xtext"] = "p_{T}(probe) [GeV]";
        eff_mc.options["ytext"] = "tag rate";
        
        histos.clear();
        histos.emplace_back(move(eff_mc));
        histos.emplace_back(move(eff_data));
        draw_histos(histos, selection + "_eff.pdf");
        
        if(selection == "presel_me"){
            TH1D * ratio = (TH1D*)histos[0].histo->Clone();
            ratio->Divide(histos[1].histo.get(), histos[0].histo.get(), 1, 1, "B");
            
            TF1 * f = new TF1("qaud", "[0] * (x - [1]) * (x - [1]) + [2]", 20.0, 200.0);
            f->SetParameter(0, 2e-8);
            f->SetParameter(1, 50.0);
            f->SetParameter(2, 0.9);
            
            auto fr = ratio->Fit(f, "RWLMNS");
            
            f = new TF1("lin", "[0] * (x - 30.0) + [1]", 20.0, 200.0);
            f->SetParameter(0, 2e-5);
            f->SetParameter(1, 0.9);
            
            fr = ratio->Fit(f, "RWLMNS");
            
            f = new TF1("const", "[0]", 20.0, 200.0);
            f->SetParameter(0, 0.9);
            
            fr = ratio->Fit(f, "RWLMS");
            //auto cov = fr->GetCovarianceMatrix();
            //cout << cov(0,0) << ", " << cov(1,1) << ", corr = " << cov(1,0) << endl;
            
            Histogram h;
            h.histo.reset(ratio);
            h.options["use_errors"] = "1";
            h.options["ymin"] = "0.6";
            h.options["ymax"] = "1.4";
            h.options["ytext"] = "SF_{b}";
            h.options["xtext"] = "p_{T}(probe) [GeV]";
            h.legend = "";
            histos.clear();
            histos.emplace_back(move(h));
            draw_histos(histos, selection + "_eff_fit.pdf");
        }
    }
}

