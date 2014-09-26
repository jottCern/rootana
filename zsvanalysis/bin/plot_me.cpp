#include "plot/include/utils.hpp"

using namespace std;

int main(){
    const string inputdir = "/nfs/dust/cms/user/ottjoc/omega-out/ntuple-latest/recoplots/";
    shared_ptr<ProcessHistograms> top(new ProcessHistogramsTFile({inputdir + "ttbar.root", inputdir + "st*.root"}, "top"));
    shared_ptr<ProcessHistograms> mue(new ProcessHistogramsTFile({inputdir + "mue*.root"}, "mue"));
    
    selection_type selection = "presel_me";
    
    Histogram ptjp = top->get_histogram(selection, "ptjp");
    Histogram ptjp_mcb = top->get_histogram(selection, "ptjp_mcb_r3");
    Histogram ptjp_bc_r2 = top->get_histogram(selection, "ptjp_bc_r3");
    
    Histogram data_ptjp = mue->get_histogram(selection, "ptjp");
    Histogram data_ptjp_bc_r2 = mue->get_histogram(selection, "ptjp_bc_r3");
    
    TH1* h_ptjp = ptjp.histo.get();
    TH1* h_ptjp_mcb = ptjp_mcb.histo.get();
    TH1* h_ptjp_bc_r2 = ptjp_bc_r2.histo.get();
    
    ptjp_mcb.histo->Divide(ptjp.histo.get());
    vector<Histogram> histos;
    histos.emplace_back(move(ptjp_mcb));
    draw_histos(histos, "probe_purity.pdf");
    
    Histogram eff_mc, eff_data;
    eff_mc.histo.reset(new TH1D("eff_mc", "eff_mc", h_ptjp->GetNbinsX(), h_ptjp->GetXaxis()->GetXmin(), h_ptjp->GetXaxis()->GetXmax()));
    eff_data.histo.reset(new TH1D("eff_data", "eff_data", h_ptjp->GetNbinsX(), h_ptjp->GetXaxis()->GetXmin(), h_ptjp->GetXaxis()->GetXmax()));
    eff_mc.process = "mc";
    eff_data.process = "data";
    eff_mc.histo->SetLineColor(kRed);
    eff_mc.options["draw_ratio"] = "data / mc";
    eff_mc.options["ratio_ymin"] = "0.7";
    eff_mc.options["ratio_ymax"] = "1.3";
    eff_mc.options["use_errors"] = "1";
    for(int i=1; i<=h_ptjp->GetNbinsX(); ++i){
        double eff_mc_i = h_ptjp_bc_r2->GetBinContent(i) / (h_ptjp_mcb->GetBinContent(i) * h_ptjp->GetBinContent(i));
        if(!std::isnan(eff_mc_i)){
            eff_mc.histo->SetBinContent(i, eff_mc_i);
            // relative error is quadratic sum of input binomial error + error on purity:
            double n_eff_total = pow(h_ptjp->GetBinContent(i)/ h_ptjp->GetBinError(i), 2);
            double err = eff_mc_i * (1 - eff_mc_i) / n_eff_total; // +  pow(h_ptjp_mcb->GetBinError(i) / h_ptjp_mcb->GetBinContent(i), 2);
            err = sqrt(err) * eff_mc_i;
            eff_mc.histo->SetBinError(i, err);
        }
        double eff_data_i = data_ptjp_bc_r2.histo->GetBinContent(i) / (h_ptjp_mcb->GetBinContent(i) * data_ptjp.histo->GetBinContent(i));
        if(!std::isnan(eff_mc_i)){
            eff_data.histo->SetBinContent(i, eff_data_i);
            double n_eff_total = pow(data_ptjp.histo->GetBinContent(i)/ data_ptjp.histo->GetBinError(i), 2);
            double err = eff_data_i * (1 - eff_data_i) / n_eff_total; // +  pow(h_ptjp_mcb->GetBinError(i) / h_ptjp_mcb->GetBinContent(i), 2);
            err = sqrt(err) * eff_data_i;
            eff_data.histo->SetBinError(i, err);
        }
    }
    double sfsum = 0.0;
    int sfn = 0;
    for(int i=1; i<=h_ptjp->GetNbinsX(); ++i){
        if( eff_data.histo->GetXaxis()->GetBinLowEdge(i) < 30.0) continue;
        if( eff_data.histo->GetXaxis()->GetBinUpEdge(i) > 130.0) break;
        double sf = eff_data.histo->GetBinContent(i) / eff_mc.histo->GetBinContent(i);
        if(!std::isnan(sf)){
            sfsum += sf;
            sfn++;
        }
    }
    cout << sfsum / sfn << endl;
    vector<Histogram> histos2;
    histos2.emplace_back(move(eff_mc));
    histos2.emplace_back(move(eff_data));
    draw_histos(histos2, "eff.pdf");
}

