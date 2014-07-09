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
    virtual void on_state_transition(const WorkerId & w1, const dc::StateGraph::StateId & from, const dc::StateGraph::StateId & to) override;
    
    // MasterObserver:
    virtual void set_master(Master * master);
    virtual void on_dataset_start(const ra::s_dataset & dataset);
    
    virtual ~ProgressPrinter();
    
private:
    
    const Master * master;
    std::unique_ptr<ra::progress_bar> pb;
    std::string current_dataset;
    std::map<dc::StateGraph::StateId, size_t> nworkers;
    dc::StateGraph::StateId s_start, s_configure, s_process, s_close, s_merge, s_stop, s_failed;
};
    
void local_run(const std::string & cfgfile, int nworkers, const std::shared_ptr<MasterObserver> & observer = std::shared_ptr<MasterObserver>());

}

#endif
