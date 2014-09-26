#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"

#include "zsvtree.hpp"

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
    Event::Handle<double> h_weight, h_output;
    boost::optional<std::string> output;
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
    h_weight = in.get_handle<double>("weight");
    if(output){
        h_output = in.get_handle<double>(*output);
    }
}

void mcweight::process(Event & event){
    double weight = is_real_data ? 1.0 : target_lumi / dataset_lumi;
    event.set(h_weight, weight);
    if(output){
        event.set(h_output, weight);
    }
}

REGISTER_ANALYSIS_MODULE(mcweight)


/** \brief Apply a constant event weight to MC
 */
class weight: public AnalysisModule {
public:
    
    explicit weight(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    double w;
    bool is_real_data;
    Event::Handle<double> h_weight;
};

weight::weight(const ptree & cfg){
    w = ptree_get<double>(cfg, "weight");
}

void weight::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    is_real_data = dataset.tags.get<bool>("is_real_data");
    h_weight = in.get_handle<double>("weight");
}

void weight::process(Event & event){
    if(!is_real_data){
        event.get(h_weight) *= w;
    }
}

REGISTER_ANALYSIS_MODULE(weight)


/** \brief Reweight ttbar for W branching fraction
 * 
 * MadGraph samples are simulated with BR(W->l) = 0.1111, but should be 0.1086  (pdg 2014)
 */
class ttdec_weight: public AnalysisModule {
public:
    
    explicit ttdec_weight(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    bool is_real_data;
    Event::Handle<double> h_weight;
    Event::Handle<int> h_ttdec;
};

ttdec_weight::ttdec_weight(const ptree & cfg){
}

void ttdec_weight::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    is_real_data = dataset.tags.get<bool>("is_real_data");
    h_weight = in.get_handle<double>("weight");
    h_ttdec = in.get_handle<int>("ttdec");
}

void ttdec_weight::process(Event & event){
    static const double simulated_w_brlep = 1. / 9;
    static const double correct_w_brlep = 0.1086;
    
    if(!is_real_data){
        int ttdec = event.get(h_ttdec);
        double & w = event.get(h_weight);
        if(ttdec < 0){
            // unknown: do nothing.
        }
        else if(ttdec == 0){ // allhad
            w *= pow((1.0 - 3 * correct_w_brlep) / (1.0 - 3 * simulated_w_brlep), 2);
        }
        else if(ttdec <= 3){ // lepton+jets
            w *= (1.0 - 3 * correct_w_brlep) / (1.0 - 3 * simulated_w_brlep) * correct_w_brlep / simulated_w_brlep;
        }
        else{ // dilepton:
            w *= pow(correct_w_brlep / simulated_w_brlep, 2);
        }
    }
}

REGISTER_ANALYSIS_MODULE(ttdec_weight)


/** \brief Claculate the b-tagging scale factor for the CSV tagger
 * 
 * configuration:
 * \code
 * {
 *    type btagsf
 *    n_btag_min 1
 *    n_btag_max 2
 *    csv_wp medium
 *    bjets_input bjets_csvm
 *    sf_output bsf12
 * }
 * \endcode
 * 
 * It calculates the b-tagging scale factor for a selection requiring [n_btag_min, n_btag_max] b-jets. These b-jets
 * must be already selected in bjets_input at the given csv working point. the result is written as a double scale factor
 * into the event member with name sf_output.
 */
class btagsf: public AnalysisModule {
public:
    
    explicit btagsf(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    enum e_csv_wp {
        csv_loose, csv_medium, csv_tight
    };
    
    bool is_real_data;
    size_t n_btag_min, n_btag_max;
    
    string bjets_input, sf_output;
    e_csv_wp csv_wp;
    
    Event::Handle<vector<jet>> h_bjets;
    Event::Handle<double> h_sf;
    
    // get the scale factor for this MC event if the selection requires exactly nbtag_required bjets,
    // the b-tagged jets in MC, bjets.
    //
    // nbtag_required <= bjets.size() must hold (otherwise weight is 0 which should be handled outside this method).
    // see: https://twiki.cern.ch/twiki/bin/view/CMS/BTagSFMethods ,  1c) Event reweighting using scale factors only 
    double w(size_t nbtag_required, const std::vector<jet> & bjets);
    
    // the argument is a jet that has been tagged in MC.
    double sfb(const jet & bjet);
};

double btagsf::sfb(const jet & bjet){
    // https://twiki.cern.ch/twiki/pub/CMS/BtagPOG/SFb-pt_WITHttbar_payload_EPS13.txt
    double x = bjet.p4.pt();
    if(x < 20.0) x = 20.0;
    if(x > 800.0) x = 800.0;
    switch(csv_wp){
        case csv_loose:
            return 0.997942*((1.+(0.00923753*x))/(1.+(0.0096119*x)));
        case csv_medium:
            return (0.938887+(0.00017124*x))+(-2.76366e-07*(x*x));
        case csv_tight:
            return (0.927563+(1.55479e-05*x))+(-1.90666e-07*(x*x));
    }
    throw logic_error("btagsf::sfb logic error");
}

namespace {
    
