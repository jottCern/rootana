#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"

#include "base/include/utils.hpp"

#include "zsvtree.hpp"

using namespace ra;
using namespace std;

/** \brief Filter the b candidates vector in the event, only keeping B candidates with certain kinematics
 * 
 * 
 * Configuration:
 * \code
 * type filter_bcands
 * output filtered_bcands ; event member name of the filtered bs and selection
 * aetamin 1.0 ; optional, default: 0.0 (no cut)
 * aetamax 2.0 ; optional, default: infinity
 * ptmin 10.0  ; optional, default: 0.0
 * ptmax 20.0  ; optional, default: infinity
 * ntracksmin 3 ; optional, default is 0
 * \endcode
 */
class filter_bcands: public AnalysisModule {
public:
    
    explicit filter_bcands(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out) override {
        h_output = in.get_handle<vector<Bcand>>(output);
        h_selected_bcands = in.get_handle<vector<Bcand>>("selected_bcands");
    }
    virtual void process(Event & event) override;
    
private:
    float aetamin, aetamax, ptmin, ptmax;
    int ntracksmin;
    std::string output;
    
    Event::Handle<vector<Bcand>> h_selected_bcands, h_output;
};


filter_bcands::filter_bcands(const ptree & cfg){
    aetamin = ptree_get<float>(cfg, "aetamin", 0.0f);
    aetamax = ptree_get<float>(cfg, "aetamax", numeric_limits<float>::infinity());
    ptmin = ptree_get<float>(cfg, "ptmin", 0.0f);
    ptmax = ptree_get<float>(cfg, "ptmax", numeric_limits<float>::infinity());
    ntracksmin = ptree_get<int>(cfg, "ntracksmin", 0);
    output = ptree_get<string>(cfg, "output");
}

void filter_bcands::process(Event & event){
    const auto & bcands = event.get<vector<Bcand>>(h_selected_bcands);
    vector<Bcand> new_bcands;
    
    for(auto & b : bcands){
        if(fabs(b.flightdir.eta()) >= aetamin && fabs(b.flightdir.eta()) < aetamax && b.p4.pt() >= ptmin && b.p4.pt() < ptmax && b.ntracks >= ntracksmin){
            new_bcands.push_back(b);
        }
    }
    event.set(h_output, move(new_bcands));
}


REGISTER_ANALYSIS_MODULE(filter_bcands)
