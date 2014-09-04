#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"

#include "base/include/utils.hpp"

#include "zsvtree.hpp"
#include "eventids.hpp"

using namespace ra;
using namespace std;
using namespace zsv;

/** \brief Filter the mcb vector in the event, only keeping B hadrons with certain kinematics
 * 
 * 
 * Configuration:
 * \code
 * type filter_mcb
 * output_name mcbs_forward ; event member name of the filtered bs and selection
 * event_DRBmin 0.2 ; optional: default: 0.0
 * aetamin 1.0 ; optional, default: 0.0 (no cut)
 * aetamax 2.0 ; optional, default: infinity
 * ptmin 10.0  ; optional, default: 0.0
 * ptmax 20.0  ; optional, default: infinity
 * \endcode
 * 
 * \c event_DRBmin defines the boolean output_name which will be set on the event: it will indicate whether there are NO
 *   two close-by B hadrons with DeltaR smaller than the provided value. Note that this cut is checked before the others
 *   so for making this decision, ALL B hadrons are considered, not only those passing the other cuts.
 * 
 *  All other settings specify the kinematics required for keeping the B hadron, they are used to filter the 'mc_bs' member of the event.
 */
class filter_mcb: public AnalysisModule {
public:
    
    explicit filter_mcb(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out) override {
        h_mc_bs = in.get_handle<vector<mcparticle>>("mc_bs");
        h_output = in.get_handle<vector<mcparticle>>(output);
        h_output_bool = in.get_handle<bool>(output);
    }
    virtual void process(Event & event) override;
    
private:
    
    float event_DRBmin;
    float aetamin, aetamax, ptmin, ptmax;
    std::string output;
    
    Event::Handle<vector<mcparticle>> h_mc_bs, h_output;
    Event::Handle<bool> h_output_bool;
};


filter_mcb::filter_mcb(const ptree & cfg){
    aetamin = ptree_get<float>(cfg, "aetamin", 0.0f);
    aetamax = ptree_get<float>(cfg, "aetamax", numeric_limits<float>::infinity());
    ptmin = ptree_get<float>(cfg, "ptmin", 0.0f);
    ptmax = ptree_get<float>(cfg, "ptmax", numeric_limits<float>::infinity());
    event_DRBmin = ptree_get<float>(cfg, "event_DRBmin", 0.0f);
    output = ptree_get<string>(cfg, "output_name");
}

void filter_mcb::process(Event & event){
    const auto & mc_bs = event.get(h_mc_bs);
    event.set(h_output_bool, true);
    if(event_DRBmin > 0.0f){
        size_t nb = mc_bs.size();
        for(size_t i=0; i<nb; ++i){
            for(size_t j=i+1; j < nb; ++j){
                if(deltaR(mc_bs[i].p4, mc_bs[j].p4) < event_DRBmin){
                    event.set(h_output_bool, false);
                }
            }
        }
    }
    
    vector<mcparticle> new_mc_bs;
    for(auto & mcb : mc_bs){
        if(fabs(mcb.p4.eta()) >= aetamin && fabs(mcb.p4.eta()) < aetamax && mcb.p4.pt() >= ptmin && mcb.p4.pt() < ptmax){
            new_mc_bs.push_back(mcb);
        }
    }
    event.set(h_output, std::move(new_mc_bs));
}


REGISTER_ANALYSIS_MODULE(filter_mcb)