    // generate all possible ways to select k elements from the vector 'all_elements', so the callback will be called (n over k).
    // This is suitable for selecting k elements in case the order does not matter.
    // Note that when calling the callback, the k elements appear in the same order as they have been passed i all_elements.
    template<typename T>
    void combination(size_t k, const vector<T> & all_elements, function<void (const vector<T> &)> callback, vector<T> & start){
        //cout << k << " from " << all_elements.size() << endl;
        assert(k <= all_elements.size());
        if(k==0){
            callback(start);
        }
        else if(k == all_elements.size()){
            start.insert(start.end(), all_elements.begin(), all_elements.end());
            callback(start);
            start.resize(start.size() - k);
        }else{
            vector<T> remaining_elements(all_elements.begin() + 1, all_elements.end());
            // two cases:
            // 1. first element is not in, so pick k from the ramining ones:
            combination(k, remaining_elements, callback, start);
            // 2. first element is in, so pick k-1 from the reamining ones:
            start.push_back(all_elements[0]);
            combination(k - 1, remaining_elements, callback, start);
            start.pop_back();
        }
    }
    
}


double btagsf::w(size_t nbtag_required, const std::vector<jet> & bjets){
    assert(nbtag_required <= bjets.size());
    double result = 0.0;
    // go through all combinations of selecting nbtag_required jets from bjets; those
    // are the ones actually b-tagged in data and get a factor SFb, while all other get a factor 1- SFb.
    // We select the combinations based on the indices:
    std::vector<size_t> all_indices(bjets.size());
    for(size_t i=0; i<bjets.size(); ++i){
        all_indices[i] = i;
    }
    std::function<void (const vector<size_t> & )> callback = [&bjets,&result,this](const vector<size_t> & bjet_indices) -> void{
        double factor = 1.0;
        size_t i_bjet_indices = 0;
        for(size_t i=0; i<bjets.size(); ++i){
            if(i_bjet_indices < bjet_indices.size() && bjet_indices[i_bjet_indices] == i){
                factor *= sfb(bjets[i]);
                ++i_bjet_indices;
            }
            else{
                factor *= 1 - sfb(bjets[i]);
            }
        }
        result += factor;
    };
    vector<size_t> start;
    start.reserve(nbtag_required);
    combination(nbtag_required, all_indices, callback, start);
    return result;
}

btagsf::btagsf(const ptree & cfg){
    n_btag_min = ptree_get<size_t>(cfg, "n_btag_min");
    n_btag_max = ptree_get<size_t>(cfg, "n_btag_max");
    auto s_csv_wp = ptree_get<string>(cfg, "csv_wp");
    if(s_csv_wp == "loose"){
        csv_wp = csv_loose;
    }
    else if(s_csv_wp == "medium"){
        csv_wp = csv_medium;
    }
    else if(s_csv_wp == "tight"){
        csv_wp = csv_tight;
    }
    else{
        throw runtime_error("invalid csv_wp given");
    }
    bjets_input = ptree_get<string>(cfg, "bjets_input");
    sf_output = ptree_get<string>(cfg, "sf_output");
}

void btagsf::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    is_real_data = dataset.tags.get<bool>("is_real_data");
    h_bjets = in.get_handle<vector<jet>>(bjets_input);
    h_sf = in.get_handle<double>(sf_output);
}

void btagsf::process(Event & event){
    if(is_real_data){
        event.set(h_sf, 1.0);
        return;
    }
    const auto & bjets = event.get(h_bjets);
    // in case there are fewer bjets in MC than required, this event does not contibute:
    if(bjets.size() < n_btag_min){
        event.set(h_sf, 0.0);
        return;
    }
    // if we're here, this MC event contributes with non-zero weight. Add sf
    // for all allowed n_bjets:
    double total_sf;
    size_t current_n_btag_max = min(n_btag_max, bjets.size());
    // for example, bjets.size() == 2, and we require >= 1 bjets in our selection. In the notation of the twiki page, we have
    //   total_sf = w(1|2) + w(2|2)
    // or, alternatively:
    //   total_sf = 1 - w(0|2).
    // If we would ask for exaclty 1 b-jet, e would have either:
    //   total_sf = w(1|2)
    // or:
    //   total_sf = 1 - w(0|2) - w(2|2)
    // To save some time, always use the method with fewer evaluations of 'w'.
    size_t n_eval_method1 = current_n_btag_max - n_btag_min + 1;
    size_t n_eval_method2 = n_btag_min;
    if(bjets.size() > n_btag_max){
        n_eval_method2 += bjets.size() - n_btag_max;
    }
        
    if(n_eval_method1 < n_eval_method2){
        total_sf = 0.0;
        for(size_t n_req = n_btag_min; n_req <= current_n_btag_max; ++n_req){
            total_sf += w(n_req, bjets);
        }
    }
    else{
        // calculate 1 - w(X|nbjets) where X runs over the complement of [n_btag_min, n_btag_max] in [0, bjets.size()]:
        total_sf = 1.0;
        for(size_t n_req = 0; n_req < n_btag_min; ++n_req){
            total_sf -= w(n_req, bjets);
        }
        for(size_t n_req = n_btag_max + 1; n_req <= bjets.size(); ++n_req){
            total_sf -= w(n_req, bjets);
        }
    }
    event.set(h_sf, total_sf);
}

REGISTER_ANALYSIS_MODULE(btagsf)

