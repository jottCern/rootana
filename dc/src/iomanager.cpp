#include "iomanager.hpp"
#include "base/include/log.hpp"

#include <fcntl.h>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <string.h>
#include <stdexcept>
#include <deque>
#include <cassert>
#include <sys/timerfd.h>
#include <sstream>

using namespace std;
using namespace std::placeholders;
using namespace dc;

bool operator<(const timespec & t1, const timespec & t2){
    return (t1.tv_sec < t2.tv_sec) || (t1.tv_sec == t2.tv_sec && t1.tv_nsec < t2.tv_nsec);
}

namespace {
timespec gettime(){
    timespec now;
    int res = clock_gettime(CLOCK_MONOTONIC, &now);
    if(res < 0){
        throw runtime_error("clock_gettime returned error");
    }
    return now;
}

int signal_fd = -1;
deque<siginfo_t> pending_signals;
    
void iomanager_signal_handler(int signo, siginfo_t * info, void * context){
    pending_signals.push_back(*info);
    const uint64_t one = 1;
    ssize_t res;
    do{
        res = write(signal_fd, &one, sizeof(uint64_t));
    }while(res < 0 && errno == EINTR);
}

struct sm_restorer{
    sigset_t * sset;
    sm_restorer(sigset_t * sset_): sset(sset_){}
    ~sm_restorer(){
        sigprocmask(SIG_SETMASK, sset, 0);
    }
};

}


IOManager::IOManager(): logger(Logger::get("dc.IOManager")), stopped(false), epoll_active(0), processing(false), next_timer_handle(0){
    epoll_fd = epoll_create(100);
    if(epoll_fd < 0){
        LOG_ERRNO("epoll_create");
        throw runtime_error("error creating epoll");
    }
    timer_handle_active = -1;
    timed_callbacks_nweak = 0;
    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if(timerfd < 0){
        LOG_ERRNO("timerfd_create");
        throw runtime_error("timerfd_create returned error");
    }
    //LOG_DEBUG("IOManager(): timerfd = " << timerfd);
    add(timerfd, bind(&IOManager::timer_callback, this, _1, _2));
    set_events(timerfd, true, false);
    // signals:
    signal_fd = eventfd(0, EFD_NONBLOCK);
    if(signal_fd < 0){
        LOG_ERRNO("error from eventfd");
        throw runtime_error("error from eventfd");
    }
    add(signal_fd, bind(&IOManager::signal_in, this, _1, _2));
    set_events(signal_fd, true, false);
}

void IOManager::update_timeout(){
    //note: if the last callback has just been canceled, we leave
    // the timer_fd setting as it is and handle the extra (unnecessary) call to timer_callback there.
    if(!timed_callbacks_index_due.empty()){
        itimerspec itspec;
        itspec.it_interval = timespec{0,0};
        itspec.it_value = timed_callbacks_index_due.begin()->first;
        int res = timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &itspec, NULL);
        if(res < 0){
            LOG_ERRNO("timerfd_settime");
            throw runtime_error("timerfd_settime");
        }
    }
}

void IOManager::timer_callback(IOEvent evt, int error){
    if(evt==IOEvent::error){
        LOG_THROW("error from timerfd: " << strerror(error));
        return;
    }
    uint64_t n;
    int res;
    do{
        res = read(timerfd, &n, 8);
    }while(res < 0 && errno == EINTR);
    if(res < 0){
        if(errno == EAGAIN){
            LOG_ERRNO_LEVEL(loglevel::warning, "read timer_fd");
            return;
        }
        else{
            LOG_ERRNO("read timer_fd");
            throw runtime_error("error reading timer_fd");
        }
    }
    assert(res == 8);
    
    // Now call all expired timers.
    // Note that using an infinite loop here might starve IOManager::process as the callbacks called here
    // might add other timed callbacks, etc. ...Therefore limit the number of callbacks processed here
    // to the initial number of callbacks, which should be Ok for all "well-behaved" applications.
    timespec now = gettime();
    
    const size_t npasses = timed_callbacks_index_due.size(); // note: can be 0, if the last callback was canceled.
    for(size_t i=0; i<npasses; ++i){
        if(timed_callbacks_index_due.empty()) break; // can happen if one callback cancels all others
        auto due_it = timed_callbacks_index_due.begin();
        if(due_it->first < now){
            // process and remove:
            LOG_DEBUG("> timeout handler handle=" << due_it->second->handle);
            timer_handle_active = due_it->second->handle;
            due_it->second->callback();
            timer_handle_active = -1;
            LOG_DEBUG("< timeout handler handle=" << due_it->second->handle);
            timed_callbacks_index_handle.erase(due_it->second->handle);
            if(due_it->second->weak) --timed_callbacks_nweak;
            timed_callbacks.erase(due_it->second);
            timed_callbacks_index_due.erase(due_it);
        }
        else{
            // check again the time, as the callbacks could have taken some time:
            now = gettime();
            // if still not due, break:
            if(!(due_it->first < now)) break;
        }
    }
    update_timeout();
}

