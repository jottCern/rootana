#include "master.hpp"
#include "local.hpp"
#include "dc/include/netutils.hpp"

using namespace dra;
using namespace dc;
using namespace std;

class Stopper: public MasterObserver {
public:
    Stopper(IOManager & iom_, TcpAcceptor & acc_): iom(iom_), acc(acc_){}
    
    virtual void set_master(Master * master_){
        master = master_;
    }
    
    virtual void on_state_transition(const WorkerId & w1, const StateGraph::StateId & from, const StateGraph::StateId & to){
        if(master->failed() || master->all_done()){
            acc.stop();
            iom.stop();
        }
    }
    
private:
    const Master * master;
    IOManager & iom;
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
        shared_ptr<ProgressPrinter> pp(new ProgressPrinter());
        shared_ptr<Stopper> stopper(new Stopper(iom, acc));
        master.add_observer(pp);
        master.add_observer(stopper);
        master.start();
    
    
        acc.start([&](unique_ptr<Channel> c){
                master.add_worker(move(c));
            }
        , "*", 5473);
        iom.process();
        pp.reset();
        success = master.all_done();
    }
    
    if(success){
        cout << "Master completed successfully." << endl;
    }
    else{
        cerr << "Configuration NOT processed completely; see log for details." << endl;
    }
}
