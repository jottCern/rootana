#include "ra/include/hists.hpp"

#include "zsvtree.hpp"

using namespace ra;
using namespace std;


class TriggerHists: public Hists {
public:
    TriggerHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, InputManager & in, OutputManager & out): Hists(dirname, dataset, out){
        auto h_lepton_offline = in.get_handle<bool>("lepton_offline");
        auto h_lepton_trigger = in.get_handle<bool>("lepton_trigger");
        auto h_lepton_triggermatch = in.get_handle<bool>("lepton_triggermatch");
        
        // fill trigger bit for those that pass the lepton offline selection:
        book_1d_autofill([=](Event & e){
            if(!e.get(h_lepton_offline)) return NAN;
            return e.get(h_lepton_trigger) ? 1.0f : 0.0f;
        }, "lepton_trigger", 2, 0.0, 2.0);
        
        // for the offline AND trigger selected, count how many have trigger matching:
        book_1d_autofill([=](Event & e){
            if(!e.get(h_lepton_offline)) return NAN;
            if(!e.get(h_lepton_trigger)) return NAN;
            return e.get(h_lepton_triggermatch) ? 1.0f : 0.0f;
        }, "lepton_triggermatch", 2, 0.0, 2.0);
    }
    
};


REGISTER_HISTS(TriggerHists)
