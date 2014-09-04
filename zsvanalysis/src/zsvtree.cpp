#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"

#include "base/include/ptree-utils.hpp"

#include "zsvtree.hpp"
#include "eventids.hpp"

using namespace ra;
using namespace std;
using namespace zsv;

class zsvtree: public AnalysisModule {
public:
    
    explicit zsvtree(const ptree & cfg){
        only_re = ptree_get<bool>(cfg, "only_re", false);
        out = ptree_get<bool>(cfg, "out", false);
    }
    
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    
    virtual void process(Event & event){}
    
private:
    bool only_re;
    bool out;
};

void zsvtree::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out_){
    in.declare_event_input<int>("runNo");
    in.declare_event_input<int>("eventNo");
    
    if(!only_re){
        in.declare_event_input<int>("lumiNo");

        in.declare_event_input<bool>("passed_reco_selection");
    
        in.declare_event_input<float>("met");
        in.declare_event_input<float>("met_phi");
    
        in.declare_event_input<lepton>("lepton_plus");
        in.declare_event_input<lepton>("lepton_minus");

        in.declare_event_input<vector<Bcand>>("selected_bcands");
        in.declare_event_input<vector<Bcand>>("additional_bcands");
        
        in.declare_event_input<vector<jet>>("jets");
        
        in.declare_event_input<int>("npv");
    
        // MC info:
        in.declare_event_input<float>("mc_true_pileup");
            
        in.declare_event_input<vector<mcparticle>>("mc_leptons");
        in.declare_event_input<vector<LorentzVector>>("mc_jets");
        
        in.declare_event_input<int>("mc_n_me_finalstate");
        
        in.declare_event_input<vector<mcparticle>>("mc_bs");
        in.declare_event_input<vector<mcparticle>>("mc_cs");
    }
    
    if(out){
        out_.declare_event_output<int>("runNo");
        out_.declare_event_output<int>("eventNo");
        
        if(!only_re){
            out_.declare_event_output<int>("lumiNo");

            out_.declare_event_output<bool>("passed_reco_selection");
        
            out_.declare_event_output<float>("met");
            out_.declare_event_output<float>("met_phi");
        
            out_.declare_event_output<lepton>("lepton_plus");
            out_.declare_event_output<lepton>("lepton_minus");

            out_.declare_event_output<vector<Bcand>>("selected_bcands");
            out_.declare_event_output<vector<Bcand>>("additional_bcands");
            
            out_.declare_event_output<vector<jet>>("jets");
            
            out_.declare_event_output<int>("npv");
        
            // MC info:
            out_.declare_event_output<float>("mc_true_pileup");
                
            out_.declare_event_output<vector<mcparticle>>("mc_leptons");
            out_.declare_event_output<vector<LorentzVector>>("mc_jets");
            
            out_.declare_event_output<int>("mc_n_me_finalstate");
            
            out_.declare_event_output<vector<mcparticle>>("mc_bs");
            out_.declare_event_output<vector<mcparticle>>("mc_cs");
        }
    }
}

REGISTER_ANALYSIS_MODULE(zsvtree)


class calc_zp4: public AnalysisModule {
public:
    
    explicit calc_zp4(const ptree & cfg){}
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
        h_zp4 = in.get_handle<LorentzVector>("zp4");
        h_lepton_plus = in.get_handle<lepton>("lepton_plus");
        h_lepton_minus = in.get_handle<lepton>("lepton_minus");
    }
    virtual void process(Event & event){
        event.set<LorentzVector>(h_zp4, event.get<lepton>(h_lepton_plus).p4 + event.get<lepton>(h_lepton_minus).p4);
    }
    
private:
    Event::Handle<lepton> h_lepton_plus, h_lepton_minus;
    Event::Handle<LorentzVector> h_zp4;
};

REGISTER_ANALYSIS_MODULE(calc_zp4)
