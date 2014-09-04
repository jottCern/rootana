#include "controller.hpp"
#include "utils.hpp"
#include "config.hpp"
#include "context.hpp"
#include "analysis.hpp"

#include "TFile.h"
#include "TTree.h"

using namespace ra;
using namespace std;

AnalysisController::AnalysisController(const s_config & config_, bool parallel): logger(Logger::get("ra.AnalysisController")),
  config(config_), current_idataset(-1), current_ifile(-1), infile_nevents(0) {
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
        if(parallel && !modules.back()->is_parallel_safe()){
            LOG_THROW("Error: module " << module_names.back() << " is not parallel safe.");
        }
    }
}

void AnalysisController::start_dataset(size_t idataset, const string & new_outfile_path){
    if(current_idataset == idataset && outfile_path == new_outfile_path) return;
    LOG_DEBUG("start_dataset idataset = " << idataset << "; outfile = " << new_outfile_path);
    // cleanup previous per-file info:
    current_ifile = -1;
    infile.reset();
    // cleanup previous per-dataset info, in reverse order of construction:
    in.reset();
    event.reset();
    out.reset();
    
    current_idataset = idataset;
    if(idataset == size_t(-1)) return;
    
    if(idataset >= config.datasets.size()){
        LOG_THROW("start_dataset (idataset = " << idataset << "): idataset out of range");
    }
    // initialize all per-dataset infos:
    std::unique_ptr<TFile> outfile(new TFile(new_outfile_path.c_str(), "recreate"));
    if(!outfile->IsOpen()){
        LOG_THROW("could not open output file '" + new_outfile_path + "'");
    }
    outfile_path = new_outfile_path;
    const s_dataset & dataset = config.datasets[current_idataset];
    event.reset(new Event());
    handle_stop = event->get_handle<bool>("stop");
    out.reset(new TFileOutputManager(move(outfile), dataset.treename, *event));
    in.reset(new TTreeInputManager(*event, config.options.lazy_read));
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
    LOG_DEBUG("start_file " << ifile);
    if(current_ifile == ifile) return;
    const s_dataset & dataset = current_dataset();
    if(ifile >= dataset.files.size()){
        throw invalid_argument("no such file in current dataset");
    }
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
    current_ifile = ifile;
    infile_nevents = tree->GetEntries();
}

size_t AnalysisController::get_file_size() const{
    check_file();
    return infile_nevents;
}

void AnalysisController::process(size_t imin, size_t imax, ProcessStatistics * stats){
    LOG_DEBUG("process events " << imin << " -- " << imax);
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
    size_t nevents_survived = 0;
    for(size_t ientry = imin; ientry < imax; ++ientry){
        try{
            in->read_entry(ientry);
        }
        catch(...){
            LOG_ERROR("Exception caught in read_entry while reading entry " << ientry << " of file " << current_dataset().files[current_ifile].path << "; re-throwing.");
            throw;
        }
        bool event_selected = true;
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
            if(event->get_state<bool>(handle_stop) == Event::state::valid && event->get(handle_stop)){
                event_selected = false;
                break;
            }
        }
        if(event_selected){
            ++nevents_survived;
            out->write_event();
        }
    }
    if(stats){
        stats->nbytes_read = in->nbytes_read();
        stats->nevents_survived = nevents_survived;
    }
}

AnalysisController::~AnalysisController(){
    start_dataset(-1, "");
}
