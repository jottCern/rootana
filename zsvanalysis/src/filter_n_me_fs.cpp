#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"

#include "zsvtree.hpp"
#include <list>

using namespace ra;
using namespace std;

class filter_n_me_fs: public AnalysisModule {
public:
    
    explicit filter_n_me_fs(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    std::map<std::string, std::tuple<int, int> > dset_mm; // dataset name to (min,max).
    int nmin, nmax;
    
    Event::Handle<int> h_mc_n_me_finalstate;
    Event::Handle<bool> h_stop;
};


filter_n_me_fs::filter_n_me_fs(const ptree & cfg){
    nmin = nmax = -1;
    for(auto & setting : cfg){
        if(setting.first=="type") continue;
        int nmin = ptree_get<int>(setting.second, "nmin");
        int nmax = ptree_get<int>(setting.second, "nmax");
        dset_mm[setting.first] = make_tuple(nmin, nmax);
    }
}

void filter_n_me_fs::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    nmin = nmax = -1;
    auto it = dset_mm.find(dataset.name);
    if(it!=dset_mm.end()){
        tie(nmin, nmax) = it->second;
    }
    h_stop = in.get_handle<bool>("stop");
    h_mc_n_me_finalstate = in.get_handle<int>("mc_n_me_finalstate");
}

void filter_n_me_fs::process(Event & event){
    if(nmin < 0 and nmax < 0){
        return;
    }
    int n = event.get(h_mc_n_me_finalstate);
    bool passed = (nmin < 0 || n >= nmin) && (nmax < 0 || n <= nmax);
    if(!passed){
        event.set(h_stop, true);
    }
}

REGISTER_ANALYSIS_MODULE(filter_n_me_fs)
