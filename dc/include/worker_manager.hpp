#ifndef DC_WORKER_MANAGER_HPP
#define DC_WORKER_MANAGER_HPP

#include <set>
#include <string>
#include <vector>
#include <stdexcept>
#include <typeinfo>
#include <functional>

#include "state_graph.hpp"
#include "channel.hpp"

namespace dc{


// Manages a single worker: handle one Worker-StateGraph combination by forwarding messages
// on an underlying Channel to previously registered callbacks.
//
// Usage: create an IOManager and a Channel which is connected to a Master (SwarmManager).
//  Using that Channel, create a WorkerManager and connect all callbacks.
// Call WorkerManager::start to start listening for Messages (via the IOManager).
//
// Upon arrival of a new Message, the correspnding callback is called, and the result of this
// callback is (wrapped in a WorkerResponse object) sent back to the Master, and new
// Messages are waited for.
//
// So the call graph for a message arrival is:
// IOManager -> Channel -> WorkerManager -> callback
//
//
// On an Channel error:
// 1. the Channel is closed (this is automatic by the Channel class)
// 2. the state is updated:
//   * in case the connection is closed and the current status is 'stop' and the last message has been written completely, the status
//     is still 'stop'
//   * in all other cases, the status is 'failed'
// 3. The error handler is called, if provided
class WorkerManager {
public:
    
    // eof: connection to master closed cleanly between messages
    // io: connection error (including closed connections in the middle of a message)
    // aborted: got an 'Abort' request from the master
    // messagetype: invalid messagetype for current state received
    enum class ErrorType { io, aborted, messagetype };
    
    typedef std::function<void (ErrorType e)> error_handler_type;
    
    // use Channel c to communicate with the Master
    explicit WorkerManager(const StateGraph & s);
    
    // connect certain (Worker) methods with a callback:
    // if the worker is in state s and a message of type T is received, callback is called;
    // the next state is implied by the StateGraph.
    //
    // The return value of the callback is wrapped into a WorkerResponse object; it can be a null pointer.
    template<typename T>
    void connect(const StateGraph::StateId & from, std::function<std::unique_ptr<Message> (T &)> callback){
        StateGraph::StateId to = graph.next_state<T>(from);
        callbacks[std::make_pair(from,to)].reset(new CallbackWrapper<T>(std::move(callback)));
    }
    
    // check that the connections made with connect are complete, i.e. all state transitions in the stategraph
    // have an associated callback.
    // throws an exception if connections are missing.
    void check_connections() const;
    
    // start by waiting for Messages in the input channel. throws a invalid_argument exception
    // if not everything is completely connected.
    void start(std::unique_ptr<Channel> c);
    
    // tell the master that we want to stop. This sill sends the rs_stop request_state as soon as
    // possible (in the next response); it's still the master deciding what to do next.
    void stop();
    
    // get current state:
    StateGraph::StateId state() const{
        return current_state;
    }
    
    // set an error handler; note that the default action to close the channel and set the current state to "failed"
    // takes place whether there is or isn't an error handler.
    void set_error_handler(const error_handler_type & h){
        error_handler = h;
    }
    
    const StateGraph & get_graph() const {
        return graph;
    }
    
    ~WorkerManager();
    
private:
    void on_in_message(std::unique_ptr<Message> message);
    void on_out_message();
    void on_error(int errorcode);
    
    void handle_error(ErrorType e);
    
    class AbstractCallback{
    public:
        virtual ~AbstractCallback();
        virtual std::unique_ptr<Message> operator()(std::unique_ptr<Message> &) = 0;
    };
    
    template<typename MT>
    class CallbackWrapper: public AbstractCallback{
    public:
        std::function<std::unique_ptr<Message> (MT &)> actual_callback;
        
        explicit CallbackWrapper(std::function<std::unique_ptr<Message> (MT &)> actual_callback_): actual_callback(std::move(actual_callback_)){}
        
        virtual std::unique_ptr<Message> operator()(std::unique_ptr<Message> & m){
            return actual_callback(static_cast<MT&>(*m));
        }
    };
    
    std::shared_ptr<Logger> logger;
    const StateGraph & graph;
    const StateGraph::StateId stop_state, failed_state;
    
    std::unique_ptr<Channel> channel;
    StateGraph::StateId current_state;
    int requested_state;
    
    boost::optional<error_handler_type> error_handler;
    
    // (from, to) -> callback
    typedef std::pair<StateGraph::StateId, StateGraph::StateId> StatePair;
    std::map<StatePair, std::unique_ptr<AbstractCallback> > callbacks;
    mutable bool check_connections_ok;
};


}

#endif
