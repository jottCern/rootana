#include "hists.hpp"
#include "TFile.h"
#include "config.hpp"

#include <boost/algorithm/string.hpp>

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>

using namespace std;
using namespace ra;

Hists::Hists(const std::string & dirname_, const s_dataset & dataset, OutputManager & out_): dirname(dirname_), out(out_){
    // ensure dirname end with '/':
    if(!dirname.empty()){
        if(dirname[dirname.size() - 1] != '/') dirname += '/';
    }
}

Hists::~Hists(){}

void Hists::book_1d_autofill(event_functor f, const char * name, int nbins, double xmin, double xmax, Event::Handle<double> weight_handle){
    TH1D * histo = book<TH1D>(name, nbins, xmin, xmax);
    autofill_histos.emplace_back(autofill_histo{move(f), weight_handle, histo});
}

void Hists::process_all(Event & e){
    process(e);
    const Event::Handle<double> invalid_handle;
    for(auto & it : autofill_histos){
        double value = it.f(e);
        if(std::isnan(value)) continue;
        if(it.weight_handle == invalid_handle){
            it.weight_handle = e.get_handle<double>("weight");
        }
        double weight = e.get(it.weight_handle);
        it.histo->Fill(value, weight);
    }
}

TH1* Hists::get(const identifier & id){
    auto it = i2h.find(id);
    if(it!=i2h.end()) return it->second;
    throw runtime_error("did not find histogram '" + id.name() + "'");
}

HistFiller::HistFiller(const ptree & cfg_): cfg(cfg_){}

// Fill od.hists
// od.dirname and od.selid should be set to their defaults.
void HistFiller::parse_dir_cfg(const ptree & dircfg, outdir & od, const s_dataset & dataset, InputManager & in, OutputManager & out){
    for(const auto & it : dircfg){
        if(it.first == "hists"){
            ptree hists_cfg;
            std::string type;
            if(it.second.size() > 0){ // it's a group of settings:
                hists_cfg = it.second;
                type = ptree_get<string>(hists_cfg, "type");
            }
            else{
                type = it.second.data();
            }
            try{
                od.hists.emplace_back(HistsRegistry::build(type, hists_cfg, od.dirname, dataset, in, out));
            }
            catch(runtime_error & ex){
                throw runtime_error("Error while building Hists instance of type '" + type + "' for directory '" + od.dirname + "': " + ex.what());
            }
        }
        else if(it.first == "selection"){
            od.sel_handle = in.get_handle<bool>(it.second.data());
        }
        else{
            throw runtime_error("unknown setting '" + it.first + "' in HistFiller directory '" + od.dirname + "'");
        }
    }
}

void HistFiller::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    outdirs.clear();
    boost::optional<ptree> last_dir_cfg;
    for(const auto & it : cfg){
        if(it.first=="type") continue;
        if(it.first=="_cfg"){
            last_dir_cfg = it.second;
        }
        else if(it.first=="_dirs"){
            if(!last_dir_cfg){
                throw runtime_error("_cfg needed before _dir!");
            }
            string selections = it.second.data();
            vector<string> vsels;
            boost::split(vsels, selections, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
            for(const auto & sel : vsels){
                outdir od;
                od.dirname = sel;
                od.sel_handle = in.get_handle<bool>(sel);
                parse_dir_cfg(*last_dir_cfg, od, dataset, in, out);
                outdirs.emplace_back(move(od));
            }
        }
        else{
            outdir od;
            od.dirname = it.first;
            od.sel_handle = in.get_handle<bool>(it.first);
            parse_dir_cfg(it.second, od, dataset, in, out);
            outdirs.emplace_back(move(od));
        }
    }
}

void HistFiller::process(Event & event){
    for(auto & dir : outdirs){
        if(!event.get(dir.sel_handle)) continue;
        size_t ihists = 0;
        for(auto & hf : dir.hists){
            try{
                hf->process_all(event);
            }
            catch(...){
                auto logger = Logger::get("Hists");
                LOG_ERROR("HistFiller::process: about to re-throwing exception in HistFiller dir " << dir.dirname << " hists " << ihists);
                throw;
            }
            ++ihists;
        }
    }
}

REGISTER_ANALYSIS_MODULE(HistFiller)
