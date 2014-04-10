#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"

using namespace ra;
using namespace std;


/** \brief Apply an event weight based on data and MC luminosity
 * 
 * Uses the dataset tag "lumi" which should be set to the integrated luminosity in pb^{-1} of the resp. dataset.
 * This module is then configured with the parameter "target_lumi" which is the luminosity of data to reweight the MC to.
 * 
 * Real data (dataset tag "is_real_data") is not reweighted and its weight is set to 1.0.
 * 
 * An optional setting "output" can be specified. In this case, the lumi weight is saved in addition in a double of that name
 * in the event container.
 */
class mcweight: public AnalysisModule {
public:
    
    explicit mcweight(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    double target_lumi;
    double dataset_lumi;
    bool is_real_data;
    boost::optional<identifier> output;
};

mcweight::mcweight(const ptree & cfg){
    target_lumi = ptree_get<double>(cfg, "target_lumi");
    output = cfg.get_optional<string>("output");
}

void mcweight::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    is_real_data = dataset.tags.get<bool>("is_real_data");
    if(!is_real_data){
        dataset_lumi = dataset.tags.get<double>("lumi");
    }
}

void mcweight::process(Event & event){
    double weight = is_real_data ? 1.0 : target_lumi / dataset_lumi;
    event.set_weight(weight);
    if(output){
        event.set<double>(*output, weight);
    }
}

REGISTER_ANALYSIS_MODULE(mcweight)
