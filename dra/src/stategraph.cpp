#include "stategraph.hpp"
#include "messages.hpp"

using namespace dc;
using namespace dra;

const StateGraph & dra::get_stategraph(){
    static bool init = false;
    static StateGraph sg;
    if(!init){
        auto s_configure = sg.add_state("configure");
        auto s_process = sg.add_state("process");
        auto s_close = sg.add_state("close");
        auto s_merge = sg.add_state("merge");
        
        auto s_start = sg.get_state("start");
        auto s_stop = sg.get_state("stop");
        
        sg.add_state_transition<Configure>(s_start, s_configure);
        //sg.add_state_transition<Configure>(s_configure, s_configure);
        
        sg.add_state_transition<Process>(s_configure, s_process);
        sg.add_state_transition<Process>(s_process, s_process);
        sg.add_state_transition<Process>(s_merge, s_process);
        sg.add_state_transition<Process>(s_close, s_process);
        sg.add_state_transition<Close>(s_process, s_close);
        sg.add_state_transition<Merge>(s_close, s_merge);
        sg.add_state_transition<Merge>(s_merge, s_merge);
        
        sg.add_state_transition<Stop>(s_close, s_stop);
        sg.add_state_transition<Stop>(s_merge, s_stop);
        sg.add_state_transition<Stop>(s_configure, s_stop);
        sg.add_state_transition<Stop>(s_start, s_stop);
        
        auto r_noprocess = sg.add_restriction_set("noprocess");
        sg.add_restriction(r_noprocess, s_configure, s_process);
        sg.add_restriction(r_noprocess, s_process, s_process);
        sg.add_restriction(r_noprocess, s_merge, s_process);
        sg.add_restriction(r_noprocess, s_close, s_process);
        
        auto r_nomerge = sg.add_restriction_set("nomerge");
        sg.add_restriction(r_nomerge, s_close, s_merge);
        sg.add_restriction(r_nomerge, s_merge, s_merge);
        init = true;
    }
    return sg;
}

