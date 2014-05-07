#ifndef DRA_LOCAL_HPP
#define DRA_LOCAL_HPP

// run the "distributed" analysis locally by using subprocesses.

#include <string>
#include "master.hpp"
#include "ra/include/utils.hpp"

namespace dra {
    
using dc::WorkerId;
    
class ProgressPrinter: public MasterObserver {
public:
    ProgressPrinter();
    // SwarmObserver:
    virtual void on_state_transition(const WorkerId & w1, const dc::StateGraph::StateId & from, const dc::StateGraph::StateId & to);
    virtual void on_idle(const WorkerId & w, const dc::StateGraph::StateId & current_state);
    virtual void on_target_changed(const dc::StateGraph::StateId & new_target);
    virtual void on_restrictions_changed(const std::set<dc::StateGraph::RestrictionSetId> & new_restrictions);
    
    // MasterObserver:
    virtual void set_master(const Master * master);
    virtual void on_dataset_start(const ra::s_dataset & dataset);
    
    virtual ~ProgressPrinter();
    
private:
    void print_summary();
    
    const Master * master;
    std::unique_ptr<ra::progress_bar> pb;
    std::string current_dataset;
    std::map<dc::StateGraph::StateId, size_t> nworkers;
    dc::StateGraph::StateId s_start, s_configure, s_process, s_close, s_merge, s_stop, s_failed;
    
};
    
void local_run(const std::string & cfgfile, int nworkers, const std::shared_ptr<MasterObserver> & observer = std::shared_ptr<MasterObserver>());

}

#endif
