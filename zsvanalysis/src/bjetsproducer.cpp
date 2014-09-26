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

/** \brief Select bjets based on jets matching to B candidates
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
 * Puts a new vector<jet> in the event called 'bjets' which are those jets with pt above \c ptjmin which match a B candidate within DR < \c drmax.
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
    h_jets = in.get_handle<vector<jet>>("jets");
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


/** \brief Select bjets based on CSV discriminator value
 * 
 * Configuration:
 * \code
 * {
 *    type BJetsProducerCSV
 *    wp medium
 *    ptjmin 20
 *    etajmax 2.4
 *    output 
 * }
 * \endcode
 *
 * Puts a new vector<jet> in the event with the name given by output jets with pt above \c ptjmin and |eta| < \c etajmax which are selected
 * by the CSV algorithm.
 */
class BJetsProducerCSV: public AnalysisModule {
public:
    
    explicit BJetsProducerCSV(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
private:
    double ptjmin, etajmax, csvmin;
    string s_output;
    Event::Handle<vector<jet>> h_jets, h_output;
};

void BJetsProducerCSV::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    h_output = in.get_handle<vector<jet>>(s_output);
    h_jets = in.get_handle<vector<jet>>("jets");
}

BJetsProducerCSV::BJetsProducerCSV(const ptree & cfg){
    ptjmin = ptree_get<double>(cfg, "ptjmin");
    etajmax = ptree_get<double>(cfg, "etajmax");
    auto wp = ptree_get<string>(cfg, "wp");
    // see https://twiki.cern.ch/twiki/bin/viewauth/CMS/BTagPerformanceOP
    if(wp=="loose"){
        csvmin = 0.244;
    }
    else if(wp == "medium"){
        csvmin = 0.679;
    }
    else if(wp == "tight"){
        csvmin = 0.898;
    }
    else{
        throw runtime_error("unknown wp '" + wp + "'");
    }
    s_output = ptree_get<string>(cfg, "output");
}

void BJetsProducerCSV::process(Event & event){
    const auto & jets = event.get<vector<jet>>(h_jets);
    
    vector<jet> bjets;
    
    for(const auto & jet : jets){
        if(jet.p4.pt() > ptjmin && fabs(jet.p4.eta()) < etajmax && jet.btag > csvmin){
            bjets.push_back(jet);
        }
    }
    event.set(h_output, move(bjets));
}

REGISTER_ANALYSIS_MODULE(BJetsProducerCSV)

