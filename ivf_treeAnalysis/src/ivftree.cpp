#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"

#include "ivftree.hpp"

using namespace ra;
using namespace std;

//#define ID(name) identifier name(#name)

class ivftree: public AnalysisModule {
public:
    
    explicit ivftree(const ptree & cfg){}
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    
    virtual void process(Event & event){
//         ID(lepton_plus);
//         ID(lepton_minus);
//         ID(zp4);
//         event.set<LorentzVector>(zp4, event.get<lepton>(lepton_plus).p4 + event.get<lepton>(lepton_minus).p4);
    }
};

void ivftree::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    in.declare_event_input<LorentzVector>("p4daughters");
    in.declare_event_input<Point>("recoPos");
    in.declare_event_input<int>("numberTracks");
    
    in.declare_event_input<float>("impact2D");
    in.declare_event_input<float>("impactSig2D");
    in.declare_event_input<float>("impact3D");
    in.declare_event_input<float>("impactSig3D");
    
    in.declare_event_input<GlobalVector>("flightdir");
    
    in.declare_event_input<Flags>("thisProcessFlags");
    in.declare_event_input<Flags>("historyProcessFlags");
    
    in.declare_event_input<CovarianceMatrix>("error");
    
    in.declare_event_input<float>("ndof");
    in.declare_event_input<float>("chi2");
    
    in.declare_event_input<TV>("trackingVertex");
    in.declare_event_input<float>("tvquality");
}

REGISTER_ANALYSIS_MODULE(ivftree)
