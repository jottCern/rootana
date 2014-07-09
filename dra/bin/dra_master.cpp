#include "master.hpp"
#include "local.hpp"
#include "stategraph.hpp"
#include "dc/include/netutils.hpp"

using namespace dra;
using namespace dc;
using namespace std;

class Stopper: public MasterObserver {
public:
    explicit Stopper(TcpAcceptor & acc_): logger(Logger::get("dra.Stopper")), acc(acc_){}
    
    virtual void on_target_changed(const StateGraph::StateId & new_target) override {
        if(new_target == get_stategraph().get_state("stop") || new_target == get_stategraph().get_state("failed")){
            LOG_DEBUG("new target is stop, stop accepting new Worker connections");
            acc.stop();
        }
    }
    
private:
    shared_ptr<Logger> logger;
    
    TcpAcceptor & acc;
};

int main(int argc, char ** argv){
    if(argc != 2){
        cerr << "Usage: " << argv[0] << " <config file>" << endl;
        exit(1);
    }
    
    IOManager iom;    
    TcpAcceptor acc(iom);
    bool success = false;
    {
        Master master(argv[1]);
        iom.setup_signal_handler(SIGINT, [&](const siginfo_t &){ 
                cout << "SIGINT: aborting Master" << endl;
                master.abort();
        });
        
        shared_ptr<ProgressPrinter> pp(new ProgressPrinter());
        shared_ptr<Stopper> stopper(new Stopper(acc));
        master.add_observer(pp);
        master.add_observer(stopper);
        master.start();

        acc.start([&](unique_ptr<Channel> c){ master.add_worker(move(c)); }, "*", 5473);
        iom.process();
        pp.reset();
        success = !master.failed();
        // remove signal handler which refers to master which is about to be destroyed
        iom.reset_signal_handler(SIGINT, 0);
    }
    if(success){
        cout << "Master completed successfully." << endl;
    }
    else{
        cerr << "Data NOT processed completely; see log for details." << endl;
        return 1;
    }
}