namespace {
void add_seconds(timespec & t, float seconds){
    time_t timeout_s = time_t(seconds);
    long timeout_ns = long((seconds - timeout_s) * 1e9f + 0.5f);
    t.tv_sec += timeout_s;
    t.tv_nsec += timeout_ns;
    if(t.tv_nsec >= 1000000000l){
        t.tv_nsec -= 1000000000l;
        t.tv_sec += 1;
    }
}
}

IOManager::timer_handle IOManager::schedule(void_function_type callback, float timeout, bool weak){
    if(timeout < 0.0f){
        LOG_THROW("schedule: timeout parameter was negative");
    }
    timespec due = gettime();
    add_seconds(due, timeout);
    timed_callbacks.emplace_back(timed_callback{due, next_timer_handle++, move(callback), weak});
    auto it = timed_callbacks.end();
    --it;
    timed_callbacks_index_due.insert(make_pair(it->due, it));
    timed_callbacks_index_handle[it->handle] = it;
    if(weak) ++timed_callbacks_nweak;
    update_timeout();
    return it->handle;
}

void IOManager::cancel(timer_handle h){
    if(timer_handle_active == h){
        throw invalid_argument("timed callback tried to cancel itself");
    }
    auto handle_it = timed_callbacks_index_handle.find(h);
    if(handle_it == timed_callbacks_index_handle.end()){
        throw invalid_argument("no such timer handle");
    }
    bool do_update = timed_callbacks_index_handle.begin()->second == handle_it->second;
    if(handle_it->second->weak) --timed_callbacks_nweak;
    timed_callbacks_index_due.erase(handle_it->second->due);
    timed_callbacks.erase(handle_it->second);
    timed_callbacks_index_handle.erase(handle_it);
    if(do_update){
        update_timeout();
    }
}

void IOManager::add(int fd, event_handler_type event_handler){    
    auto it = fd_to_epoll_data.find(fd);
    if(it != fd_to_epoll_data.end()){
        LOG_THROW("add: fd " << fd << " already exists");
    }
    std::unique_ptr<epoll_data> d(new epoll_data);
    d->handler = move(event_handler);
    d->fd = fd;
    d->events = EPOLLERR;
    int res = fcntl(fd, F_SETFL, O_NONBLOCK);
    if(res < 0){
        LOG_ERRNO("fcntl to set fd=" << fd << " to non-blocking");
        throw runtime_error("error in fcntl");
    }
    epoll_event evt;
    evt.data.ptr = d.get();
    evt.events = EPOLLERR;
    ++epoll_active;
    res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &evt);
    if(res < 0){
        LOG_ERRNO("error adding fd=" << fd << " to epoll");
        throw runtime_error("error in epoll_ctl ADD");
    }
    fd_to_epoll_data[fd] = move(d);
}

void IOManager::set_event_handler(int fd, event_handler_type event_handler){
    auto it = fd_to_epoll_data.find(fd);
    if(it == fd_to_epoll_data.end()){
        LOG_THROW("set_event_handler: fd " << fd << " does not exist");
    }
    it->second->handler = move(event_handler);
}

namespace{
    struct resetter{
        bool & flag;
        resetter(bool & flag_): flag(flag_){
            flag = true;
        }
        ~resetter(){
            flag = false;
        }
    };
    
string strevents(int emask){
    string result;
    if((emask & EPOLLIN)) result += " | IN";
    if((emask & EPOLLOUT)) result += " | OUT";
    if((emask & EPOLLERR)) result += " | ERR";
    if((emask & EPOLLHUP)) result += " | HUP";
    if(result.empty()) result = " | (None)";
    return result.substr(3);
}
    
}

