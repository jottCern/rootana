#include "local.hpp"
#include "Cintex/Cintex.h"

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
        if(nworkers <= 0 || nworkers > 1000){
            cerr << "nworkers <= 0 or > 1000 is not supported" << endl;
            exit(1);
        }
    }
    else{
        nworkers = 16; // TODO: read out hardware concurrency!
    }
    
    try{
        ROOT::Cintex::Cintex::Enable();
        std::shared_ptr<ProgressPrinter> pp(new ProgressPrinter());
        dra::local_run(argv[1], nworkers, pp);
        // TODO: set up logging.
    }
    catch(std::runtime_error & ex){
        cerr << "main: runtime error ocurred: " << ex.what() << endl;
        exit(1);
    }
}

