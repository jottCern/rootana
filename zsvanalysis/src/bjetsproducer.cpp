#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/root-utils.hpp"

#include "zsvtree.hpp"
#include "eventids.hpp"

using namespace ra;
using namespace std;
using namespace zsv;

/** \brief Apply the dimuon scale factor for the Mu17_Mu8 dimuon trigger and tight id, loose isolation on the event to MC samples.
 * 
 * Configuration:
 * \code
 * {
 *    type BJetsProducer
 *    drmax 0.5 
 *    ptjmin 20
 * }
 * \endcode
 *
 * Puts a vector<jet> in the event calles 'bjets' which are those jets with pt above \c ptjmin which match a B candidate within DR < drmax.
 */
class BJetsProducer: public AnalysisModule {
public:
    
    explicit BJetsProducer(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
private:
    float drmax, ptjmin;
    string s_output;
    Event::Handle<vector<jet>> h_jets, h_output;
    Event::Handle<vector<Bcand>> h_selected_bcands;
};

void BJetsProducer::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    h_output = in.get_handle<vector<jet>>(s_output);
    h_jets = in.get_handle<vector<jet>>("jet");
    h_selected_bcands = in.get_handle<vector<Bcand>>("selected_bcands");
}

BJetsProducer::BJetsProducer(const ptree & cfg){
    ptjmin = ptree_get<float>(cfg, "ptjmin");
    drmax = ptree_get<float>(cfg, "drmax");
    s_output = ptree_get<string>(cfg, "output");
}

void BJetsProducer::process(Event & event){
    const auto & jets = event.get<vector<jet>>(h_jets);
    const auto & bcands = event.get<vector<Bcand> >(h_selected_bcands);
    
    vector<jet> bjets;
    
    for(const auto & jet : jets){
        for(const auto & bc : bcands){
            if(jet.p4.pt() > ptjmin && deltaR(jet.p4, bc.flightdir) < drmax){
                bjets.push_back(jet);
                break;
            }
        }
    }
    event.set(h_output, move(bjets));
}

REGISTER_ANALYSIS_MODULE(BJetsProducer)
