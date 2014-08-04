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
 *    weight_data true ; optional, default is false
 *    trigger dimu     ; optional, default is 'dimu'
 *    isolation loose  ; optional, default is 'loose'
 *    id tight         ; optional, default is 'tight'
 * }
 * \endcode
 * 
 * \c weight_data  : if true, also real data is weighted (you do not want that unless you are doing special cross-checks).
 * 
 * \c trigger, \c isolation, \c id control which scale factors to apply. The only ones supported so far are the ones above.
 *   It is also possible to set them to an empty string to prevent the factor from being applied.
 */
class dimusf: public AnalysisModule {
public:
    
    explicit dimusf(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    bool weight_data;
    std::string trigger, isolation, id;
    
    bool is_real_data;
    
    unique_ptr<TH2F> sf_trigger;
    unique_ptr<TGraphAsymmErrors> sf_id_lt09, sf_id_09t12, sf_id_12t21,  sf_id_21t24;
    unique_ptr<TGraphAsymmErrors> sf_iso_lt09, sf_iso_09t12, sf_iso_12t21, sf_iso_21t24;
    
    TH1D * sf_out;
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

float get_sf(float eta, float pt, const vector<TGraphAsymmErrors*> & sf_eta_pt, const vector<float> & max_abseta){
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
    return tg->GetY()[j];
}

}

dimusf::dimusf(const ptree & cfg){
    weight_data = ptree_get<bool>(cfg, "weight_data", false);
    
    trigger = ptree_get<string>(cfg, "trigger", "dimu");
    isolation = ptree_get<string>(cfg, "isolation", "loose");
    id = ptree_get<string>(cfg, "id", "tight");
    
    if(trigger == "dimu"){
        string trigger_filename = resolve_file("MuHLTEfficiencies_Run_2012ABCD_53X_DR03-2.root");
        sf_trigger = get_th2f(trigger_filename, "DATA_over_MC_Mu17Mu8_Tight_Mu1_20ToInfty_&_Mu2_20ToInfty_with_SYST_uncrt");
    }
    else if(!trigger.empty()){
        throw runtime_error("unknown trigger '" + trigger + "'");
    }
    
    
    if(id=="tight"){
        string id_filename = resolve_file("MuonEfficiencies_Run2012ReReco_53X.root");
        sf_id_lt09 = get_tg(id_filename, "DATA_over_MC_Tight_pt_abseta<0.9");
        sf_id_09t12 = get_tg(id_filename, "DATA_over_MC_Tight_pt_abseta0.9-1.2");
        sf_id_12t21 = get_tg(id_filename, "DATA_over_MC_Tight_pt_abseta1.2-2.1");
        sf_id_21t24 = get_tg(id_filename, "DATA_over_MC_Tight_pt_abseta2.1-2.4");
    }
    else if(!id.empty()){
        throw runtime_error("unknown id '" + id + "'");
    }
    
    if(isolation == "loose"){
        string iso_filename = resolve_file("MuonEfficiencies_ISO_Run_2012ReReco_53X.root");
        sf_iso_lt09 = get_tg(iso_filename, "DATA_over_MC_combRelIsoPF04dBeta<02_Tight_pt_abseta<0.9");
        sf_iso_09t12 = get_tg(iso_filename, "DATA_over_MC_combRelIsoPF04dBeta<02_Tight_pt_abseta0.9-1.2");
        sf_iso_12t21 = get_tg(iso_filename, "DATA_over_MC_combRelIsoPF04dBeta<02_Tight_pt_abseta1.2-2.1");
        sf_iso_21t24 = get_tg(iso_filename, "DATA_over_MC_combRelIsoPF04dBeta<02_Tight_pt_abseta2.1-2.4");
    }
    else if(!isolation.empty()){
        throw runtime_error("unknown isolation '" + isolation + "'");
    }
}

void dimusf::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    is_real_data = dataset.tags.get<bool>("is_real_data");
    if(!is_real_data || weight_data){
        sf_out = new TH1D("dimusf", "dimusf", 100, 0.0, 2.0);
        out.put("dimusf", sf_out);
    }
}

void dimusf::process(Event & event){
    if(is_real_data && !weight_data){
        return;
    }
    const LorentzVector & lp_p4 = event.get<lepton>(id::lepton_plus).p4;
    const LorentzVector & lm_p4 = event.get<lepton>(id::lepton_minus).p4;
    
    float total_sf = 1.0f;
    
    // Trigger scale factor:
    if(sf_trigger){
        double eta_larger, eta_smaller;
        eta_larger = fabs(lp_p4.eta());
        eta_smaller = fabs(lm_p4.eta());
        if(eta_larger < eta_smaller){
            swap(eta_larger, eta_smaller);
        }
        int ibin_trigger = sf_trigger->FindBin(eta_larger, eta_smaller);
        total_sf *= sf_trigger->GetBinContent(ibin_trigger);
    }
    
    // id:
    if(sf_id_lt09){
        float id_sf1 = get_sf(lp_p4.eta(), lp_p4.pt(), {sf_id_lt09.get(), sf_id_09t12.get(), sf_id_12t21.get(), sf_id_21t24.get()}, {0.9f, 1.2f, 2.1f, 2.401f});
        float id_sf2 = get_sf(lm_p4.eta(), lm_p4.pt(), {sf_id_lt09.get(), sf_id_09t12.get(), sf_id_12t21.get(), sf_id_21t24.get()}, {0.9f, 1.2f, 2.1f, 2.401f});
        total_sf *= id_sf1 * id_sf2;
    }
    
    // isolation:
    if(sf_iso_09t12){
        float iso_sf1 = get_sf(lp_p4.eta(), lp_p4.pt(), {sf_iso_lt09.get(), sf_iso_09t12.get(), sf_iso_12t21.get(), sf_iso_21t24.get()}, {0.9f, 1.2f, 2.1f, 2.401f});
        float iso_sf2 = get_sf(lm_p4.eta(), lm_p4.pt(), {sf_iso_lt09.get(), sf_iso_09t12.get(), sf_iso_12t21.get(), sf_iso_21t24.get()}, {0.9f, 1.2f, 2.1f, 2.401f});
        total_sf *= iso_sf1 * iso_sf2;
    }
    
    sf_out->Fill(total_sf);
    event.set_weight(event.weight() * total_sf);
}

REGISTER_ANALYSIS_MODULE(dimusf)
