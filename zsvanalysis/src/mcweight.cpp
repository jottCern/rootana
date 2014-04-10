#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"

using namespace ra;

class mcweight: public AnalysisModule {
public:
    
    explicit mcweight(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    double target_lumi;
    double dataset_lumi;
    bool is_real_data;
};

mcweight::mcweight(const ptree & cfg){
    target_lumi = ptree_get<double>(cfg, "target_lumi");
}

void mcweight::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    is_real_data = dataset.tags.get<bool>("is_real_data");
    if(!is_real_data){
        dataset_lumi = dataset.tags.get<double>("lumi");
    }
}

void mcweight::process(Event & event){
    if(!is_real_data){
        event.set_weight(target_lumi / dataset_lumi);
    }
    else{
        event.set_weight(1.0);
    }
}

REGISTER_ANALYSIS_MODULE(mcweight)
