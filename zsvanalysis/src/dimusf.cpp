#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/root-utils.hpp"

#include "TH2F.h"
#include "TFile.h"
#include "TGraphAsymmErrors.h"

#include "zsvtree.hpp"
#include "eventids.hpp"

#include <list>

using namespace ra;
using namespace std;
using namespace zsv;

/** \brief Apply the dimuon scale factor for the Mu17_Mu8 dimuon trigger and tight id, loose isolation on the event to MC samples.
 * 
 * see https://twiki.cern.ch/twiki/bin/viewauth/CMS/MuonReferenceEffs for the input root files.
 * The root filenames are currently hard-coded to the convention used on the twiki page; the files are
 * searched in all 'searchpath' given in the 'options' section of the config file.
 * 
 * Configuration:
 * \code
 * {
 *    type dimusf
 * }
 * \endcode
 */
class dimusf: public AnalysisModule {
public:
    
    explicit dimusf(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    bool is_real_data;
    
    unique_ptr<TH2F> sf_dimutrigger; // for HLT_Mu17_Mu8
    
    unique_ptr<TGraphAsymmErrors> sf_id_lt09, sf_id_09t12, sf_id_12t21,  sf_id_21t24;
    unique_ptr<TGraphAsymmErrors> sf_iso_lt09, sf_iso_09t12, sf_iso_12t21, sf_iso_21t24;
    
    TH1D * sf_out;
    
    Event::Handle<double> h_weight, h_musf, h_musf_error_id, h_musf_error_iso, h_musf_error_trigger;
    Event::Handle<lepton> h_lepton_minus, h_lepton_plus;
};


namespace{
    
unique_ptr<TH2F> get_th2f(const string & filename, const string & hname){
    TFile f(filename.c_str(), "read");
    if(!f.IsOpen()){
        throw runtime_error("could not open root file '" + filename + "'");
    }
    return gethisto<TH2F>(f, hname);
}


unique_ptr<TGraphAsymmErrors> get_tg(const string & filename, const string & name){
    TFile f(filename.c_str(), "read");
    if(!f.IsOpen()){
        throw runtime_error("could not open root file '" + filename + "'");
    }
    TObject * t = f.Get(name.c_str());
    if(t==0){
        throw runtime_error("did not find '" + name + "' in root file '" + filename + "'");
    }
    TGraphAsymmErrors * t2 = dynamic_cast<TGraphAsymmErrors*>(t);
    if(!t2){
        throw runtime_error("found '" + name + "' in root file '" + filename + "', but it was not of type TGraphAsymErrors");
    }
    unique_ptr<TGraphAsymmErrors> result((TGraphAsymmErrors*)t2->Clone());
    return result;
}

// get pair of (value, error) of scale factor
pair<float, float> get_sf(float eta, float pt, const vector<TGraphAsymmErrors*> & sf_eta_pt, const vector<float> & max_abseta){
    eta = fabs(eta);
    assert(max_abseta.size() == sf_eta_pt.size());
    size_t i = 0;
    while(i < max_abseta.size() && eta > max_abseta[i]) ++i;
    if(i==max_abseta.size()){
        stringstream ss;
        ss << "eta value for muon: " << eta << " out of range for etas: max value is " << max_abseta.back();
        throw runtime_error(ss.str());
    }
    
    TGraphAsymmErrors * tg = sf_eta_pt[i];
    // now, find out index j for the right pt bin and return y value there:
    int j = 0;
    double * x = tg->GetX();
    double * ex_high = tg->GetEXhigh();
    int n = tg->GetN();
    while(j < n && pt > x[j] + ex_high[j]) ++j;
    if(j==n) j = n-1;
    float value = tg->GetY()[j];
    float error = 0.5 * (tg->GetErrorYlow(j) + tg->GetErrorYhigh(j));
    return pair<float, float>(value, error);
}

}

dimusf::dimusf(const ptree & cfg){    
    string trigger_filename = resolve_file("MuHLTEfficiencies_Run_2012ABCD_53X_DR03-2.root");
    //sf_dimutrigger = get_th2f(trigger_filename, "DATA_over_MC_Mu17Mu8_Tight_Mu1_20ToInfty_&_Mu2_20ToInfty_with_SYST_uncrt");
    sf_dimutrigger = get_th2f(trigger_filename, "DATA_over_MC_Mu17Mu8_OR_Mu17TkMu8_Tight_Mu1_20ToInfty_&_Mu2_20ToInfty_with_SYST_uncrt");
    
    string id_filename = resolve_file("MuonEfficiencies_Run2012ReReco_53X.root");
    sf_id_lt09 = get_tg(id_filename, "DATA_over_MC_Tight_pt_abseta<0.9");
    sf_id_09t12 = get_tg(id_filename, "DATA_over_MC_Tight_pt_abseta0.9-1.2");
    sf_id_12t21 = get_tg(id_filename, "DATA_over_MC_Tight_pt_abseta1.2-2.1");
    sf_id_21t24 = get_tg(id_filename, "DATA_over_MC_Tight_pt_abseta2.1-2.4");

    string iso_filename = resolve_file("MuonEfficiencies_ISO_Run_2012ReReco_53X.root");
    sf_iso_lt09 = get_tg(iso_filename, "DATA_over_MC_combRelIsoPF04dBeta<02_Tight_pt_abseta<0.9");
    sf_iso_09t12 = get_tg(iso_filename, "DATA_over_MC_combRelIsoPF04dBeta<02_Tight_pt_abseta0.9-1.2");
    sf_iso_12t21 = get_tg(iso_filename, "DATA_over_MC_combRelIsoPF04dBeta<02_Tight_pt_abseta1.2-2.1");
    sf_iso_21t24 = get_tg(iso_filename, "DATA_over_MC_combRelIsoPF04dBeta<02_Tight_pt_abseta2.1-2.4");
}

void dimusf::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    is_real_data = dataset.tags.get<bool>("is_real_data");
    if(!is_real_data){
        sf_out = new TH1D("dimusf", "dimusf", 100, 0.0, 2.0);
        out.put("dimusf", sf_out);
    }
    h_weight = in.get_handle<double>("weight");
    h_musf = in.get_handle<double>("musf");
    h_lepton_plus = in.get_handle<lepton>("lepton_plus");
    h_lepton_minus = in.get_handle<lepton>("lepton_minus");
    h_musf_error_id = in.get_handle<double>("musf_id");
    h_musf_error_iso = in.get_handle<double>("musf_iso");
    h_musf_error_trigger = in.get_handle<double>("musf_trigger");
}

