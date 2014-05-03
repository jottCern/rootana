#include "controller.hpp"
#include "utils.hpp"
#include "config.hpp"
#include "context.hpp"
#include "analysis.hpp"

#include "TFile.h"
#include "TTree.h"

using namespace ra;
using namespace std;

namespace {
ra::identifier id_stop("stop");
}

AnalysisController::AnalysisController(const s_config & config_): logger(Logger::get("dra.AnalysisController")),
  config(config_), current_idataset(-1), current_ifile(-1), infile_nevents(0), n_read(0){
    for(const string & sp : config.options.searchpaths){
        add_searchpath(sp, -1);
    }
    // load libraries:
    for(const string & lib : config.options.libraries){
        load_lib(lib);
    }
    
    // construct modules:
    for(auto & module_cfg : config.modules_cfg){
        const string & name = module_cfg.first;
        string type = ptree_get<string>(module_cfg.second, "type");
        modules.emplace_back(AnalysisModuleRegistry::build(type, module_cfg.second));
        module_names.push_back(name);
    }
}

void AnalysisController::start_dataset(size_t idataset, const string & new_outfile_path){
    bool cleanup_file = true;
    if(current_idataset == idataset){
        if(outfile_path == new_outfile_path) return;
        cleanup_file = false;
    }
    // cleanup previous per-file info:
    if(cleanup_file){
       current_ifile = -1;
       infile.reset();
    }
    // cleanup previous per-dataset info, in reverse order of construction:
    in.reset();
    event.reset();
    out.reset();
    // TODO: review outfile close sequence ...
    if(outfile){
        outfile->cd();
        outfile->Write();
        outfile->Close();
        outfile.reset();
    }
    
    if(idataset == size_t(-1)) return;
    
    if(idataset >= config.datasets.size()){
        LOG_THROW("start_dataset ( idataset = " << idataset << "): idataset out of range");
    }
    // initialize all per-dataset infos:
    current_idataset = idataset;
    outfile.reset(new TFile(new_outfile_path.c_str(), "recreate"));
    if(!outfile->IsOpen()){
        LOG_THROW("could not open output file '" + new_outfile_path + "'");
    }
    outfile_path = new_outfile_path;
    const s_dataset & dataset = config.datasets[current_idataset];
    out.reset(new TFileOutputManager(outfile.get(), dataset.treename));
    event.reset(new Event());
    in.reset(new TTreeInputManager(*event));
    for(auto & m : modules){
        m->begin_dataset(dataset, *in, *out);
    }
}

const s_dataset & AnalysisController::current_dataset() const{
    check_dataset();
    return config.datasets[current_idataset];
}


void AnalysisController::check_dataset() const{
    // includes the case of current_idataset = size_t(-1)
    if(current_idataset >= config.datasets.size()){
        throw invalid_argument("no dataset initialized");
    }
}

void AnalysisController::check_file() const{
    if(current_ifile >= current_dataset().files.size()){
        throw invalid_argument("no file initialized");
    }
}

void AnalysisController::start_file(size_t ifile){
    if(current_ifile == ifile) return;
    check_dataset();
    const s_dataset & dataset = current_dataset();
    const auto & f = dataset.files[ifile];
    infile.reset(new TFile(f.path.c_str(), "read"));
    if(!infile->IsOpen()){
        LOG_THROW("start_file: could not open '" << f.path << "' for reading");
    }
    for(auto & m : modules){
        m->begin_in_file(*infile);
    }
    TTree * tree = dynamic_cast<TTree*>(infile->Get(dataset.treename.c_str()));
    if(!tree){
        LOG_THROW("start_file: did not find input tree '" << dataset.treename << "' in input file '" << f.path << "'");
    }
    in->setup_tree(tree);
    infile_nevents = tree->GetEntries();
}

size_t AnalysisController::get_file_size() const{
    check_file();
    return infile_nevents;
}

void AnalysisController::process(size_t imin, size_t imax){
    check_file();
    if(infile_nevents == 0) return;
    if(imin >= infile_nevents){
        LOG_THROW("process: imin = " << imin << " specified, but file has only " << infile_nevents << " events");
    }
    if(imax > infile_nevents){
        LOG_DEBUG("process: imax = " << imax << " truncated to " << infile_nevents);
        imax = infile_nevents;
    }
    if(imax == imin) return;
    if(imax < imin){
        LOG_THROW("process called with imax < imin");
    }
    for(size_t ientry = imin; ientry < imax; ++ientry){
        n_read += in->read_entry(ientry);
        for(size_t i=0; i<modules.size(); ++i){
            try{
                modules[i]->process(*event);
            }
            catch(...){
                LOG_ERROR("Exception caught while calling 'process' method of module " << i << " (name: "
                          << module_names[i] << ") for entry " << ientry << " of file "
                          << current_dataset().files[current_ifile].path << "; re-throwing.");
                throw;
            }
            if(event->get_presence<bool>(id_stop) == Event::presence::present && event->get<bool>(id_stop)){
                break;
            }
        }
    }
}

AnalysisController::~AnalysisController(){
    start_dataset(-1, "");
}

size_t AnalysisController::nbytes_read(){
    size_t result = n_read;
    n_read = 0;
    return result;
}

