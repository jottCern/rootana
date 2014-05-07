#ifndef DC_IOMANAGER_HPP
#define DC_IOMANAGER_HPP

#include <functional>
#include <ostream>
#include <set>
#include <memory>
#include <vector>
#include <list>
#include <signal.h>

#include "base/include/log.hpp"
#include "fwd.hpp"

using std::function;

namespace dc{
    

/** \brief Manage IO Multiplexing to many file descriptors
 * 
 * C++ interface to epoll.
 * 
 * Error handling policy:
 *  - Errors indicating interface misuse such as errno = EBADFD are reported via exceptions.
 *  - io errors on operations on the underlying fd are reported by calling the event_handler with the appropriate error code.
 *    Note that a closed connection (hangup) is reported as ECONNRESET error. In case of an error, the handler should ALWAYS remove
 *    the fd causing the error from the IOManager by calling IOManager::remove in the event handler.
 */
class IOManager{
public:
    
    typedef function<void (IOEvent, int error)> event_handler_type;
    typedef function<void ()> void_function_type;
    typedef function<void (const siginfo_t &)> signal_handler_type;
    
    typedef int timer_handle;
    
    // do not copy:
    IOManager(const IOManager &) = delete;

    /** \brief Process pending IO events
     * 
     * Call the callbacks for all registered file descriptors with possible I/O.
     * 
     * \c npasses controls how often the processing is done and whether it blocks:
     *   - \c npasses < 0: process until stop is called or no active fd exists
     * 0 means to process once but without waiting (i.e. only currently pending I/O, do not block)
     */
    void process(int npasses = -1);

    // make 'process' return as soon as possible. Event processing can be resumed by calling process() again.
    void stop();
    
    // add an fd.
    // NOTE: if using bind to construct the event handler, make sure to use std::ref to avoid copies when applicable
    void add(int fd, event_handler_type event_handler);
    
    void set_event_handler(int fd, event_handler_type event_handler);
    
    // set whether the callback of input and output events should be active. Note
    // that the error event is always active.
    void set_events(int fd, bool in, bool out);
    
    // remove the fd from the set of watched file descriptors. removing a non-watched fd is ignored.
    void remove(int fd, bool close_fd = true);
    
    // setup signal handler
    void setup_signal_handler(int signo, signal_handler_type signal_handler);
    // stop listening for a signal. This removes the thread-wide (IOManager's internal) signal handler for that signal.
    // To avoid race conditions, this has to be done by replacing it with a new signal handler  (instead of simply "removing" it).
    // new_sa can be the null pointer in which case the system default (SIG_DFL) is set.
    void reset_signal_handler(int signo, struct sigaction * new_sa);
    
    // queue the given callback for execution; it will be called from IOManager::process as soon as possible.
    // Example use: delete an object which keeps the event handler from within the event handler.
    // This cannot be done directly from the event handler itself, but the event handler can 'queue' a method
    // performing the deletion.
    void queue(void_function_type callback){
        callbacks.emplace_back(std::move(callback));
    }
    
    // schedule a callback with a timeout. timeout is in seconds and is the minimum time waited until the callback is called.
    // If a timeout is weak, it will not count when determining whether there is pending I/O in process. This
    // is useful for monitoring that should stop automatically when the rest of the system stops. When
    // IOManager::process returns because there is no more I/O, it cancels all weak callbacks.
    timer_handle schedule(void_function_type callback, float timeout, bool weak = false);
    void cancel(timer_handle h);
    
    IOManager();
    ~IOManager();
    
private:
    
    void call_all_callbacks();
    
    void signal_in(IOEvent, int);
    
    std::shared_ptr<Logger> logger;
  
    struct epoll_data{
        int events; // events we are listening to
        int fd;
        event_handler_type handler;
    };
    
    bool stopped;
    size_t epoll_active;
    bool processing;
    
    int epoll_fd;
    std::map<int, std::unique_ptr<epoll_data> > fd_to_epoll_data;
    std::vector<std::unique_ptr<epoll_data> > epoll_data_cleanup;
    
    std::list<void_function_type> callbacks;
    
    std::map<int, signal_handler_type> signal_handlers; // map signal numbers to callbacks.
    
    // ** timer stuff:
    // save timed_callback_info indexed by the timeout and by the timer_handle.
    struct timed_callback {
        timespec due; // absolute time.
        timer_handle handle;
        void_function_type callback;
        bool weak;
    };
    
    // this is where the callback info is actually saved:
    std::list<timed_callback> timed_callbacks;
    typedef std::list<timed_callback>::iterator itimed_callback;
    
    // some indices into timed_callbacks:
    std::multimap<timespec, itimed_callback> timed_callbacks_index_due;
    std::map<timer_handle, itimed_callback> timed_callbacks_index_handle;
    
    size_t timed_callbacks_nweak;
    
    int timerfd;
    timer_handle next_timer_handle;
    timer_handle timer_handle_active;
    
    void update_timeout();
    void timer_callback(IOEvent, int);
};

}

#endif