void dimusf::process(Event & event){
    if(is_real_data){
        return;
    }
    const auto & lp = event.get<lepton>(h_lepton_plus);
    const auto & lm = event.get<lepton>(h_lepton_minus);
    
    double total_sf = 1.0;
    double trigger_sf_error = 0.0, id_sf_error = 0.0, iso_sf_error = 0.0; // relative errors of these parts of the total sf
    
    // Dimuon trigger scale factor. NOTE: currently no scale factor for mu leg of mu-e trigger, but maybe not needed(?)
    if(abs(lp.pdgid) == 13 && abs(lm.pdgid) == 13){
        double eta_larger, eta_smaller;
        eta_larger = fabs(lp.p4.eta());
        eta_smaller = fabs(lm.p4.eta());
        if(eta_larger < eta_smaller){
            swap(eta_larger, eta_smaller);
        }
        int ibin_trigger = sf_dimutrigger->FindBin(eta_larger, eta_smaller);
        double trigger_sf = sf_dimutrigger->GetBinContent(ibin_trigger);
        total_sf *= trigger_sf;
        // relative error on trigger scale factor
        trigger_sf_error = sf_dimutrigger->GetBinError(ibin_trigger) / trigger_sf;
    }
    
    // lp scale factor, if it's a muon:
    if(abs(lp.pdgid)==13){
        auto sf_id_error = get_sf(lp.p4.eta(), lp.p4.pt(), {sf_id_lt09.get(), sf_id_09t12.get(), sf_id_12t21.get(), sf_id_21t24.get()}, {0.9f, 1.2f, 2.1f, 2.401f});
        auto sf_iso_error = get_sf(lp.p4.eta(), lp.p4.pt(), {sf_iso_lt09.get(), sf_iso_09t12.get(), sf_iso_12t21.get(), sf_iso_21t24.get()}, {0.9f, 1.2f, 2.1f, 2.401f});
        total_sf *= sf_id_error.first * sf_iso_error.first;
        id_sf_error += sf_id_error.second / sf_id_error.first;
        iso_sf_error += sf_iso_error.second / sf_iso_error.first;
    }
    if(abs(lm.pdgid)==13){
        auto sf_id_error = get_sf(lm.p4.eta(), lm.p4.pt(), {sf_id_lt09.get(), sf_id_09t12.get(), sf_id_12t21.get(), sf_id_21t24.get()}, {0.9f, 1.2f, 2.1f, 2.401f});
        auto sf_iso_error = get_sf(lm.p4.eta(), lm.p4.pt(), {sf_iso_lt09.get(), sf_iso_09t12.get(), sf_iso_12t21.get(), sf_iso_21t24.get()}, {0.9f, 1.2f, 2.1f, 2.401f});
        total_sf *= sf_id_error.first * sf_iso_error.first;
        // Note: error is assumed to be fully correlated for the two muons, so add linearly (not in quadrature):
        id_sf_error += sf_id_error.second / sf_id_error.first;
        iso_sf_error += sf_iso_error.second / sf_iso_error.first;
    }
    
    sf_out->Fill(total_sf);
    event.set(h_musf, total_sf);
    event.set(h_musf_error_id, id_sf_error);
    event.set(h_musf_error_iso, iso_sf_error);
    event.set(h_musf_error_trigger, trigger_sf_error);
    event.get<double>(h_weight) *= total_sf;
}

REGISTER_ANALYSIS_MODULE(dimusf)