void IOManager::process(int npass){
    if(processing){
        LOG_THROW("called IOManager::process from an event handler; this is not allowed");
    }
    resetter r(processing); // set the processing flag to true; set it to false on leaving this method.
    stopped = false;
    size_t ipass = 0;
    while(true){
        if(stopped) break;
        ipass++;
        if(npass > 0 && static_cast<size_t>(npass) == ipass)break;
        call_all_callbacks();
        if(stopped) break;
        // test whether there is anything to do: "ground state" of nothing to do
        // is that only the timerfd and the signal_fd are active in the epoll set (i.e. epoll_active == 2) and that the only
        // scheduled timed callbacks are weak and there is no pending signal
        if(epoll_active==2 && timed_callbacks.size() == timed_callbacks_nweak && callbacks.empty() && pending_signals.empty()){
            LOG_DEBUG("process: no registered i/o, returning");
            //cancel all weak callbacks (note: always delete back() to avoid re-setting timerfd too much)
            while(!timed_callbacks.empty()) cancel(timed_callbacks.back().handle);
            break;
        }
        epoll_event events[epoll_active];
        LOG(logger, loglevel::debug, "calling epoll_wait, epoll_active=" << epoll_active);
        int timeout = npass == 0 ? 0 : -1;
        int res;
        do{
            res = epoll_wait(epoll_fd, events, epoll_active, timeout);
        } while(res < 0 && errno == EINTR);
        LOG(logger, loglevel::debug, "epoll_wait returned with res = " << res);
        if(res < 0){
            LOG_ERRNO("epoll_wait");
            throw runtime_error("error from epoll_wait");
        }
        for(int i=0; i<res; ++i){
            LOG_DEBUG("  epoll_wait_result[" << i << "]: fd=" << reinterpret_cast<epoll_data*>(events[i].data.ptr)->fd << ": events=" << events[i].events << " = " << strevents(events[i].events));
        }
        for(int i=0; i<res; ++i){
            if(stopped) break;
            // NOTE: if an event handler of one fd closes another fd via IOManager::close for which we already have an event
            // pending here, we have to be careful not to destroy the associated epoll_data for the closed fd too soon, as
            // otherwise the next line could give as an invalid pointer.
            // Therefore, IOManager::close closes the fd and sets the fd in epoll_data to -1 (for which we check here)
            // but does not delete the epoll_data immediately; instead, the fd is
            // added to fd_to_epoll_cleanup which is cleaned up later (see below)
            epoll_data * d = reinterpret_cast<epoll_data*>(events[i].data.ptr);
            if(d->fd == -1) continue;
            int fd = d->fd;
            // otherwise, report read and write as two separate calls to the handler:
            if((events[i].events & EPOLLIN)){
                LOG_DEBUG("> handler in fd=" << fd);
                d->handler(IOEvent::in, 0);
                LOG_DEBUG("< handler in fd=" << fd);
            }
            call_all_callbacks();
            if(d->fd!= -1 && (events[i].events & EPOLLOUT)){
                LOG_DEBUG("> handler out fd=" << fd);
                d->handler(IOEvent::out, 0);
                LOG_DEBUG("< handler out fd=" << fd);
            }
            call_all_callbacks();
            if(d->fd != -1 && ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP))){
                int error = 0;
                if((events[i].events & EPOLLHUP)){
                    error = ECONNRESET;
                }
                else if((events[i].events & EPOLLERR)){
                    LOG_DEBUG("got error for fd=" << d->fd << "; reading error from getsockopt");
                    socklen_t s = sizeof(int);
                    getsockopt(d->fd, SOL_SOCKET, SO_ERROR, &error, &s); // can fail if fd is not a socket, leaving error set to 0.
                    LOG_DEBUG("getsockopt for fd=" << d->fd << " returned " << error << " (" << strerror(error) << ")");
                    if(error == 0){
                        LOG_WARNING("got unknown error for fd= " << d->fd << "; reporting as ECONNABORTED");
                        error = ECONNABORTED;
                    }
                }
                // save handler, as calling remove makes the handler in d invalid:
                LOG_DEBUG("> handler error fd = " << fd);
                d->handler(IOEvent::error, error);
                LOG_DEBUG("< handler error fd = " << fd);
                if(d->fd != -1){
                    LOG_ERROR("error handler did not close fd " << fd << " for epoll_data at " << (void*)d << ": fd = " << d->fd);
                }
            }
        }
        for(auto & ed : epoll_data_cleanup){
            LOG_DEBUG("cleaning up epoll data at " << (void*)ed.get());
        }
        epoll_data_cleanup.clear();
    }
}


void IOManager::call_all_callbacks(){
    size_t n = 0;
    size_t nmax = callbacks.size() * 10;
    while(!callbacks.empty() && !stopped){
        LOG_DEBUG("> user callback");
        callbacks.front()();
        LOG_DEBUG("< user callback");
        callbacks.pop_front();
        ++n;
        if(n > nmax){
            throw invalid_argument("too many nested callbacks");
        }
    }
}


