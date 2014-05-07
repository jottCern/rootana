#include "swarm_manager.hpp"
#include "fwmessages.hpp"

#include <sys/resource.h>
#include <iomanip>

using namespace dc;
using namespace std;
using namespace std::placeholders;

SwarmObserver::~SwarmObserver(){}

SwarmManager::SwarmManager(const StateGraph & g, const failed_handler_type & failed_handler_): logger(Logger::get("dc.SwarmManager")), graph(g), failed_handler(failed_handler_),
    aborting(false){
}

SwarmManager::Worker::Worker(unique_ptr<Channel> c_, StateGraph::StateId state_): c(move(c_)), last_state(state_), state(state_), active(false),
   requested_state(WorkerResponse::rs_work){
}


void SwarmManager::check_connections() const{
    const StateGraph::StateId invalid_state;
    if(target_state==invalid_state){
        LOG_THROW("target state is invalid");
    }
    set<StateGraph::StateId> all_ids = graph.all_states();
    for(const StateGraph::StateId & state : all_ids){
        auto next = graph.next_states(state);
        for(auto & n : next){
            if(message_generators.find(make_pair(state, n)) == message_generators.end()){
                LOG_THROW("check_connections: missing connection for state transition " + graph.name(state) + " -> " + graph.name(n))
            }
        }
    }
}


WorkerId SwarmManager::add_worker(unique_ptr<Channel> c){
    LOG_DEBUG("add worker");
    WorkerId result = worker_ids.create_anonymous();
    Worker w(move(c), graph.get_state("start"));
    w.c->set_error_handler(bind(&SwarmManager::on_error, this, result, _1));
    workers.insert(make_pair(result, move(w)));
    const StateGraph::StateId invalid_state;
    for(auto & observer : observers){
        observer->on_state_transition(result, invalid_state, w.state);
    }
    give_work_to(result);
    return result;
}

void SwarmManager::set_failed(WorkerId wid){
    auto w_it = workers.find(wid);
    assert(w_it!=workers.end());
    Worker & w = w_it->second;
    w.active = false;
    StateGraph::StateId last_state = w.state;
    w.state = graph.get_state("failed");
    for(auto & observer : observers){
        observer->on_state_transition(wid, last_state, w.state);
    }
    failed_handler(wid, last_state);
}

namespace {
    
    // get the next state if we want to go from current_state to target_state, taking into account the active_restrictions.
    // If it's currently not possible to go there, an invalid state is returned.
    StateGraph::StateId get_next_state(const StateGraph & graph, const StateGraph::StateId & current_state, const StateGraph::StateId & target_state,
                                       const std::set<StateGraph::RestrictionSetId> & active_restrictions){
        const std::set<StateGraph::StateId> all_states = graph.all_states();
        std::set<StateGraph::StateId> visited_states;
        vector<vector<StateGraph::StateId> > paths;
        
        // start with a single path only containing the current state:
        paths.push_back({current_state});
        
        // iteratively make all paths one node longer until we found the target state. For making
        // the paths longer, do not consider any node visited already by any Path (up to the previous iteration).
        
        // loop over the number of hops. Note that the number of hops is strictly limited by the total number of states:
        for(size_t ihop = 0; ihop < all_states.size(); ++ihop){
            // at this point, visited_states should contain all nodes up to and including paths[*][ihop-1]
            // (note that not including paths[*][ihop] allows searching for loops for which current_state == target_state).
            vector<vector<StateGraph::StateId> > new_paths;
            for(size_t ip = 0; ip < paths.size(); ++ip){
                StateGraph::StateId state = paths[ip].back();
                std::set<StateGraph::StateId> next_states = graph.next_states(state);
                // filter out next states with active_restrictions:
                std::set<StateGraph::StateId> allowed_next_states(next_states);
                for(auto & r : active_restrictions){
                    for(auto & s : next_states){
                        if(graph.is_restricted(r, state, s)){
                            allowed_next_states.erase(s);
                        }
                    }
                }
                next_states = allowed_next_states;
                // now filter out those we already visited in the current paths:
                for(auto & s : next_states){
                    if(visited_states.find(s) != visited_states.end()){
                        allowed_next_states.erase(s);
                    }
                }
                // now use all allowed_next_states to build several new paths out of paths[ip]:
                for(auto & s: allowed_next_states){
                    new_paths.push_back(paths[ip]);
                    new_paths.back().push_back(s);
                    // if we found the target state, return the beginning of this path:
                    if(s==target_state){
                        return new_paths.back()[1];
                    }
                }
            }
            paths = move(new_paths);
            if(paths.size()==0) break; // did not find any valid path, e.g. because we already visited everything.
            // in visited_states, add all last nodes in the paths:
            for(auto & p : paths){
                visited_states.insert(p.back());
            }
        }
        StateGraph::StateId invalid_state;
        return invalid_state;
    }
    
};

