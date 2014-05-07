#include "local.hpp"
#include "worker.hpp"
#include "stategraph.hpp"
#include "base/include/log.hpp"
#include "base/include/utils.hpp"
#include "dc/include/iomanager.hpp"
#include "ra/include/config.hpp"
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <vector>


using namespace std;
using namespace dc;
using namespace dra;
using namespace ra;

namespace {
    
void chld_handler(int){}
    
void setup_handler(){
    struct sigaction sa;
    sa.sa_handler = &chld_handler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // avoid EINTR loop in waitpid
}

}

ProgressPrinter::ProgressPrinter(): master(0){
    const StateGraph & g = get_stategraph();
    s_start = g.get_state("start");
    s_configure = g.get_state("configure");
    s_process = g.get_state("process");
    s_close = g.get_state("close");
    s_merge = g.get_state("merge");
    s_stop = g.get_state("stop");
    s_failed = g.get_state("failed");
}

void ProgressPrinter::on_state_transition(const WorkerId & w1, const StateGraph::StateId & from, const StateGraph::StateId & to){
    StateGraph::StateId invalid_state;
    if(from != invalid_state){
        nworkers[from] -= 1;
    }
    nworkers[to] += 1;
    
    if(pb){ // is false if dataset_start has not been called
        ssize_t ntotal = master->nevents_total();
        if(ntotal == 0) return; // this happens in a state transition to a new dataset
        size_t nevents_left = master->nevents_left();
        pb->set("events_left", nevents_left);
        pb->set("events_total", abs(ntotal));
        if(ntotal >=0){
            pb->set("events_total_suff", "");
        }
        pb->set("start", nworkers[s_start]);
        pb->set("configure", nworkers[s_configure]);
        pb->set("process", nworkers[s_process]);
        pb->set("close", nworkers[s_close]);
        pb->set("merge", nworkers[s_merge]);
        pb->set("stop", nworkers[s_stop]);
        pb->set("failed", nworkers[s_failed]);
        if(nevents_left > 0){
            pb->check_autoprint();
        }
        else{
            pb->print();
        }
    }
}

void ProgressPrinter::on_idle(const WorkerId & w, const StateGraph::StateId & current_state){}
void ProgressPrinter::on_target_changed(const StateGraph::StateId & new_target){}
void ProgressPrinter::on_restrictions_changed(const std::set<StateGraph::RestrictionSetId> & new_restrictions){}

void ProgressPrinter::print_summary(){
    if(pb){
        auto nevents_total = pb->get_int("events_total");
        auto time_total = pb->get_dt();
        pb.reset();
        cout << "Total time for dataset " << current_dataset << ": " << time_total << "s; " << nevents_total / time_total << " evt/s" << endl;
    }
}

ProgressPrinter::~ProgressPrinter(){
    print_summary();
}

// MasterObserver:
void ProgressPrinter::on_dataset_start(const ra::s_dataset & dataset){
    print_summary();
    current_dataset = dataset.name;
    pb.reset(new progress_bar("Processing dataset '" + dataset.name + "'. Events left: %(events_left)10ld / %(events_total)10ld%(events_total_suff)s"
            " Workers:  %(start)ld S, %(configure)ld C, %(process)ld p,  %(close)ld c, %(merge)ld m,  %(stop)ld s,  %(failed)ld F"));
    pb->set("dataset", dataset.name);
    pb->set("events_left", master->nevents_left());
    ssize_t ntotal = master->nevents_total();
    pb->set("events_total", abs(ntotal));
    pb->set("events_total_suff", "+");
    pb->set("start", nworkers[s_start]);
    pb->set("configure", nworkers[s_configure]);
    pb->set("process", nworkers[s_process]);
    pb->set("close", nworkers[s_close]);
    pb->set("merge", nworkers[s_merge]);
    pb->set("stop", nworkers[s_stop]);
    pb->set("failed", nworkers[s_failed]);
    pb->print();
}

void ProgressPrinter::set_master(const Master * master_){
    master = master_;
}

void dra::local_run(const std::string & cfgfile, int nworkers, const std::shared_ptr<MasterObserver> & observer){
    auto logger = Logger::get("dra.local_run");
    vector<int> worker_fds;
    vector<int> worker_pids;
    setup_handler();
    for(int iworker = 0; iworker < nworkers; ++iworker){
        int sockets[2];
        int res = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
        if(res < 0){
            LOG_ERRNO("socketpair for worker " << iworker);
            throw runtime_error("socketpair failed");
        }
        
        pid_t pid = dofork();
        if(pid < 0){
            LOG_ERRNO("fork for worker " << iworker);
        }
        if(pid == 0){
            // the child becomes the worker:
            close(sockets[0]);
            IOManager iom;
            Worker w;
            w.setup(unique_ptr<Channel>(new Channel(sockets[1], iom)));
            iom.process();
            exit(0);
        }
        else{
            // save the rest for the master:
            close(sockets[1]);
            worker_fds.push_back(sockets[0]);
            worker_pids.push_back(pid);
        }
    }
    // the parent becomes the master:
    IOManager iom; // important: iom has to outlibe the master, as the master references channels which reference the iom, so iom has to be there when the channels are closed in the master destructor
    Master master(cfgfile);
    master.add_observer(observer);
    master.start();
    for(int i=0; i<nworkers; ++i){
        master.add_worker(unique_ptr<Channel>(new Channel(worker_fds[i], iom)));
    }
    iom.process();
    LOG_INFO("Master: IOManager returned, waiting for all child processes ... ");
    for(int i=0; i<nworkers; ++i){
        int status = 0;
        pid_t pid2;
        do{
            pid2 = waitpid(worker_pids[i], &status, 0);
        }while(pid2 < 0 && errno == EINTR);
        if(pid2 < 0){
            LOG_ERRNO("waitpid");
        }
    }
    if(!master.all_done()){
        LOG_THROW("Master: dataset NOT processed completely.");
    }
}

