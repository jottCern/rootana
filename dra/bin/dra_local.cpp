#include "local.hpp"
#include <thread>

using namespace dra;
using namespace std;

int main(int argc, char ** argv){
    if(argc < 2 || argc > 3){
        cerr << "Usage: " << argv[0] << " <config file> [nworkers]" << endl;
        exit(1);
    }
    
    int nworkers;
    if(argc == 3){
        nworkers = boost::lexical_cast<int>(argv[2]);
        if(nworkers <= 0 || nworkers > 100){
            cerr << "nworkers <= 0 or > 100 is not supported" << endl;
            exit(1);
        }
    }
    else{
         // use half the available cores, but not more than 8 workers:
        nworkers = std::min(std::thread::hardware_concurrency() / 2, 8u);
        if(nworkers == 0){
            nworkers = 4;
        }
    }
    
    try{
        std::shared_ptr<ProgressPrinter> pp(new ProgressPrinter());
        if(!dra::local_run(argv[1], nworkers, pp)){
            cerr << endl << "WARNING: Dataset has NOT been processed completely (see above / log for details)" << endl;
            exit(1);
        }
    }
    catch(std::exception & ex){
        cerr << argv[0] << " in main: exception ocurred: " << ex.what() << endl;
        exit(1);
    }
}