void SwarmManager::give_work_to(WorkerId wid){
    LOG_DEBUG("entering give_work_to " << wid.id());
    if(aborting){
        LOG_WARNING("give_work_to called while aborting");
    }
    auto w_it = workers.find(wid);
    assert(w_it != workers.end());
    Worker & w = w_it->second;
    assert(w.state!=graph.get_state("failed") && w.state!=graph.get_state("stop") && !w.active);
    
    const StateGraph::StateId invalid_state;
    StateGraph::StateId target = w.requested_state == WorkerResponse::rs_stop ? graph.get_state("stop") : target_state;
    StateGraph::StateId next = get_next_state(graph, w.state, target, active_restrictions);  
    
    if(next==invalid_state){
        LOG_DEBUG("give_work_to: no next state found for worker " << wid.id() << " (state: " << graph.name(w.state) << "); leaving worker idle");
        for(auto & observer : observers){
            observer->on_idle(wid, w.state);
        }
    }
    else{
        // generate a message for the found state transition and send it to the worker:
        LOG_DEBUG("give_work_to: initiating state transition for worker " << wid.id() << ": " << graph.name(w.state) << " -> " << graph.name(next));
        w.last_state = w.state;
        w.state = next;
        w.active = true;
        std::unique_ptr<Message> msg = (*message_generators[make_pair(w.last_state, w.state)])(wid);
        //assert(w.c->can_write());
        w.c->write(*msg, bind(&SwarmManager::on_out_message, this, wid));
        for(auto & observer : observers){
            observer->on_state_transition(wid, w.last_state, w.state);
        }
    }
}


void SwarmManager::on_in_message(WorkerId wid, std::unique_ptr<Message> message){
    LOG_DEBUG("got message from worker " << wid.id());
    auto w_it = workers.find(wid);
    Worker & w = w_it->second;
    if(!w.active){
        LOG_THROW("got message from worker " << wid.id() << " although it is not active");
    }
    w.active = false;
    WorkerResponse * wr = dynamic_cast<WorkerResponse*>(message.get());
    if(wr==0){
        LOG_ERROR("got message from worker " << wid.id() << " which is not a WorkerResponse; closing connection and marking this worker as failed.");
        w.c->close();
        set_failed(wid);
        return;
    }
    //StateGraph::PrioritySetId response_pset(wr->priority_set);
    if(w.requested_state != wr->requested_state){
        LOG_DEBUG("worker " << wid.id() << " switched to requested state " << wr->requested_state);
        w.requested_state = wr->requested_state;
    }
    auto rc = result_callbacks.find(w.state);
    if(rc != result_callbacks.end()){
        LOG_DEBUG("calling result callback for worker " << wid.id());
        rc->second(wid, move(wr->response_details));
    }
    // give it work now. Note that the callback can call give_work_to already (e.g. via calling abort()), so only
    // distribute work if the callback did not do that already:
    if(!w.active){
        if(w.state == graph.get_state("stop") || w.state==graph.get_state("failed")){
            LOG_DEBUG("on_in_message: got message from worker " << wid.id() << " which is in terminal state " << graph.name(w.state) << "; not giving it any work");
            // do nothing in this case ...
        }
        else{
            give_work_to(wid);
        }
    }
}

void SwarmManager::on_out_message(WorkerId wid){
    auto w_it = workers.find(wid);
    assert(w_it != workers.end());
    Worker & w = w_it->second;
    w.c->set_read_handler(bind(&SwarmManager::on_in_message, this, wid, _1));
}

void SwarmManager::on_error(WorkerId wid, int errorcode){
    LOG_DEBUG("on_error: got communication error for worker " << wid.id() << "; errorcode: " << errorcode << " (" << strerror(errorcode) << ")");
    if(aborting){
        LOG_DEBUG("  ignoring error as we are in ABORTING state");
        return;
    }
    auto w_it = workers.find(wid);
    assert(w_it != workers.end());
    Worker & w = w_it->second;
    if(w.state == graph.get_state("failed")){
        LOG_DEBUG("  ignoring error as worker is in failed state anyway");
        return;
    }
    // handle graceful shutdown:
    if(!w.active and w.state == graph.get_state("stop") and errorcode == ECONNRESET){
        LOG_DEBUG("  ignoring error as worker is stopping");
        return;
    }
    //otherwise: set to a failed state (note that channel is closed anyway).
    LOG_WARNING("Error communicating with worker " << wid.id() << ": " << strerror(errorcode) << "; setting to failed state and calling failed_handler");
    set_failed(wid);
}

void SwarmManager::abort(){
    aborting = true;
    const auto s_failed = graph.get_state("failed");
    const auto s_stop = graph.get_state("stop");
    target_state = s_failed;
    for(auto & observer : observers){
        observer->on_target_changed(target_state);
    }
    for(auto & i_w : workers){
        Worker & w = i_w.second;
        if(w.active || (w.state != s_failed && w.state != s_stop)){
            w.last_state = w.state;
            w.state = s_failed;
            w.active = false;
            w.c->close();
            for(auto & observer : observers){
                observer->on_state_transition(i_w.first, w.last_state, s_failed);
            }
        }
    }
}

