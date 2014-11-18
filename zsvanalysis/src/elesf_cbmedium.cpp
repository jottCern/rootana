#include "ra/include/analysis.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/event.hpp"

#include "zsvtree.hpp"

using namespace ra;
using namespace std;

// apply electron data/MC efficiency correction factors for cut-based medium ID, see
// https://twiki.cern.ch/twiki/bin/view/Main/EGammaScaleFactors2012#2012_8_TeV_data_53X
//
// works for events with any number of electrons.
// Assumes pt > 20
class elesf_cbmedium: public AnalysisModule {
public:
    
    explicit elesf_cbmedium(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    // scale factor and total (relative) uncertainty
    pair<double, double> getsf(const lepton & lep);
    
    bool is_real_data;
    
    Event::Handle<double> h_weight, h_elesf, h_elesf_error;
    Event::Handle<lepton> h_lepton_minus, h_lepton_plus;
};

elesf_cbmedium::elesf_cbmedium(const ptree & cfg){}

pair<double, double> elesf_cbmedium::getsf(const lepton & lep){
    auto eta = std::abs(lep.sc_eta);
    auto pt = lep.p4.pt();
    typedef pair<double, double> p;
    if(eta < 0.8){
        if(pt < 30) return p(0.986, 0.014);
        else if(pt < 40) return p(1.002, 0.003); 
        else if(pt < 50) return p(1.005, 0.0017);
        else return p(1.004, 0.0042);
    }
    else if(eta < 1.442){
        if(pt < 30) return p(0.959, 0.014);
        else if(pt < 40) return p(0.98, 0.003); 
        else if(pt < 50) return p(0.988, 0.0017);
        else return p(0.988, 0.0046);
    }
    else if(eta < 1.556){
        if(pt < 30) return p(0.967, 0.058);
        else if(pt < 40) return p(0.95, 0.025); 
        else if(pt < 50) return p(0.958, 0.0057);
        else return p(0.966, 0.010);
    }
    else if(eta < 2.0){
        if(pt < 30) return p(0.941, 0.023);
        else if(pt < 40) return p(0.967, 0.0066); 
        else if(pt < 50) return p(0.992, 0.0036);
        else return p(1.0, 0.006);
    }
    else{
        if(pt < 30) return p(1.02, 0.022);
        else if(pt < 40) return p(1.021, 0.0066); 
        else if(pt < 50) return p(1.019, 0.0036);
        else return p(1.022, 0.0066);
    }
}

void elesf_cbmedium::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    is_real_data = dataset.tags.get<bool>("is_real_data");
    h_weight = in.get_handle<double>("weight");
    h_elesf = in.get_handle<double>("elesf");
    h_elesf_error = in.get_handle<double>("elesf_error");
    h_lepton_plus = in.get_handle<lepton>("lepton_plus");
    h_lepton_minus = in.get_handle<lepton>("lepton_minus");
}

void elesf_cbmedium::process(Event & event){
    if(is_real_data){
        return;
    }
    const auto & lp = event.get<lepton>(h_lepton_plus);
    const auto & lm = event.get<lepton>(h_lepton_minus);
    
    double sf = 1.0, sf_relative_error = 0.0;
    if(abs(lp.pdgid)==11){
        auto sf_error = getsf(lp);
        sf *= sf_error.first;
        sf_relative_error += sf_error.second;
    }
    if(abs(lm.pdgid)==11){
        auto sf_error = getsf(lm);
        sf *= sf_error.first;
        sf_relative_error += sf_error.second;
    }
    event.set(h_elesf, sf);
    event.set(h_elesf_error, sf_relative_error);
    event.get<double>(h_weight) *= sf;
}

REGISTER_ANALYSIS_MODULE(elesf_cbmedium)
