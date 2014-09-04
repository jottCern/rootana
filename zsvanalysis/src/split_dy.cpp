#include "ra/include/analysis.hpp"
#include "ra/include/root-utils.hpp"
#include "ra/include/utils.hpp"
#include "ra/include/context.hpp"
#include "ra/include/config.hpp"

#include "zsvtree.hpp"

#include <boost/algorithm/string.hpp>

using namespace ra;
using namespace std;

/** \brief Split the DY-sample into sub-components
 * 
 * 1. bbvis: the "visible phase space" for the B hadrons (note: only include B hadron cuts, not lepton cuts): ==2 B hadrons
 * 2. bbother: >= 2 B hadrons (no kinematic cuts), but not in bbvis
 * 3. cc: >= 2 D hadrons (no kinematic cuts), but not in bbvis / bbother
 * 4. other: all the rest
 * 
 * \code
 *  {
 *    type split_dy
 *    bbvis_mc_bs   selected_mc_bs
 *    select bbvis
 *  }
 * \endcode
 * 
 * \c bbvis_mc_bs is the name of the mcparticle-vector in the event to use for the definition of bbvis: exactly two particles are required
 *    in that vector.
 * 
 * \c select has to be one of 'bbvis', 'bbother', 'cc', 'other'
 *
 * The module will stop the event processing unless the event fulfills the criteria defined by \c select.
 */
class split_dy: public AnalysisModule {
public:
    
    explicit split_dy(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    enum e_select { bbvis, bbother, cc, other };
    e_select select;
    string bbvis_mc_bs;
    
    Event::Handle<bool> h_stop;
    Event::Handle<vector<mcparticle>> h_bbvis_mc_bs, h_mc_bs, h_mc_cs;
};


split_dy::split_dy(const ptree & cfg){
    string cfg_select = ptree_get<string>(cfg, "select");
    if(cfg_select == "bbvis"){
        select = bbvis;
    }
    else if(cfg_select == "bbother"){
        select = bbother;
    }
    else if(cfg_select == "cc"){
        select = cc;
    }
    else if(cfg_select == "other"){
        select = other;
    }
    else{
        throw runtime_error("unknown value for select: '" + cfg_select + "' (allowed: bbvis, bbother, cc, other)");
    }
    bbvis_mc_bs = ptree_get<string>(cfg, "bbvis_mc_bs");
}

void split_dy::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    h_stop = in.get_handle<bool>("stop");
    h_bbvis_mc_bs = in.get_handle<vector<mcparticle>>(bbvis_mc_bs);
    h_mc_bs = in.get_handle<vector<mcparticle>>("mc_bs");
    h_mc_cs = in.get_handle<vector<mcparticle>>("mc_cs");
}


void split_dy::process(Event & event){
    // test for bbvis:
    const auto & selected_bs = event.get(h_bbvis_mc_bs);
    bool passes_bbvis = selected_bs.size() == 2;
    if(select == bbvis){
        event.set<bool>(h_stop, !passes_bbvis);
        return;
    }
    
    const auto & all_mc_bs = event.get<vector<mcparticle>>(h_mc_bs);
    bool passes_bbother = !passes_bbvis and all_mc_bs.size() >= 2;
    if(select == bbother){
        event.set<bool>(h_stop, !passes_bbother);
        return;
    }
    
    const auto & all_mc_cs = event.get<vector<mcparticle>>(h_mc_cs);
    bool passes_cc = !passes_bbvis and !passes_bbother and all_mc_cs.size() >= 2;
    if(select == cc){
        event.set<bool>(h_stop, !passes_cc);
        return;
    }
    
    assert(select == other);
    bool passes_other = !passes_bbvis and !passes_bbother and !passes_cc;
    event.set<bool>(h_stop, !passes_other);
}

REGISTER_ANALYSIS_MODULE(split_dy)


