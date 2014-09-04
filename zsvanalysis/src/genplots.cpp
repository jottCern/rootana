#include "ra/include/event.hpp"
#include "ra/include/hists.hpp"
#include "ra/include/utils.hpp"
#include "base/include/ptree-utils.hpp"

#include <iostream>
#include <set>
#include "TH1D.h"
#include "TH2D.h"

#include "zsvtree.hpp"

using namespace ra;
using namespace std;

namespace {

ID(ptb0);
ID(ptb1);
ID(etab0);
ID(etab1);
ID(nb);
ID(drbb);
ID(dphibb);

ID(minpt_drbb);
ID(maxpt_drbb);
ID(sumpt_drbb);
ID(reldiffpt_drbb);


}


class GenPlots: public Hists {
public:
    GenPlots(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, InputManager & in, OutputManager & out): Hists(dirname, dataset, out){
        string mc_bs_input = ptree_get<string>(cfg, "mc_bs_input");
        h_mc_bs_input = in.get_handle<vector<mcparticle>>(mc_bs_input);
        h_weight = in.get_handle<double>("weight");
        
        book<TH1D>(ptb0, 200, 0, 200);
        book<TH1D>(ptb1, 200, 0, 200);
        book<TH1D>(etab0, 180, -6, 6);
        book<TH1D>(etab1, 180, -6, 6);
        
        book<TH1D>(nb, 10, 0, 10);
        
        // in case of >= 2 b hadrons (using the leading B hadrons):
        book<TH1D>(drbb, 250, 0, 5);
        book<TH1D>(dphibb, 160, 0, 3.2);
        
        book<TH2D>(minpt_drbb, 100, 0, 5, 200, 0, 200);
        book<TH2D>(maxpt_drbb, 100, 0, 5, 200, 0, 200);
        book<TH2D>(sumpt_drbb, 100, 0, 5, 200, 0, 200);
        book<TH2D>(reldiffpt_drbb, 100, 0, 5, 100, 0, 1);
    }
    
    void fill(const identifier & id, double value){
        get(id)->Fill(value, current_weight);
    }
    
    void fill2d(const identifier & id, double xvalue, double yvalue){
        static_cast<TH2*>(get(id))->Fill(xvalue, yvalue, current_weight);
    }
    
    void process(Event & e){
        current_weight = e.get<double>(h_weight);
        vector<mcparticle> bs = e.get(h_mc_bs_input);
        sort(bs.begin(), bs.end(), [](const mcparticle & i, const mcparticle & j){ return i.p4.pt() > j.p4.pt();});
        
        fill(nb, bs.size());
        
        if(bs.size() >= 1){
            fill(ptb0, bs[0].p4.pt());
            fill(etab0, bs[0].p4.eta());
            if(bs.size() >= 2){
                fill(ptb1, bs[1].p4.pt());
                fill(etab1, bs[1].p4.eta());
                
                float dr = deltaR(bs[0].p4, bs[1].p4);
                fill(drbb, dr);
                fill(dphibb, deltaPhi(bs[0].p4, bs[1].p4));
                
                fill2d(minpt_drbb, dr, min(bs[0].p4.pt(), bs[1].p4.pt()));
                fill2d(maxpt_drbb, dr, max(bs[0].p4.pt(), bs[1].p4.pt()));
                fill2d(sumpt_drbb, dr, bs[0].p4.pt() + bs[1].p4.pt());
                fill2d(reldiffpt_drbb, dr, (bs[0].p4.pt() - bs[1].p4.pt()) / (bs[0].p4.pt() + bs[1].p4.pt()));
            }
        }
    }
    
private:
    double current_weight;
    Event::Handle<vector<mcparticle>> h_mc_bs_input;
    Event::Handle<double> h_weight;
};

REGISTER_HISTS(GenPlots)

