#include "selections.hpp"
#include "context.hpp"

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace ra;

Selections::Selections(const ptree & cfg_): cfg(cfg_){
}

void Selections::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    selections.clear();
    for(const auto & setting : cfg){
        if(setting.first == "type") continue;
        identifier id(setting.first);
        selections.emplace_back(id, SelectionRegistry::build(setting.second.get<string>("type"), setting.second, out));
    }
}
    
void Selections::process(Event & event){
    for(const auto & id_sel : selections){
        Selection & sel = *(get<1>(id_sel));
        bool passed = sel(event);
        event.set<bool>(get<0>(id_sel), passed);
    }
}

REGISTER_ANALYSIS_MODULE(Selections)

stop_unless::stop_unless(const ptree & cfg){
    selection = ptree_get<string>(cfg, "selection");
}

void stop_unless::process(Event & event){
    if(event.get_state<bool>(selection) != Event::state::valid || !event.get<bool>(selection)){
        event.set<bool>(fwid::stop, true);
    }
}

REGISTER_ANALYSIS_MODULE(stop_unless)

AndSelection::AndSelection(const ptree & cfg, OutputManager & out): cutflow(0), cutflow_raw(0){
    // get selections as string:
    std::string ids = cfg.get<string>("selections");
    vector<string> vids;
    boost::split(vids, ids, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
    for(const string & id : vids){
        selids.emplace_back(id);
    }
    string hname = cfg.get<string>("cutflow_hname", "");
    if(!hname.empty()){
        cutflow = new TH1D(hname.c_str(), hname.c_str(), vids.size()+1, 0, vids.size()+1);
        cutflow_raw = new TH1D((hname + "_raw").c_str(), (hname + "_raw").c_str(), vids.size()+1, 0, vids.size()+1);
        for(TH1D * h : {cutflow, cutflow_raw}){
            h->Sumw2();
            TAxis * x = h->GetXaxis();
            x->SetBinLabel(1, "before cuts");
            for(auto i=0ul; i < vids.size(); ++i){
                x->SetBinLabel(i+2, vids[i].c_str());
            }
            out.put(h->GetName(), h);
        }
    }
}
    
bool AndSelection::operator()(const Event & event){
    auto w = event.get<double>(fwid::weight);
    bool result = true;
    float x = 0.5;
    if(cutflow){
        cutflow->Fill(x, w);
        cutflow_raw->Fill(x);
    }
    for(const auto & id : selids){
        x += 1;
        if(!event.get<bool>(id)){
            result = false;
            break;
        }
        if(cutflow){
            cutflow->Fill(x, w);
            cutflow_raw->Fill(x);
        }
    }
    return result;
}

REGISTER_SELECTION(AndSelection)
REGISTER_SELECTION(PassallSelection)

AndNotSelection::AndNotSelection(const ptree & cfg, OutputManager & out){
    std::string ids = cfg.get<string>("selections");
    vector<string> vids;
    boost::split(vids, ids, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
    for(const string & id : vids){
        selids.emplace_back(id);
    }
}

bool AndNotSelection::operator()(const Event & event){
     // return true iff no selection has been passed:
     return none_of(selids.begin(), selids.end(), [&event](const identifier & id){return event.get<bool>(id);});
}

REGISTER_SELECTION(AndNotSelection)
