#include <boost/test/unit_test.hpp>
#include "iomanager.hpp"
#include "utils.hpp"

#include <string.h>
#include <functional>

#include <sys/types.h>
#include <sys/wait.h>

using namespace std;
using namespace dc;
using namespace std::placeholders;

namespace {
    
    
struct reader{
    reader(IOManager & iom_, int fd_, size_t nbytes_, size_t npasses_): iom(iom_), fd(fd_), nbytes(nbytes_), npasses(npasses_), ipass(0), n_read(0){
        iom.add(fd, bind(&reader::io_callback, this, _1, _2));
        iom.set_events(fd, true, false);
        buffer.reset(new char[nbytes]);
    }
    
    bool complete() const{
        return fd == -1;
    }
    
    void io_callback(IOEvent e, int error){
        BOOST_REQUIRE(fd != -1);
        while(true){
            int res = read(fd, buffer.get() + n_read, nbytes - n_read);
            //cout << " .. res = " << res << endl;
            if(res < 0){
                int error = errno;
                // this can happen sometimes: EINTR or EAGAIN ...
                if(error == EAGAIN) break; // and try again later ...
                else if(error == EINTR) continue;
                else{
                    cerr << "NOTE: reader: io_callback read error on fd = " << fd << ": " << strerror(errno) << endl;
                    BOOST_REQUIRE(false);
                }
            }
            n_read += res;
            if(n_read == nbytes){
                // check what we got:
                for(size_t i=0; i<nbytes; ++i){
                    if(buffer.get()[i] != char(i*i)){
                        cerr << "data received for i=" << i << " wrong!" << endl;
                        BOOST_REQUIRE(false);
                    }
                }
                // next pass:
                ipass++;
                n_read = 0;
                if(ipass == npasses){
                    //cout << "reader: reached npasses, closing." << endl;
                    iom.remove(fd);
                    fd = -1;
                    break;
                }
            }
        }
        // handle ECONNRESET (note: after reading!):
        if(e==IOEvent::error){
            if(error != ECONNRESET){
                cerr << "reader: iocallback error: " << strerror(error) << endl;
                BOOST_REQUIRE(false);
            }
            if(fd != -1){
                iom.remove(fd);
            }
        }
    }
    
    IOManager & iom;
    int fd;
    const size_t nbytes, npasses;
    size_t ipass, n_read;
    unique_ptr<char[]> buffer;
};


struct writer{
    writer(IOManager & iom_, int fd_, size_t nbytes_, size_t npasses_): iom(iom_), fd(fd_), nbytes(nbytes_), npasses(npasses_), ipass(0), n_written(0){
        iom.add(fd, bind(&writer::io_callback, this, _1, _2));
        iom.set_events(fd, false, true);
        buffer.reset(new char[nbytes]);
        for(size_t i=0; i<nbytes; ++i){
            buffer.get()[i] = (char)i*i;
        }
    }
    
    bool complete() const {
        return fd == -1;
    }
    
    void io_callback(IOEvent e, int error){
        BOOST_REQUIRE(fd != -1);
        if(e==IOEvent::error){
            cerr << "iocallback error: " << strerror(error) << endl;
            BOOST_REQUIRE(false);
        }
        //cout << "calling write(" << fd << ", buffer + " << n_written << ", " << (nbytes - n_written) << ")" << endl;
        int res = write(fd, buffer.get() + n_written, nbytes - n_written);
        //cout << " .. res = " << res << endl;
        if(res < 0){
            int error = errno;
            // this can happen sometimes: EINTR or EAGAIN ...
            cerr << "writer: io_callback read error: " << strerror(errno) << endl;
            BOOST_REQUIRE(error == EAGAIN || error == EINTR);
            return;
        }
        n_written += res;
        if(n_written == nbytes){
            // next pass:
            ipass++;
            n_written = 0;
            if(ipass == npasses){
                //cout << "writer: reached npasses, closing." << endl;
                iom.remove(fd);
                fd = -1;
            }
        }
    }
    
    IOManager & iom;
    int fd;
    const size_t nbytes, npasses;
    size_t ipass, n_written;
    unique_ptr<char[]> buffer;
};

void event_handler_never_called(IOEvent e, int){
    BOOST_REQUIRE(false);
}

// t1 - t0, in seconds:
float subtract(const timespec & t1, const timespec & t0){
    return (t1.tv_sec - t0.tv_sec) * 1.0f + 1e-9f * (t1.tv_nsec - t0.tv_nsec);
}


}


BOOST_AUTO_TEST_SUITE(io)

