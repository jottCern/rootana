#include "base/include/log.hpp"

#include "config.hpp"
#include "analysis.hpp"
#include "context.hpp"
#include "TFile.h"
#include "TTree.h"
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
    Logger & logger = Logger::get("dra_local.run");
    
    // add searchpaths:
    for(const string & sp : config.options.searchpaths){
        add_searchpath(sp, -1);
    }
    // load libraries:
    for(const string & lib : config.options.libraries){
        load_lib(lib);
    }
    
    // construct modules:
    std::vector<std::unique_ptr<AnalysisModule>> modules;
    std::vector<std::string> module_names;
    for(auto & module_cfg : config.modules_cfg){
        const string & name = module_cfg.first;
        string type = ptree_get<string>(module_cfg.second, "type");
        std::unique_ptr<AnalysisModule> module = AnalysisModuleRegistry::build(type, module_cfg.second);
        modules.emplace_back(move(module));
        module_names.push_back(name);
    }
    
    ID(stop);
    
    // nested loops to run over all datasets -> files -> events
    for(const auto & dataset : config.datasets){
        string outfilename = config.options.output_dir + "/" + dataset.name + ".root";
        TFile * outfile = new TFile(outfilename.c_str(), "recreate");
        if(!outfile->IsOpen()){
            LOG_THROW("could not open output file '" + outfilename + "'");
        }
        TFileOutputManager out(outfile, dataset.treename);
        Event event;
        TTreeInputManager in(event);
        for(auto & m : modules){
            m->begin_dataset(dataset, in, out);
        }
        
        progress_bar progress("Progress for dataset '%(dataset)s': files: %(files)4ld / %(files_total)4ld; events: %(events)10ld (%(events)|rate|7.1f/s)");
        identifier events("events");
        progress.set("files_total", dataset.files.size());
        progress.set("dataset", dataset.name);
        progress.set("files", 0);
        progress.set("events", 0);
        progress.print();
        size_t ifile = 0;
        size_t ievent = 0;
        for(auto & f : dataset.files){
            TFile infile(f.path.c_str(), "read");
            if(!infile.IsOpen()){
                throw runtime_error("could not open '" + f.path + "' for reading");
            }
            for(auto & m : modules){
                m->begin_in_file(infile);
            }
            TTree * tree = dynamic_cast<TTree*>(infile.Get(dataset.treename.c_str()));
            if(!tree){
                throw runtime_error("did not find input tree '" + dataset.treename + "' in input file '" + f.path + "'");
            }
            in.setup_tree(tree);
            const size_t nentries = tree->GetEntries();
            size_t max_ientry = nentries;
            if(config.options.maxevents_hint > 0){
                max_ientry = min<size_t>(nentries, f.skip + config.options.maxevents_hint);
            }
            for(size_t ientry=f.skip; ientry < max_ientry; ++ientry){
                in.read_entry(ientry);
                for(size_t i=0; i<modules.size(); ++i){
                    try{
                        //cout << "module " << module_names[i] << endl;
                        modules[i]->process(event);
                    }
                    catch(...){
                        cerr << endl << "Exception caught while calling 'process' method of module " << module_names[i] << " for entry " << ientry << " of file " << f.path << "; re-throwing. " << endl;
                        throw;
                    }
                    if(event.present<bool>(stop) && event.get<bool>(stop)){
                        break;
                    }
                }
                ievent++;
                progress.set(events, ievent);
                progress.check_autoprint();
                if(interrupted) break;
            }
            if(interrupted) break;
            if(config.options.maxevents_hint > 0){
                if(ievent >= (size_t)config.options.maxevents_hint) break;
            }
            ifile++;
            progress.set("files", ifile);
            progress.print();
        }
        outfile->cd();
        outfile->Write();
        outfile->Close();
        delete outfile;
        if(interrupted){
            cout << "Interrupted by SIGINT" << endl;
            return;
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
        ROOT::Cintex::Cintex::Enable();
        // TODO: set up logging.
        run(config);
    }
    catch(std::runtime_error & ex){
        cerr << "main: runtime error ocurred: " << ex.what() << endl;
        exit(1);
    }
}

