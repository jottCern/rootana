#include "swarm_manager.hpp"
#include "worker_manager.hpp"
#include <boost/test/unit_test.hpp>
#include "utils.hpp"

#include <sys/socket.h>
#include <iomanip>

using namespace dc;
using namespace std;
using namespace std::placeholders;

namespace {

// a simple state graph for testing: 
// start ->[mwork] work  ->[mstop]  stop
// start ->[mstop] stop
// work ->[mwork] work
StateGraph test_sg0;


struct mwork: public Message {
public:
    int i;
    
    explicit mwork(int i_ = 0): i(i_){}
    
    void read_data(Buffer & b){
        b >> i;
    }
    
    void write_data(Buffer & b) const {
        b << i;
    }
};

class mstop: public Message {    
public:
    void read_data(Buffer & b){}
    void write_data(Buffer & b) const{}
};

REGISTER_MESSAGE(mwork, "mwork");
REGISTER_MESSAGE(mstop, "mstop");

int create_test_sg0(){
    auto s_work = test_sg0.add_state("work");
    auto s_start = test_sg0.get_state("start");
    auto s_stop = test_sg0.get_state("stop");
    
    test_sg0.add_state_transition<mwork>(s_start, s_work);
    test_sg0.add_state_transition<mwork>(s_work, s_work);
    
    test_sg0.add_state_transition<mstop>(s_start, s_stop);
    test_sg0.add_state_transition<mstop>(s_work, s_stop);
    
    auto r_nowork = test_sg0.add_restriction_set("nowork");
    test_sg0.add_restriction(r_nowork, s_work, s_work);
    test_sg0.add_restriction(r_nowork, s_start, s_work);
    
    return 0;
}

int dummy = create_test_sg0();

// note: worker does not know anything about workermanager, it just exposes some methods
// suitable for WorkerManager::connect ...
class Worker {
public:
    Worker(int abort_after_ = -1, bool verb = false): abort_after(abort_after_), n(0), verbose(verb), delay(0.0f), do_fail_in_stop(false){}
    
    // IMPORTANT: delay only makes sense for forking or network tests. If in the same
    // process as the master, will delay everything ...
    void set_delay(float time_seconds){
        delay = time_seconds;
    }
    
    void set_fail_in_stop(bool do_fail = true){
        do_fail_in_stop = do_fail;
    }
    
    std::unique_ptr<Message> do_work(mwork & m){
        if(verbose)cout << "worker: got work: " << m.i << endl;
        n++;
        if(n==abort_after){
            if(verbose)cout << "Worker: emulating failure now." << endl;
            throw runtime_error("worker failure");
        }
        if(delay > 0.0f){
            timespec sleep_for;
            sleep_for.tv_sec = time_t(delay);
            sleep_for.tv_nsec = long((delay - sleep_for.tv_sec) * 1000000000ul);
            timespec rem;
            int res;
            do{
                res = nanosleep(&sleep_for, &rem);
            }
            while(res < 0 && errno == EINTR);
            BOOST_CHECK_EQUAL(res, 0);
        }
        return unique_ptr<Message>(new mwork(m.i * m.i));
    }
    
    std::unique_ptr<Message> do_stop(mstop & s){
        if(verbose)cout << "worker: got stop" << endl;
        if(do_fail_in_stop){
            throw runtime_error("worker failure in stop");
        }
        return unique_ptr<Message>();
    }
    
private:
    int abort_after, n;
    bool verbose;
    float delay;
    bool do_fail_in_stop;
};


// note: for the master side, things are coupled more tightly --> use SwarmManager as member
class Master {
public:
    Master(const Master&) = delete;
    explicit Master(std::vector<int> work_, bool v = false): verbose(v), work(move(work_)), sm(test_sg0, bind(&Master::worker_failed, this, _1, _2)), nfailed(0){
        auto s_work = test_sg0.get_state("work");
        auto s_start = test_sg0.get_state("start");
        
        sm.connect<mwork>(s_work, bind(&Master::generate_work, this, _1));
        sm.connect<mwork>(s_start, bind(&Master::generate_work, this, _1));
        
        sm.connect<mstop>(s_start, bind(&Master::generate_stop, this, _1));
        sm.connect<mstop>(s_work, bind(&Master::generate_stop, this, _1));
        
        sm.set_result_callback(s_work, bind(&Master::work_complete, this, _1, _2));
        
        sm.set_target_state(s_work);
        sm.check_connections();
    }
    
