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
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){}
    virtual void process(Event & event);
private:
    float drmax, ptjmin;
    identifier output;
};

BJetsProducer::BJetsProducer(const ptree & cfg){
    ptjmin = ptree_get<float>(cfg, "ptjmin");
    drmax = ptree_get<float>(cfg, "drmax");
    output = ptree_get<string>(cfg, "output");
}

void BJetsProducer::process(Event & event){
    const auto & jets = event.get<vector<jet>>(id::jets);
    const auto & bcands = event.get<vector<Bcand> >(id::selected_bcands);
    
    vector<jet> bjets;
    
    for(const auto & jet : jets){
        for(const auto & bc : bcands){
            if(jet.p4.pt() > ptjmin && deltaR(jet.p4, bc.flightdir) < drmax){
                bjets.push_back(jet);
                break;
            }
        }
    }
    event.set(output, bjets);
}

REGISTER_ANALYSIS_MODULE(BJetsProducer)