BOOST_AUTO_TEST_CASE(add_remove0){
    int pipes[2];
    int res = pipe(pipes);
    BOOST_REQUIRE_EQUAL(res, 0);
    
    res = close(pipes[1]);
    BOOST_REQUIRE_EQUAL(res, 0);
    
    IOManager iom;
    iom.add(pipes[0], event_handler_never_called);
    iom.remove(pipes[0]);
    
    // closing was done by iom, so doing it again should fail now:
    res = close(pipes[0]);
    BOOST_REQUIRE_EQUAL(res, -1);
}

BOOST_AUTO_TEST_CASE(add_remove1){
    int pipes[2];
    int res = pipe(pipes);
    BOOST_REQUIRE_EQUAL(res, 0);
    
    close(pipes[1]);
    BOOST_REQUIRE_EQUAL(res, 0);
    
    IOManager iom;
    iom.add(pipes[0], event_handler_never_called);
    iom.remove(pipes[0], false);
    
    // closing was NOT done by iom, so it should not fail now:
    res = close(pipes[0]);
    BOOST_REQUIRE_EQUAL(res, 0);
}

BOOST_AUTO_TEST_CASE(callback){
    IOManager iom;
    
    bool called = false;
    iom.queue([&]{called = true;});
    iom.process();
    BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(infinite_callback){
    IOManager iom;

    // callback adds callback = infinite callback loop ...
    std::function<void ()> callback;
    callback = [&]{iom.queue(callback);};
    
    iom.queue(callback);
    BOOST_CHECK_THROW(iom.process(), logic_error);
}

BOOST_AUTO_TEST_CASE(double_process){
    IOManager iom;
    
    // calling process from within a handler is forbidden, make sure it throws:
    iom.queue([&]{iom.process();});
    BOOST_CHECK_THROW(iom.process(), exception);
}


BOOST_AUTO_TEST_CASE(pipe0){
    int pipes[2];
    int res = pipe(pipes);
    BOOST_REQUIRE_EQUAL(res, 0);
    
    IOManager iom;
    
    reader r(iom, pipes[0], 10, 10);
    writer w(iom, pipes[1], 10, 10);
    
    BOOST_CHECKPOINT("calling process");
    iom.process();
}

BOOST_AUTO_TEST_CASE(pipe1){
    int pipes[2];
    int res = pipe(pipes);
    BOOST_REQUIRE_EQUAL(res, 0);
    
    IOManager iom;
    
    reader r(iom, pipes[0], 1, 100000);
    writer w(iom, pipes[1], 1, 100000);
    
    BOOST_CHECKPOINT("calling process");
    iom.process();
}

BOOST_AUTO_TEST_CASE(pipe2){
    int pipes[2];
    int res = pipe(pipes);
    BOOST_REQUIRE_EQUAL(res, 0);
    
    IOManager iom;
    
    reader r(iom, pipes[0], 100000, 2);
    writer w(iom, pipes[1], 100000, 2);
    
    BOOST_CHECKPOINT("calling process");
    iom.process();
    BOOST_CHECK(r.complete());
    BOOST_CHECK(w.complete());
}

BOOST_AUTO_TEST_CASE(fork_pipe){
    int pipes[2];
    int res = pipe(pipes);
    BOOST_REQUIRE_EQUAL(res, 0);
    
    auto pid = fork();
    BOOST_REQUIRE(pid >= 0);
    
    if(pid == 0){ // child is reader:
        close(pipes[1]);
        IOManager iom;
        reader r(iom, pipes[0], 1, 100000);
        iom.process();
        BOOST_CHECK(r.complete());
        exit(0);
    }
    // parent:
    close(pipes[0]);
    IOManager iom;
    writer w(iom, pipes[1], 1, 100000);
    iom.process();
    BOOST_CHECK(w.complete());
    // wait for a lcean exit of the child:
    check_child_success(pid);
}


BOOST_AUTO_TEST_CASE(timer0){
    IOManager iom;
    timespec t0;
    BOOST_REQUIRE_EQUAL(clock_gettime(CLOCK_MONOTONIC, &t0), 0);
    timespec callback_called_at;
    callback_called_at.tv_nsec = -1;
    iom.schedule([&]{clock_gettime(CLOCK_MONOTONIC, &callback_called_at);}, 0.01f);
    iom.process();
    timespec t1;
    BOOST_REQUIRE_EQUAL(clock_gettime(CLOCK_MONOTONIC, &t1), 0);
    BOOST_REQUIRE(callback_called_at.tv_nsec >= 0);
    BOOST_CHECK_GT(subtract(t1, t0), 0.01f);
}

BOOST_AUTO_TEST_CASE(timer_weak){
    IOManager iom;
    timespec t0;
    BOOST_REQUIRE_EQUAL(clock_gettime(CLOCK_MONOTONIC, &t0), 0);
    timespec callback_called_at;
    callback_called_at.tv_nsec = -1;
    auto handle = iom.schedule([&]{clock_gettime(CLOCK_MONOTONIC, &callback_called_at);}, 0.01f, true);
    iom.process(); // should return immediately
    timespec t1;
    //test that callback has not been called
    BOOST_REQUIRE_EQUAL(clock_gettime(CLOCK_MONOTONIC, &t1), 0);
    BOOST_CHECK_LT(subtract(t1, t0), 0.005f);
    BOOST_REQUIRE_EQUAL(callback_called_at.tv_nsec, -1);
    // test that it was removed automatically:
    BOOST_CHECK_THROW(iom.cancel(handle), exception);
}


BOOST_AUTO_TEST_CASE(signals){
    IOManager iom;
    
    // get the previous action:
    struct sigaction old;
    BOOST_REQUIRE_EQUAL(sigaction(SIGUSR1, 0, &old), 0);
    BOOST_REQUIRE_EQUAL(old.sa_handler, SIG_DFL);
    
    bool handler_called = false;
    iom.setup_signal_handler(SIGUSR1, [&](const siginfo_t &){ handler_called = true;});
    kill(0, SIGUSR1);
    iom.process();
    BOOST_CHECK(handler_called);
    
    // check that resetting to default works:
    struct sigaction current;
    BOOST_REQUIRE_EQUAL(sigaction(SIGUSR1, 0, &current), 0);
    BOOST_CHECK_NE(current.sa_handler, SIG_DFL);
    
    iom.reset_signal_handler(SIGUSR1, 0);
    BOOST_REQUIRE_EQUAL(sigaction(SIGUSR1, 0, &current), 0);
    BOOST_CHECK_EQUAL(current.sa_handler, SIG_DFL);
}


BOOST_AUTO_TEST_CASE(timer2){
    IOManager iom;
    timespec t0;
    BOOST_REQUIRE_EQUAL(clock_gettime(CLOCK_MONOTONIC, &t0), 0);
    timespec callback1_time{0, -1};
    timespec callback2_time{0, -1};
    
    // in order:
    iom.schedule([&]{clock_gettime(CLOCK_MONOTONIC, &callback1_time);}, 0.01f);
    iom.schedule([&]{clock_gettime(CLOCK_MONOTONIC, &callback2_time);}, 0.02f);
    iom.process();
    BOOST_REQUIRE(callback1_time.tv_nsec >= 0);
    BOOST_REQUIRE(callback2_time.tv_nsec >= 0);
    BOOST_CHECK_GT(subtract(callback1_time, t0), 0.01f);
    BOOST_CHECK_GT(subtract(callback2_time, t0), 0.02f);
    
    // out of order:
    BOOST_REQUIRE_EQUAL(clock_gettime(CLOCK_MONOTONIC, &t0), 0);
    iom.schedule([&]{clock_gettime(CLOCK_MONOTONIC, &callback2_time);}, 0.02f);
    iom.schedule([&]{clock_gettime(CLOCK_MONOTONIC, &callback1_time);}, 0.01f);
    iom.process();
    BOOST_CHECK_GT(subtract(callback1_time, t0), 0.01f);
    BOOST_CHECK_LT(subtract(callback1_time, t0), 0.018f);
    BOOST_CHECK_GT(subtract(callback2_time, t0), 0.02f);
}

BOOST_AUTO_TEST_CASE(timer_cancel){
    IOManager iom;
    bool called = false;
    auto h = iom.schedule([&]{called = true;}, 0.01f);
    iom.cancel(h);
    iom.process();
    BOOST_CHECK(!called);
}

// let one timer cancel another
BOOST_AUTO_TEST_CASE(timer_cancel2){
    IOManager iom;
    bool called = false;
    auto handle_later = iom.schedule([&]{called = true;}, 0.1f);
    iom.schedule([&]{iom.cancel(handle_later);}, 0.01f);
    iom.process();
    BOOST_CHECK(!called);
}

// timer cancels itself: this is not allowed
BOOST_AUTO_TEST_CASE(timer_selfcancel){
    IOManager iom;
    IOManager::timer_handle h;
    h = iom.schedule([&]{iom.cancel(h);}, 0.001f);
    BOOST_CHECK_THROW(iom.process(), invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

