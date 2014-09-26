#include "ra/include/analysis.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/event.hpp"
#include "ra/include/utils.hpp"

#include "zsvtree.hpp"

using namespace ra;
using namespace std;

class ttdec: public AnalysisModule {
public:
    explicit ttdec(const ptree & cfg){}
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    bool is_real_data;
    
    Event::Handle<vector<mcparticle>> h_mc_partons;
    Event::Handle<int> h_ttdec;
};


void ttdec::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    is_real_data = dataset.tags.get<bool>("is_real_data");
    h_mc_partons = in.get_handle<vector<mcparticle>>("mc_partons");
    h_ttdec = in.get_handle<int>("ttdec");
}

// -1 = unknown (sample is not ttbar MC / sample is data)
// 0 = hadron, 1 = e+jets, 2 = mu+jets, 3 = tau+jets, 4 = ee, 5 = emu, 6 = etau, 7 = mm, 8 = mt, 9 = tt
void ttdec::process(Event & event){
    if(is_real_data){
        event.set(h_ttdec, -1);
        return;
    }
    const auto & mc_partons = event.get(h_mc_partons);
    int ntop = 0, nq = 0, ne = 0, nm = 0, nt = 0;
    for(const auto & p : mc_partons){
        if(abs(p.pdgid) < 6) ++nq;
        else if(abs(p.pdgid)==6) ++ntop;
        else if(abs(p.pdgid)==11) ++ne;
        else if(abs(p.pdgid)==13) ++nm;
        else if(abs(p.pdgid)==15) ++nt;
    }
    if(ntop != 2){
        event.set(h_ttdec, -1);
        return;
    }
    
    int nl = ne + nm + nt;
    
    if(nl == 0){
        event.set(h_ttdec, 0);
    }
    else if(nl == 1){
        if(ne > 0){
            event.set(h_ttdec, 1);
        }
        else if(nm > 0){
            event.set(h_ttdec, 2);
        }
        else if(nt > 0){
            event.set(h_ttdec, 3);
        }
        else{
            cout << "unexpected gen lepton counts for nq=4: ne=" << ne << "; nm=" << nm << "; nt=" << nt << endl;
        }
    }
    else if(nl == 2){
        if(ne == 2){
            event.set(h_ttdec, 4);
        }
        else if(ne==1 and nm == 1){
            event.set(h_ttdec, 5);
        }
        else if(ne==1 and nt == 1){
            event.set(h_ttdec, 6);
        }
        else if(nm==2){
            event.set(h_ttdec, 7);
        }
        else if(nm==1 and nt == 1){
            event.set(h_ttdec, 8);
        }
        else if(nt==2){
            event.set(h_ttdec, 9);
        }
        else{
            cout << "unexpected gen lepton counts for nq=2: ne=" << ne << "; nm=" << nm << "; nt=" << nt << endl;
        }
    }
    else{
        cout << "unexpected nl=" << nl << endl;
        for(const auto & p : mc_partons){
            cout << p.pdgid << " " << p.p4 << endl;
        }
    }
}

REGISTER_ANALYSIS_MODULE(ttdec)
