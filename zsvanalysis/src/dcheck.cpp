#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"

#include "eventids.hpp"

#include <set>

using namespace ra;
using namespace std;


/** \brief Check for and remove duplicates
 * 
 * configured like this
 * \code
 * fdup {
 *    type dcheck
 *    verbose true ; optional, default is false
 *    dataset data ; optional, see below
 * }
 * \endcode
 * 
 * If one or more "dataset" lines are given, the filtering is only applied to datasets names listed; otherwise, filtering is always
 * applied. In case "verbose" is true, information about the duplicates found are printed to stdout (to keep the output
 * manageable, not all duplicates are printed; see code below and modify if needed).
 */
class dcheck: public AnalysisModule {
public:
    
    explicit dcheck(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
    virtual bool is_parallel_safe() const { return false; }
    
    virtual ~dcheck(){
        print();
    }
    
private:
    void print();
    
    typedef pair<int, int> re; //runid, eventid
    
    std::set<std::string> datasets;
    bool verbose;
    
    // set per-dataset:
    std::string current_dataset;
    bool active;
    std::set<re> res;
    int ndup;
    int divisor; // in case of verbose true, print out every XXX duplicate
};

dcheck::dcheck(const ptree & cfg): ndup(0){
    verbose = ptree_get<bool>(cfg, "verbose", false);
    for(auto & setting : cfg){
        if(setting.first == "dataset"){
            datasets.insert(setting.second.data());
        }
    }
}

void dcheck::print(){
    if(ndup > 0){
        cout << "dcheck: found " << ndup << " duplicates in dataset " << current_dataset << endl;
    }
}

void dcheck::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    print();
    current_dataset = dataset.name;
    active = datasets.find(dataset.name) != datasets.end() || datasets.empty();
    res.clear();
    ndup = 0;
    divisor = 1;
}

void dcheck::process(Event & event){
    int runid = event.get<int>("runNo");
    int eventid = event.get<int>("eventNo");
    re current_re{runid, eventid};
    if(res.find(current_re)!=res.end()){
        ++ndup;
        event.set<bool>(ra::fwid::stop, true);
        if(verbose){
            if(ndup % divisor == 0){
                cout << ndup << ". duplicate event found: runid=" << runid << "; eventid=" << eventid << endl;
            }
            if(ndup == 100 || ndup == 1000 || ndup == 10000){
                divisor = ndup;
                cout << "   So many duplicates! Only printing every " << divisor << "th duplicate in this dataset from now on." << endl;
            }
        }
    }
    else{
        res.insert(current_re);
    }
}

REGISTER_ANALYSIS_MODULE(dcheck)
