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
    auto logger  = Logger::get("ra.ra");
    AnalysisController controller(config, false);
    
    bool do_stop = false;
    // 3 nested loops to run over all datasets -> files -> events
    for(size_t idataset = 0; idataset < config.datasets.size(); idataset ++){
        if(do_stop) break;
        auto & dataset = config.datasets[idataset];
        string outfilename = config.options.output_dir + "/" + dataset.name;
        controller.start_dataset(idataset, outfilename);
        std::unique_ptr<progress_bar> p(new progress_bar("Progress for dataset '%(dataset)s': files: %(files)4ld / %(files_total)4ld; events: %(events)10ld (%(events)|rate|7.1f/s); data: %(mbytes)7.2f MB (%(mbytes)|rate|5.2fMB/s)"));
        identifier events("events");
        identifier mbytes("mbytes");
        p->set(mbytes, 0.0);
        p->set("files_total", dataset.files.size());
        p->set("dataset", dataset.name);
        p->set("files", 0);
        p->set("events", 0);
        p->print();
        size_t nevents_done = 0;
        size_t nbytes = 0;
        size_t nevents_survived = 0;
        for(size_t ifile=0; ifile < dataset.files.size(); ++ifile){
            controller.start_file(ifile);
            size_t nevents = controller.get_file_size();
            size_t imin = 0;
            while(true){
                if(do_stop || interrupted) break;
                size_t imax = min<size_t>(imin + config.options.blocksize, nevents);
                if(imin == imax) break;
                AnalysisController::ProcessStatistics s;
                controller.process(imin, imax, &s);
                nbytes += s.nbytes_read;
                nevents_survived += s.nevents_survived;
                nevents_done += imax - imin;
                p->set(events, nevents_done);
                p->set(mbytes, nbytes * 1e-6);
                p->check_autoprint();
                if(config.options.maxevents_hint > 0){
                    if(nevents_done >= (size_t)config.options.maxevents_hint){
                        do_stop = true;
                        break;
                    }
                }
                imin = imax;
            }
            if(do_stop || interrupted) break;
            p->set("files", ifile + 1);
            p->print();
        }
        if(do_stop || interrupted){
            break;
        }
        p.reset();
        cout << "Events survived for this dataset: " << nevents_survived << endl;
    }
    if(interrupted){
          LOG_WARNING("Interrupted by SIGINT, not all data has been processed.");
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
        cout << "error establishing signal handler (ignoring ...)" << endl;
    }
    
    try{
        s_config config(argv[1]);
        config.logger.apply(config.options.output_dir);
        ROOT::Cintex::Cintex::Enable();
        run(config);
    }
    catch(std::runtime_error & ex){
        cerr << "main: runtime error ocurred: " << ex.what() << endl;
        exit(1);
    }
}

