#include "worker_manager.hpp"
#include "fwmessages.hpp"
#include <stdexcept>

using namespace std;
using namespace std::placeholders;
using namespace dc;

WorkerManager::AbstractCallback::~AbstractCallback(){}

WorkerManager::WorkerManager(const StateGraph & sg): logger(Logger::get("dc.WorkerManager")), graph(sg),
  stop_state(graph.get_state("stop")), failed_state(graph.get_state("failed")), current_state(graph.get_state("start")),
  requested_state(WorkerResponse::rs_work), check_connections_ok(false){
      
}

WorkerManager::~WorkerManager(){
    LOG_DEBUG("~WorkerManager entered");
    if(!channel->closed()){
        LOG_ERROR("~WorkerManager with unclosed channel");
    }
}

void WorkerManager::check_connections() const{
    if(check_connections_ok) return;
    set<StateGraph::StateId> all_ids = graph.all_states();
    for(const StateGraph::StateId & state : all_ids){
        auto next = graph.next_states(state);
        for(auto & n : next){
            if(callbacks.find(make_pair(state, n)) == callbacks.end()){
                LOG_THROW("check_connections: missing connection for state transition " + graph.name(state) + " -> " + graph.name(n))
            }
        }
    }
    check_connections_ok = true;
}

void WorkerManager::stop(){
    requested_state = WorkerResponse::rs_stop;
}

void WorkerManager::start(std::unique_ptr<Channel> c){
    LOG_INFO("start: setting up communication");
    check_connections();
    channel = move(c);
    channel->set_error_handler(bind(&WorkerManager::on_error, this, _1));
    channel->set_read_handler(bind(&WorkerManager::on_in_message, this, _1));
}
  
void WorkerManager::on_in_message(std::unique_ptr<Message> message){
    if(!message){
        LOG_ERROR("received null message; handling this as messagetype error");
        handle_error(ErrorType::messagetype);
        return;
    }
    WorkerResponse r;
    r.requested_state = requested_state;
    StateGraph::StateId next_state;
    try{
        next_state = graph.next_state(current_state, typeid(*message));
    }
    catch(invalid_argument & ){
        LOG_THROW("unknown state transition from state " << graph.name(current_state) << " on message type " << typeid(*message).name());
    }
    LOG_DEBUG("on_in_message: calling callback for state transition " << graph.name(current_state) << " -> " << graph.name(next_state));
    try{
        r.response_details = (*(callbacks[StatePair(current_state,next_state)]))(message);
    }
    catch(exception & ex){
        LOG_ERROR("callback for state transition " << graph.name(current_state) 
                  << " -> " << graph.name(next_state) << " has thrown exception: '" << ex.what() << "'; aborting.");
        handle_error(ErrorType::aborted);
        return;
    }
    catch(...){
        LOG_ERROR("callback for state transition " << graph.name(current_state) 
                  << " -> " << graph.name(next_state) << " has thrown exception; aborting");
        handle_error(ErrorType::aborted);
        return;
    }
    //LOG_DEBUG("on_in_message: callback for state transition " << graph.name(current_state) << " -> " << graph.name(next_state) << " returning, sending answer");
    current_state = next_state;
    channel->write(r, bind(&WorkerManager::on_out_message, this));
}
    
void WorkerManager::on_out_message(){
    if(current_state!=stop_state && current_state != failed_state){
        LOG_DEBUG("on_out_message: setting read handler");
        channel->set_read_handler(bind(&WorkerManager::on_in_message, this, _1));
    }
    else{
        LOG_DEBUG("on_out_message: we are in stop state; closing connection.");
        channel->close();
    }
}
    
void WorkerManager::on_error(int errorcode){
    LOG_INFO("on_error called with code " << errorcode << " (" << strerror(errorcode) << ")");
    if(current_state==failed_state){}
    // special case: connection was closed while we are in the stop state:
    else if(current_state==stop_state && errorcode==ECONNRESET){
        LOG_DEBUG("on_error: is ECONNRESET in stop state, ignoring");
    }
    else{
        handle_error(ErrorType::io);
    }
}

void WorkerManager::handle_error(ErrorType e){
    LOG_DEBUG("entering handle_error");
    channel->close();
    current_state = failed_state;
    if(error_handler){
        LOG_DEBUG("handle_error: calling user error handler");
        (*error_handler)(e);
    }
    else{
        LOG_DEBUG("handle_error: no user error handler provided");
    }
}