    std::unique_ptr<mwork> generate_work(const WorkerId & worker){
        if(verbose)cout << "master: giving worker " << worker.id() << " work " << work.back() << endl;
        BOOST_REQUIRE(!work.empty());
        std::unique_ptr<mwork> result(new mwork());
        result->i = work.back();
        work.resize(work.size() - 1);
        if(work.empty()){
            if(verbose)cout << "master: all work done, activating nowork restriction, and setting stop target" << endl;
            sm.activate_restriction_set(test_sg0.get_restriction_set("nowork"));
            sm.set_target_state(test_sg0.get_state("stop"));
        }
        return result;
    }
    
    std::unique_ptr<mstop> generate_stop(const WorkerId & id){
        return std::unique_ptr<mstop>(new mstop());
    }
    
    void work_complete(const WorkerId & worker, std::unique_ptr<Message> result){
        if(verbose){
            cout << "master: work complete for worker " << worker.id() << ": "
                 << dynamic_cast<mwork&>(*result).i << endl;
        }
    }
    
    void worker_failed(const WorkerId & worker, const StateGraph::StateId & last_state){
        if(verbose)cout << "master: worker failed: " << worker.id() << " in state " << test_sg0.name(last_state) << endl;
        ++nfailed;
    }
    
    void add_worker(unique_ptr<Channel> c){
        auto worker = sm.add_worker(move(c));
        if(verbose)cout << "master: added worker " << worker.id() << endl;
    }
    
    bool verbose;
    std::vector<int> work;
    SwarmManager sm;
    int nfailed;
};

void setup_worker_sg0(WorkerManager & wm, Worker & w, std::unique_ptr<Channel> c) {
    //cout << "setup worker" << endl;
    auto s_work = test_sg0.get_state("work");
    auto s_start = test_sg0.get_state("start");
    //auto s_stop = test_sg0.get_state("stop");
    
    wm.connect<mwork>(s_work, bind(&Worker::do_work, &w, _1));
    wm.connect<mwork>(s_start, bind(&Worker::do_work, &w, _1));
    
    wm.connect<mstop>(s_start, bind(&Worker::do_stop, &w, _1));
    wm.connect<mstop>(s_work, bind(&Worker::do_stop, &w, _1));
    wm.start(move(c));
    //cout << "setup worker end." << endl;
}

}

BOOST_AUTO_TEST_SUITE(swarm)