void IOManager::set_events(int fd, bool in, bool out){
    LOG_DEBUG("set_events for fd=" << fd << "; in=" << in << "; out=" << out);
    int emask = EPOLLERR;
    if(in) emask |= EPOLLIN;
    if(out) emask |= EPOLLOUT;
    auto it = fd_to_epoll_data.find(fd);
    if(it==fd_to_epoll_data.end()){
        LOG_THROW("set_events_active for fd=" << fd << ": unknown fd");
    }
    if(it->second->events == emask) return;
    epoll_event event;
    it->second->events = event.events = emask;
    event.data.ptr = &(*it->second);
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
    if(res < 0){
        LOG_ERRNO("set_events_active: epoll_ctl");
        throw runtime_error("set_events_active: epoll_ctl failed");
    }
}

void IOManager::signal_in(IOEvent e, int err){
    // block all signals; otherwise, pending_signals is accessed both from here and from the signal handler
    // which is not good ...
    sigset_t oldset, newset;
    sigfillset(&newset);
    int res = sigprocmask(SIG_SETMASK, &newset, &oldset);
    sm_restorer smr(&oldset);
    
    assert(res==0); // should never fail
    uint64_t n_signals = 0;
    res = read(signal_fd, &n_signals, sizeof(uint64_t));
    if(res < 0){
        LOG_ERRNO("read from signal_fd returned error");
        return;
    }
    assert(n_signals == pending_signals.size());
    while(!pending_signals.empty()){
        siginfo_t si = pending_signals.front();
        pending_signals.pop_front();
        auto it = signal_handlers.find(si.si_signo);
        if(it==signal_handlers.end()){
            LOG_ERROR("received signal " << si.si_signo << " with unknown handler");
            return;
        }
        it->second(si);
    }
}


void IOManager::setup_signal_handler(int signo, signal_handler_type signal_handler){
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_flags = SA_SIGINFO;
    sigfillset(&sa.sa_mask);
    sa.sa_sigaction = &iomanager_signal_handler;
    int res = sigaction(signo, &sa, 0);
    if(res < 0){
        LOG_ERRNO("setup_signal_handler sigaction");
        throw runtime_error("setup_signal_handler sigaction");
    }
    signal_handlers[signo] = move(signal_handler);
}

void IOManager::reset_signal_handler(int signo, struct sigaction * new_sa){
    auto it = signal_handlers.find(signo);
    if(it == signal_handlers.end()){
        LOG_THROW("reset_signal_handler: no signal handler registered for signal " << signo << " (" << strsignal(signo) << ")");
    }
    struct sigaction * psa;
    struct sigaction sa;
    if(new_sa){
        psa = new_sa;
    }
    else{
        psa = &sa;
        memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_handler = SIG_DFL;
        sigfillset(&sa.sa_mask);
    }
    int res = sigaction(signo, psa, 0);
    if(res < 0){
        LOG_ERRNO("reset_signal_handler sigaction");
        throw runtime_error("reset_signal_handler sigaction");
    }
    signal_handlers.erase(it);
}

void IOManager::remove(int fd, bool do_close){
    LOG_DEBUG("remove(fd=" << fd << ", do_close = " << do_close << ")");
    
    // to support closing twice, look if we have it at all:
    auto it = fd_to_epoll_data.find(fd);
    if(it==fd_to_epoll_data.end() || it->second->fd == -1){
        LOG_DEBUG("close fd=" << fd << ": invalid fd or already closed");
        return; // ignore if we don't find the fd; this makes remove idempotent
    }

    // mark as closed in epoll_data by setting fd to -1 and schedule for cleanup by
    // moving it to epoll_data_cleanup:
    it->second->fd = -1;
    epoll_data_cleanup.emplace_back(move(it->second));
    fd_to_epoll_data.erase(it);

    // remove from epoll:
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    --epoll_active;
    if(res < 0){
        LOG_ERRNO_LEVEL(loglevel::warning, " removing fd=" << fd << " from epoll returned error");
    }
    
    // do the actual close itself:
    if(do_close){
        res = ::close(fd);
        if(res < 0){
            LOG_ERRNO_LEVEL(loglevel::warning, " in close(fd=" << fd << ") returned error");
        }
    }
}

void IOManager::stop(){
    LOG(logger, loglevel::debug, "stop called");
    stopped = true;
}

IOManager::~IOManager(){
    while(!signal_handlers.empty()){
        reset_signal_handler(signal_handlers.begin()->first, 0);
    }
    remove(timerfd);
    remove(signal_fd);
    if(epoll_active){
        LOG_ERROR("In ~IOManager: there are still active fds!");
    }
    int res = close(epoll_fd);
    if(res != 0){
        LOG_ERRNO("epoll_close in ~IOManager");
    }
}

