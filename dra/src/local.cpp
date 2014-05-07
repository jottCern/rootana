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
    int res = sigaction(SIGCHLD, &sa, 0);
    if(res < 0){
        cerr << "error setting up SIGCHLD handler" << endl;
    }
}
    
volatile sig_atomic_t interrupted = 0;

void sigint_handler(int){
    interrupted = 1;
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
        pb->set("start", nworkers[s_start]);
        pb->set("configure", nworkers[s_configure]);
        pb->set("process", nworkers[s_process]);
        pb->set("close", nworkers[s_close]);
        pb->set("merge", nworkers[s_merge]);
        pb->set("stop", nworkers[s_stop]);
        pb->set("failed", nworkers[s_failed]);
        
        ssize_t ntotal = master->nevents_total();
        if(ntotal == 0){ // this happens in a state transition to a new dataset
            pb->check_autoprint();
            return; 
        }
        size_t nevents_left = master->nevents_left();
        pb->set("events", abs(ntotal) - nevents_left);
        pb->set("mbytes", master->nbytes_read() * 1e-6);
        pb->check_autoprint();
    }
}

void ProgressPrinter::on_idle(const WorkerId & w, const StateGraph::StateId & current_state){}
void ProgressPrinter::on_target_changed(const StateGraph::StateId & new_target){}
void ProgressPrinter::on_restrictions_changed(const std::set<StateGraph::RestrictionSetId> & new_restrictions){}

ProgressPrinter::~ProgressPrinter(){}

// MasterObserver:
void ProgressPrinter::on_dataset_start(const ra::s_dataset & dataset){
    current_dataset = dataset.name;
    pb.reset(new progress_bar("Processing dataset '" + dataset.name + "': events: %(events)10ld (%(events)|rate|8.1f/s); data rate: %(mbytes)|rate|5.2fMB/s; "
            " workers:  %(start)ld S, %(configure)ld C, %(process)ld p,  %(close)ld c, %(merge)ld m,  %(stop)ld s,  %(failed)ld F"));
    pb->set("dataset", dataset.name);
    pb->set("events", 0);
    pb->set("mbytes", 0);
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
    // establish the signal handler  for sigint:
    /*struct sigaction sa;
    sa.sa_handler = &sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_restorer = 0;
    int res = sigaction(SIGINT, &sa, 0);
    if(res != 0){
        cout << "error establishing signal handler" << endl;
        exit(1);
    }*/
    
    // signal handler for sigchld:
    setup_handler();
    // fork:
    auto logger = Logger::get("dra.local_run");
    vector<int> worker_fds;
    vector<int> worker_pids;
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
            try {
                iom.process();
            }
            catch(exception & ex){
                LOG_ERROR("Exception: " << ex.what() << ";' exiting with status 1");
                exit(1);
            }
            catch(...){
                LOG_ERROR("Unknown exception; exiting with status 1");
                exit(1);
            }
            if(w.stopped_successfully()){
                LOG_INFO("Worker stopped successfully; exiting now via exit(0)");
                // and cleanup the logger before ...
                logger.reset();
                exit(0);
            }
            else{
                LOG_ERROR("Worker was not successful; exiting with status 1");
                exit(1);
            }
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
    bool childs_successful = true;
    for(int i=0; i<nworkers; ++i){
        int status = 0;
        pid_t pid2;
        do{
            pid2 = waitpid(worker_pids[i], &status, 0);
        } while(pid2 < 0 && errno == EINTR);
        if(pid2 > 0){
            if(WIFEXITED(status)){
                int exitcode = WEXITSTATUS(status);
                if(exitcode != 0){
                    LOG_ERROR("Worker process " << i << " (pid=" << pid2 << ") exited with status " << exitcode);
                    childs_successful = false;
                }
            }
            else{
                LOG_ERROR("Worker process " << i << " (pid=" << pid2 << ") exited abnormally");
                childs_successful = false;
            }
        }
        else{
            LOG_ERRNO("waitpid");
        }
    }
    if(!childs_successful){
        LOG_THROW("child processes did not exit successfully; check their logs.");
    }
    else{
        LOG_INFO("all child processes exited successfully.")
    }
    if(!master.all_done()){
        LOG_THROW("dataset NOT processed completely.");
    }
}

