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

void noop(int){}

void signal_noop(int signo){
    struct sigaction sa{0};
    sa.sa_handler = &noop;
    int res = sigaction(signo, &sa, 0);
    if(res < 0){
        auto logger = Logger::get("dra.local");
        LOG_THROW("error setting up signal_noop for signal " << signo << " (" << strsignal(signo) << "): " << strerror(errno));
    }
}


void signal_ignore(int signo){
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sa.sa_restorer = 0;
    int res = sigaction(signo, &sa, 0);
    if(res != 0){
        auto logger = Logger::get("dra.local");
        LOG_THROW("signal_ignore " << signo << " (" << strsignal(signo) << "): " << strerror(errno));
    }
}

}

ProgressPrinter::ProgressPrinter(): master(0) {
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
            pb->print();
            return; 
        }
        size_t nevents_left = master->nevents_left();
        pb->set("events", abs(ntotal) - nevents_left);
        pb->set("mbytes", master->nbytes_read() * 1e-6);
        pb->set("files_done", master->nfiles_done());
        pb->set("files_total", master->nfiles_total());
        pb->print();
    }
}

ProgressPrinter::~ProgressPrinter(){}

// MasterObserver:
void ProgressPrinter::on_dataset_start(const ra::s_dataset & dataset){
    current_dataset = dataset.name;
    pb.reset(new progress_bar("Dataset '" + dataset.name + "' %(files_done)5ld/%(files_total)5ld files, %(events)10ld events (%(events)|rate|8.1f/s), %(mbytes)|rate|5.1fMB/s; "
            " workers: %(start)ld S, %(configure)ld C, %(process)ld p,  %(close)ld c, %(merge)ld m,  %(stop)ld s,  %(failed)ld F"));
    pb->set("dataset", dataset.name);
    pb->set("files_done", 0);
    pb->set("files_total", 0);
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

void ProgressPrinter::set_master(Master * master_){
    master = master_;
}

namespace{
    
// returns whether is was successful
bool run_worker(int socket){
    auto logger = Logger::get("dra.local_run");
    IOManager iom;
    Worker w;
    w.setup(unique_ptr<Channel>(new Channel(socket, iom)));
    try {
        iom.process();
    }
    catch(exception & ex){
        LOG_ERROR("Exception during processing: '" << ex.what() << "'");
        return false;
    }
    catch(...){
        LOG_ERROR("Unknown exception during processing");
        return false;
    }
    if(w.stopped_successfully()){
        LOG_INFO("exiting run_worker; Worker stopped successfully.");
        return true;
    }
    else{
        LOG_ERROR("exiting run_worker; Worker was not successful.");
        return false;
    }
}

// wait for the child of the given pid for at most timeout seconds, then kill it with SIGKILL.
// returns true only if the child exits normally (=not due to a signal and with exit code 0).
bool wait_for_child(int pid, float timeout, bool log){
    auto logger = Logger::get("dra.local_run");
    
    int status = 0;
    pid_t wpid = waitpid(pid, &status, WNOHANG);
    if(wpid < 0){
        LOG_ERRNO("waitpid for pid " << pid);
        return false;
    }
    if(wpid == 0){
        // sleep for a while:
        timespec req, rem;
        req.tv_sec = int(timeout);
        req.tv_nsec = int((timeout - req.tv_sec) * 1e9);
        do{
            int res = nanosleep(&req, &rem);
            if(res < 0 && errno == EINTR){ // we have been interrupted sleeping, maybe with SIGCHLD we are waiting for?
                if(waitpid(pid, &status, WNOHANG) == pid){
                    wpid = pid;
                    break;
                }
                req = rem; // sleep for the remaining time the next time
            }
            else{
                // slept for the full time  ...
                break;
            }
        } while(true);
        
        // try again after sleeping:
        if(wpid!=pid){
            wpid = waitpid(pid, &status, WNOHANG);
            if(wpid < 0){
                LOG_ERRNO("waitpid after sleep for child pid " << pid);
                return false;
            }   
        }
        
        // if still not waited for child successfully: kill it:
        if(wpid != pid){
            LOG_INFO("Killing child process " << pid);
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return false;
        }
    }
    
    assert(wpid == pid);
    
    // handle wait status:
    if(WIFEXITED(status)){
        int exitcode = WEXITSTATUS(status);
        if(exitcode != 0){
            if(log){
                LOG_ERROR("Child process with pid " << pid << " exited with status " << exitcode);
            }
            return false;
        }
        else{
            return true;
        }
    }
    else{
        if(log){
            LOG_ERROR("Child process with pid " << pid << " exited abnormally");
        }
        return false;
    }
}

}

bool dra::local_run(const std::string & cfgfile, int nworkers, const std::shared_ptr<MasterObserver> & observer){
    signal_noop(SIGCHLD); // note that signal_ignore (SIGCHLD) would mean we cannot wait for the children.
    signal_ignore(SIGINT);
    signal_ignore(SIGPIPE);
    
    s_config config(cfgfile);
    config.logger.apply(config.options.output_dir);
    
    // create workers via fork:
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
            bool success = run_worker(sockets[1]);
            // and cleanup the logger before ...
            logger.reset();
            exit(success ? 0 : 1);
        }
        else{
            // save the rest for the master:
            LOG_INFO("Forked worker child process pid = " << pid);
            close(sockets[1]);
            worker_fds.push_back(sockets[0]);
            worker_pids.push_back(pid);
            
        }
    }
    // the parent becomes the master:
    IOManager iom; // important: iom has to outlive master, as master references Channels which reference iom, which should still be around in ~Channel called by ~Master
    bool master_ok = false, stopped = false, aborted = false;
    try{ // note: catch errors to wait for childs even if master throws.
        // first make sure we grab the "dangling resources" in form of worker_fds safely into channels. This
        // is to ensure that even if the Master constructor throws, the channels get closed.
        std::vector<std::unique_ptr<Channel>> channels;
        channels.reserve(nworkers);
        for(int i=0; i<nworkers; ++i){
            channels.emplace_back(new Channel(worker_fds[i], iom));
        }
        Master master(cfgfile);
        iom.setup_signal_handler(SIGINT, [&](const siginfo_t &){ 
            if(!stopped){
                cout << endl << "SIGINT: stopping" << endl;
                master.stop();
                stopped = true;
                return;
            }
            else if(!aborted){
                cout << endl << "SIGINT again: aborting" << endl;
                master.abort();
                aborted = true;
                return;
            }
        });
        master.add_observer(observer);
        master.start();
        for(int i=0; i<nworkers; ++i){
            master.add_worker(move(channels[i]));
        }
        iom.process();
        if(master.failed()){
            LOG_ERROR("Master IO processing exited before the dataset was processed completely.");
        }
        else{
          master_ok = true;
        }
    }
    catch(std::exception & ex){
        LOG_ERROR("Exception while running master: " << ex.what());
        master_ok = false;
    }
    
    LOG_INFO("Waiting for all worker processes ... ");
    bool childs_successful = true;
    for(int i=0; i<nworkers; ++i){
        bool child_result = wait_for_child(worker_pids[i], 1.0f, master_ok);
        if(!child_result){
            childs_successful = false;
            if(master_ok){
                LOG_WARNING("Worker process " << worker_pids[i] << " did not exit normally.")
            }
        }
    }
    if(master_ok){
        if(!childs_successful){
            LOG_THROW("worker processes did not exit successfully; check their logs.");
        }
        else{
            LOG_INFO("all worker processes exited successfully.")
        }
    }
    if(!master_ok || stopped){
        cerr << endl << "WARNING: Dataset has NOT been processed completely (see above / log for details)" << endl;
        return false;
    }
    return true;
}

