#include "worker.hpp"
#include "dc/include/iomanager.hpp"
#include "dc/include/netutils.hpp"

#include <boost/lexical_cast.hpp>

using namespace dra;
using namespace dc;
using namespace std;

namespace {
    
bool run_worker(int socket){
    auto logger = Logger::get("dra.local_run");
    IOManager iom;
    Worker w;
    w.setup(unique_ptr<Channel>(new Channel(socket, iom)));
    try {
        iom.process();
    }
    catch(exception & ex){
        LOG_ERROR("Exception during processing: '" << ex.what() << "'");
        return false;
    }
    catch(...){
        LOG_ERROR("Unknown exception during processing");
        return false;
    }
    if(w.stopped_successfully()){
        LOG_INFO("exiting run_worker; Worker stopped successfully.");
        return true;
    }
    else{
        LOG_ERROR("exiting run_worker; Worker was not successful.");
        return false;
    }
}

}


int main(int argc, char ** argv){
    if(argc != 3){
        cerr << "Usage: " << argv[0] << " master-host master-port" << endl;
        exit(1);
    }
    
    try{
        int port = boost::lexical_cast<int>(argv[2]);
        int socket = connect_to(argv[1], port);
        bool success = run_worker(socket);
        if(success){
            cout << "worker exiting successfully" << endl;
        }
        else{
            cerr << "worker NOT successfull (see log for details)" << endl;
        }
    }
    catch(std::runtime_error & ex){
        cerr << "main: runtime error ocurred: " << ex.what() << endl;
        exit(1);
    }
}