BOOST_AUTO_TEST_CASE(swarm1){
    // try one worker:
    int sockets[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    BOOST_REQUIRE_EQUAL(res, 0);
    
    IOManager iom;
    //cout << "master fd: " << sockets[0] << endl;
    //cout << "worker fd: " << sockets[1] << endl;
    std::unique_ptr<Channel> master_channel(new Channel(sockets[0], iom));
    std::unique_ptr<Channel> worker_channel(new Channel(sockets[1], iom));
    
    Master master({1,2,3});
    master.add_worker(move(master_channel));
    
    WorkerManager wm(test_sg0);
    Worker w;
    setup_worker_sg0(wm, w, move(worker_channel));
    
    iom.process();
    BOOST_CHECK(master.work.empty());
    BOOST_CHECK_EQUAL(master.nfailed, 0);
}


BOOST_AUTO_TEST_CASE(swarm2){
    // try two workers:
    int sockets[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    BOOST_REQUIRE_EQUAL(res, 0);
    int sockets2[2];
    res = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets2);
    BOOST_REQUIRE_EQUAL(res, 0);
    
    IOManager iom;
    
    Master master({1,2,3});
    master.add_worker(std::unique_ptr<Channel>(new Channel(sockets[1], iom)));
    master.add_worker(std::unique_ptr<Channel>(new Channel(sockets2[1], iom)));
    
    WorkerManager wm1(test_sg0), wm2(test_sg0);
    Worker w1, w2;
    setup_worker_sg0(wm1, w1, std::unique_ptr<Channel>(new Channel(sockets[0], iom)));
    setup_worker_sg0(wm2, w2, std::unique_ptr<Channel>(new Channel(sockets2[0], iom)));
    
    iom.process();
    BOOST_CHECK(master.work.empty());
    BOOST_CHECK_EQUAL(master.nfailed, 0);
}

// check that large number of workers do work:
BOOST_AUTO_TEST_CASE(swarm_fork){
    const size_t nworkers = 1000;
    const size_t Nwork = 10000;
    const bool display = true;
    float delay = display ? 0.2f : 0.0f; // 1000 workers in 100 processes should take around 2 seconds (+overhead)
    
    std::vector<int> master_fds;
    std::vector<pid_t> children;
    for(size_t i=0; i<nworkers; ++i){
        int sockets[2];
        int res = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
        BOOST_REQUIRE_EQUAL(res, 0);
        auto pid = fork();
        if(pid == -1){
            cerr << "fork failed: " << strerror(errno) << endl;
            BOOST_REQUIRE(false);
        }
        if(pid == 0){
            // worker:
            close(sockets[1]);
            IOManager iom;
            WorkerManager wm(test_sg0);
            Worker w;
            w.set_delay(delay);
            setup_worker_sg0(wm, w, std::unique_ptr<Channel>(new Channel(sockets[0], iom)));
            iom.process();
            exit(0);
        }
        else{
            // master: only keep fds, set up rest later.
            //cout << "created child " << i << ": " << pid << endl;
            close(sockets[0]);
            master_fds.push_back(sockets[1]);
            children.push_back(pid);
        }
    }
    
    // master:
    IOManager iom;
    vector<int> work;
    work.reserve(Nwork);
    for(size_t i=0; i<Nwork; ++i){
        work.push_back(i);
    }
    
    Master master(move(work));
    unique_ptr<DisplaySwarm> dis;
    if(display){
        dis.reset(new DisplaySwarm(master.sm, iom));
    }
    
    for(size_t i=0; i<nworkers; ++i){
        master.add_worker(std::unique_ptr<Channel>(new Channel(master_fds[i], iom)));
    }
    
    iom.process();
    if(dis){
        // print at the very end to display final numbers
        dis->print();
    }
    BOOST_CHECK(master.work.empty());
    BOOST_CHECK_EQUAL(master.nfailed, 0);
    
    // wait for all childs:
    BOOST_REQUIRE_EQUAL(nworkers, children.size());
    for(size_t i=0; i<nworkers; ++i){
        //cout << "waiting for worker " << i << ": " << children[i] << endl;
        check_child_success(children[i]);
    }
    // TODO: sometimes, output stops before reaching the end ...
    cout << "end test swarm_fork" << endl;
}

// check that failure is reported
BOOST_AUTO_TEST_CASE(failure){
    int sockets[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    BOOST_REQUIRE_EQUAL(res, 0);
    
    IOManager iom;
    Master master({1,2,3,4,5});
    master.add_worker(std::unique_ptr<Channel>(new Channel(sockets[0], iom)));
    
    WorkerManager wm(test_sg0);
    Worker w(2);
    setup_worker_sg0(wm, w, std::unique_ptr<Channel>(new Channel(sockets[1], iom)));
    
    iom.process();
    BOOST_CHECK_EQUAL(master.nfailed, 1);
}

// make sure a failure in stopping a worker is also recognized as failure
BOOST_AUTO_TEST_CASE(stopfail){
    int sockets[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    BOOST_REQUIRE_EQUAL(res, 0);
    
    IOManager iom;
    
    Master master({1,2,3,4,5});
    master.add_worker(std::unique_ptr<Channel>(new Channel(sockets[0], iom)));
    
    WorkerManager wm(test_sg0);
    Worker w;
    w.set_fail_in_stop();
    setup_worker_sg0(wm, w, std::unique_ptr<Channel>(new Channel(sockets[1], iom)));
    
    iom.process();
    BOOST_CHECK_EQUAL(master.nfailed, 1);
}

BOOST_AUTO_TEST_SUITE_END()

