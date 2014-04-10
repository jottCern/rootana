#include "hists.hpp"
#include "TFile.h"
#include "config.hpp"

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

void Hists::book_1d_autofill(event_functor f, const char * name, int nbins, double xmin, double xmax){
    TH1D * histo = book<TH1D>(name, nbins, xmin, xmax);
    autofill_histos.emplace_back(std::move(f), histo);
}

void Hists::process_all(Event & e){
    process(e);
    for(const auto & it : autofill_histos){
        double value = it.first(e);
        if(!std::isnan(value)){
            it.second->Fill(value, e.weight());
        }
    }
}

TH1* Hists::get(const identifier & id){
    auto it = i2h.find(id);
    if(it!=i2h.end()) return it->second;
    throw runtime_error("did not find histogram '" + id.name() + "'");
}

HistFiller::HistFiller(const ptree & cfg_): cfg(cfg_){}

void HistFiller::begin_dataset(const s_dataset & dataset, InputManager &, OutputManager & out){
    outdirs.clear();
    for(const auto & it : cfg){
        if(it.first=="type") continue;
        outdir od;
        od.dirname = it.first;
        od.selid = identifier(od.dirname);
        const ptree & dircfg = it.second;
        
        for(const auto & it2 : dircfg){
            if(it2.first == "hists"){
                ptree hists_cfg;
                std::string type;
                if(it2.second.size() > 0){ // it's a group of settings:
                    hists_cfg = it2.second;
                    type = ptree_get<string>(hists_cfg, "type");
                }
                else{
                    type = it2.second.data();
                }
                try{
                    od.hists.emplace_back(HistsRegistry::build(type, hists_cfg, od.dirname, dataset, out));
                }
                catch(runtime_error & ex){
                    throw runtime_error("Error while building Hists instance of type '" + type + "' for directory '" + od.dirname + "': " + ex.what());
                }
            }
            else if(it2.first == "selection"){
                od.selid = identifier(it2.second.data());
            }
            else{
                throw runtime_error("unknown setting '" + it.first + "' in HistFiller directory '" + od.dirname + "'");
            }
        }
        outdirs.emplace_back(move(od));
    }
}

void HistFiller::process(Event & event){
    for(auto & dir : outdirs){
        if(!event.get<bool>(dir.selid)) continue;
        for(auto & hf : dir.hists){
            hf->process_all(event);
        }
    }
}

REGISTER_ANALYSIS_MODULE(HistFiller)