void SwarmManager::set_target_state(const StateGraph::StateId & target_state_){
    if(target_state != target_state_){
        target_state = target_state_;
        for(auto & observer : observers){
            observer->on_target_changed(target_state);
        }
        for(auto & i_w : workers){
            if(!i_w.second.active && i_w.second.state != graph.get_state("failed") && i_w.second.state != graph.get_state("stop")){
                give_work_to(i_w.first);
            }
        }
    }
}

void SwarmManager::activate_restriction_set(const StateGraph::RestrictionSetId & rset){
    auto res = active_restrictions.insert(rset);
    if(res.second){
        for(auto & observer : observers){
            observer->on_restrictions_changed(active_restrictions);
        }
    }
}

void SwarmManager::deactivate_restriction_set(const StateGraph::RestrictionSetId & rset){
    if(active_restrictions.erase(rset) > 0){
        LOG_DEBUG("deactivate_restriction_set " << graph.name(rset));
        for(auto & observer : observers){
            observer->on_restrictions_changed(active_restrictions);
        }
        //now that restrictions are different (weakend), maybe we can give some inactive workers work again:
        for(auto & i_w : workers){
            if(!i_w.second.active && i_w.second.state != graph.get_state("failed") && i_w.second.state != graph.get_state("stop")){
                //LOG_DEBUG("deactivate_restriction_set: give_work_to " << i_w.first.id());
                give_work_to(i_w.first);
            }
        }
    }
}

std::set<StateGraph::RestrictionSetId> SwarmManager::active_restriction_sets() const{
    return active_restrictions;
}

std::pair<StateGraph::StateId, bool> SwarmManager::state(const WorkerId & wid) const{
    auto it = workers.find(wid);
    std::pair<StateGraph::StateId, bool> result; // stateid is invalid
    result.second = false;
    if(it==workers.end()){
        return result;
    }
    result.first = it->second.state;
    result.second = it->second.active;
    return result;
}


DisplaySwarm::DisplaySwarm(const SwarmManager & sm_, IOManager & iom_, float dt_): sm(sm_), iom(iom_), dt(dt_) {
    if(dt > 0.0f){
        iom.schedule(bind(&DisplaySwarm::print, this), dt, true);
    }
    const StateGraph & g = sm.get_graph();
    auto all_states = g.all_states();
    maxwidth = 0;
    for(auto s : all_states){
        maxwidth = max(g.name(s).size(), maxwidth);
    }
    clock_gettime(CLOCK_MONOTONIC, &t0);
    rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    ts0 = usage.ru_stime;
    tu0 = usage.ru_utime;
    self_time = -1.0f;
    tty = isatty(1);
}

void DisplaySwarm::print(){
    timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    const auto dtf = [](const timespec & t0, const timespec & t1) -> float {return t1.tv_sec - t0.tv_sec + 1e-9f*(t1.tv_nsec - t0.tv_nsec);};
    const auto dtvf = [](const timeval & t0, const timeval & t1) -> float {return t1.tv_sec - t0.tv_sec + 1e-6f*(t1.tv_usec - t0.tv_usec);};
    //const auto tv2f = [](const timeval & t) -> float {return t.tv_sec + 1e-6f * t.tv_usec;};
    rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    const StateGraph & g = sm.get_graph();
    auto workers = sm.get_workers();
    typedef std::pair<StateGraph::StateId, bool> t_state;
    map<t_state, size_t> nworkers;
    for(auto w : workers){
        auto state = sm.state(w);
        nworkers[state]++;
    }
    auto all_states = g.all_states();
    // output
    if(self_time < 0.0f){
        // first time: clear terminal:
        if(tty) cout << "\033[2J" << flush;
        self_time = 0.0f;
    }
    if(tty) cout << "\033[0;0H" << flush;
    for(auto s : all_states){
        cout << setw(maxwidth+1) << g.name(s) << " " << nworkers[make_pair(s, true)] << " " << nworkers[make_pair(s, false)] << (tty ? "\033[K" : "") << endl;
    }
    cout << "target: " << g.name(sm.get_target_state()) << (tty ? "\033[K" : "") << endl;
    cout << "time: real " << setprecision(3) << dtf(t0, t1) << "s; user "
            << dtvf(tu0, usage.ru_utime) << "s; sys " << dtvf(ts0, usage.ru_stime) << "s" << (tty ? "\033[K" : "") << endl;
    //cout << "switches: vol " << usage.ru_nvcsw << "; invol " << usage.ru_nivcsw << endl;
    //cout << "hard page faults: " << usage.ru_majflt << endl;
    /*timespec t2;
    clock_gettime(CLOCK_MONOTONIC, &t2);
    self_time += dtf(t1, t2);
    cout << "display time: " << self_time << endl;*/
    if(dt > 0.0f){
        iom.schedule(bind(&DisplaySwarm::print, this), tty? dt : 10 * dt, true);
    }
}

