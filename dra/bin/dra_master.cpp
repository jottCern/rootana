#include "master.hpp"
#include "local.hpp"
#include "stategraph.hpp"
#include "dc/include/netutils.hpp"

#include <fstream>

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

namespace {
  
void usage(const char * name, int code){
    cout << "Usage: " << name << " [-f pidfile] -p port config_file" << endl;
    exit(code);
}

}

int main(int argc, char ** argv){
    int port = -1;
    string pidfile;
    
    int c;
    while((c = getopt(argc, argv, "f:p:")) != -1){
      switch(c){
        case 'p':
          try {
             port = boost::lexical_cast<int>(optarg);
          } catch(boost::bad_lexical_cast&){} // port remains -1, triggering error below
          break;
        case 'f':
          pidfile = optarg;
          break;
        default:
          usage(argv[0], 1);
      }
    }
    
    // the only thing left chould be the config file:
    if(argc - 1 != optind || port <= 0){
       usage(argv[0], 1);
    }
    const string cfgfile = argv[argc-1];
    
    if(!pidfile.empty()){
        ofstream out(pidfile.c_str());
        if(out){
           out << getpid();
        }
        else{
           cerr << "Warning: could not create pid file " << pidfile << endl;
        }
    }
    
    IOManager iom;    
    TcpAcceptor acc(iom);
    bool success = false;
    bool stopped = false;
    {
        Master master(cfgfile);
        iom.setup_signal_handler(SIGINT, [&](const siginfo_t &){
            if(!stopped){
                stopped = true;
                cout << endl << "SIGINT: stopping Master" << endl;
                master.stop();
            }
            else{
                cout << endl<< "SIGINT: aborting Master" << endl;
                master.abort();
            }
        });
        
        shared_ptr<ProgressPrinter> pp(new ProgressPrinter());
        shared_ptr<Stopper> stopper(new Stopper(acc));
        master.add_observer(pp);
        master.add_observer(stopper);
        master.start();

        acc.start([&](unique_ptr<Channel> c){ master.add_worker(move(c)); }, "*", port);
        iom.process();
        pp.reset();
        success = master.completed();
        // remove signal handler which refers to master which is about to be destroyed
        iom.reset_signal_handler(SIGINT, 0);
    }
    if(success){
        cout << "Master: Processing completed successfully." << endl;
    }
    else{
        cerr << "Data NOT processed completely; see log for details." << endl;
        return 1;
    }
}
