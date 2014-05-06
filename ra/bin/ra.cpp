#include "base/include/log.hpp"

#include "config.hpp"
#include "analysis.hpp"
#include "controller.hpp"
#include "Cintex/Cintex.h"

#include <signal.h>

using namespace ra;
using namespace std;

namespace{
    
volatile sig_atomic_t interrupted = 0;

void sigint_handler(int){
    interrupted = 1;
}
    
}


void run(const s_config & config){
    AnalysisController controller(config);
    
    bool do_stop = false;
    // nested loops to run over all datasets -> files -> events
    size_t idataset = 0;
    for(const auto & dataset : config.datasets){
        if(do_stop) break;
        string outfilename = config.options.output_dir + "/" + dataset.name + ".root";
        controller.start_dataset(idataset, outfilename);
        progress_bar progress("Progress for dataset '%(dataset)s': files: %(files)4ld / %(files_total)4ld; events: %(events)10ld (%(events)|rate|7.1f/s)");
        identifier events("events");
        progress.set("files_total", dataset.files.size());
        progress.set("dataset", dataset.name);
        progress.set("files", 0);
        progress.set("events", 0);
        progress.print();
        size_t nevents_done = 0;
        for(size_t ifile=0; ifile < dataset.files.size(); ++ifile){
            controller.start_file(ifile);
            size_t nevents = controller.get_file_size();
            size_t imin = 0;
            while(true){
                if(do_stop || interrupted) break;
                size_t imax = min<size_t>(imin + config.options.blocksize, nevents);
                if(imin == imax) break;
                controller.process(imin, imax);
                nevents_done += imax - imin;
                progress.set("events", nevents_done);
                if(config.options.maxevents_hint > 0){
                    if(nevents_done >= (size_t)config.options.maxevents_hint){
                        do_stop = true;
                        break;
                    }
                }
                imin = imax;
            }
            if(do_stop || interrupted) break;
            progress.set("files", ifile + 1);
            progress.print();
        }
        if(do_stop){
            break;
        }
        if(interrupted){
            cout << "Interrupted by SIGINT" << endl;
            break;
        }
    }
}

int main(int argc, char ** argv){
    if(argc != 2){
        cerr << "Usage: " << argv[0] << " <config file>" << endl;
        exit(1);
    }
    struct sigaction sa;
    sa.sa_handler = &sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_restorer = 0;
    int res = sigaction(SIGINT, &sa, 0);
    if(res != 0){
        cout << "error establishing signal handler (iognoring ...)" << endl;
    }
    
    try{
        s_config config(argv[1]);
        config.logger.apply(config.options.output_dir);
        ROOT::Cintex::Cintex::Enable();
        // TODO: set up logging.
        run(config);
    }
    catch(std::runtime_error & ex){
        cerr << "main: runtime error ocurred: " << ex.what() << endl;
        exit(1);
    }
}

