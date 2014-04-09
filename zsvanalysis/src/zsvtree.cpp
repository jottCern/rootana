#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"

#include "base/include/ptree-utils.hpp"

#include "zsvtree.hpp"
#include "eventids.hpp"

using namespace ra;
using namespace std;

class zsvtree: public AnalysisModule {
public:
    
    explicit zsvtree(const ptree & cfg){
        only_re = ptree_get<bool>(cfg, "only_re", false);
    }
    
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    
    virtual void process(Event & event){}
    
private:
    bool only_re;
};

void zsvtree::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    in.declare_event_input<int>("runNo");
    in.declare_event_input<int>("eventNo");
    
    if(!only_re){
        in.declare_event_input<int>("lumiNo");
    
        in.declare_event_input<float>("met");
    
        in.declare_event_input<lepton>("lepton_plus");
        in.declare_event_input<lepton>("lepton_minus");
    
        in.declare_event_input<vector<Bcand>>("selected_bcands");
        //in.declare_event_input<vector<Bcand>>("additional_bcands");
    
        in.declare_event_input<int>("mc_n_me_finalstate");
    
        //in.declare_event_input<vector<jet>>("jets");
    }
}

REGISTER_ANALYSIS_MODULE(zsvtree)


class calc_zp4: public AnalysisModule {
public:
    
    explicit calc_zp4(const ptree & cfg){}
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){}
    virtual void process(Event & event){
        event.set<LorentzVector>(zp4, event.get<lepton>(lepton_plus).p4 + event.get<lepton>(lepton_minus).p4);
    }
};

REGISTER_ANALYSIS_MODULE(calc_zp4)
