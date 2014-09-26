#include "ra/include/analysis.hpp"
#include "ra/include/root-utils.hpp"
#include "ra/include/utils.hpp"
#include "ra/include/context.hpp"
#include "ra/include/config.hpp"

#include "zsvtree.hpp"

#include <boost/algorithm/string.hpp>

using namespace ra;
using namespace std;

/** \brief Split the ttbar-sample into sub-components with different number of B hadrons
 * 
 * 
 * \code
 *  {
 *    type split_dy
 *    select 2b
 *  }
 * \endcode
 * 
 * \c select has to be one of '2b', or '4b'. '2b' will select all events with 2 or less B hadrons, while 4b will select
 *   all other events; for ttbar this means with at least 4 B hadrons.
 *
 * The module will stop the event processing unless the event fulfills the criteria defined by \c select.
 */
class split_ttbar: public AnalysisModule {
public:
    
    explicit split_ttbar(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    int n_b;

    
    Event::Handle<bool> h_stop;
    Event::Handle<vector<mcparticle>> h_mc_bs;
};


split_ttbar::split_ttbar(const ptree & cfg){
    string cfg_select = ptree_get<string>(cfg, "select");
    if(cfg_select == "2b"){
        n_b = 2;
    }
    else if(cfg_select == "4b"){
        n_b = 4;
    }
    else{
        throw runtime_error("unknown value for select: '" + cfg_select + "' (allowed: 2b, 4b)");
    }
}

void split_ttbar::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    h_stop = in.get_handle<bool>("stop");
    h_mc_bs = in.get_handle<vector<mcparticle>>("mc_bs");
}


void split_ttbar::process(Event & event){
    const auto & mc_bs = event.get<vector<mcparticle>>(h_mc_bs);
    
    if(!((mc_bs.size() <= 2 && n_b == 2) || (mc_bs.size() >= 3 && n_b == 4))){
        event.set<bool>(h_stop, true);
    }

}

REGISTER_ANALYSIS_MODULE(split_ttbar)
