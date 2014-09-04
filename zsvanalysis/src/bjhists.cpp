#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/hists.hpp"

#include "eventids.hpp"

#include <iostream>
#include <set>
#include "TH1D.h"
#include "TH2D.h"
#include "zsvtree.hpp"

using namespace ra;
using namespace std;
using namespace zsv;


class BJHists: public Hists{
public:
    BJHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, InputManager & in, OutputManager & out): Hists(dirname, dataset, out){       
        book<TH1D>("nbj", 4, 0, 4); // number of b jets in event
        book<TH1D>("nbh", 4, 0, 4); // for each jet: number of b hadrons matching a b jet
        book<TH1D>("bj_pt", 200, 0, 200); // bjet pt
        book<TH1D>("bj_eta", 120, -3, 3); // bjet eta
        book<TH1D>("bc_pt_over_bj_pt", 200, 0, 1.5); // only for unique matches
        book<TH1D>("dr_bc_bj", 200, 0, 1.5); // only for unique matched
        
        h_weight = in.get_handle<double>("weight");
        h_selected_bcands = in.get_handle<vector<Bcand> >("selected_bcands");
        h_bjets = in.get_handle<vector<jet> >("bjets");
    }
    
    void fill(const identifier & id, double value){
        get(id)->Fill(value, current_weight);
    }
    
    void fill(const identifier & id, double xvalue, double yvalue){
        ((TH2*)get(id))->Fill(xvalue, yvalue, current_weight);
    }
    
    virtual void process(Event & e){
        ID(nbj);
        ID(nbh);
        ID(bj_pt);
        ID(bj_eta);
        ID(bc_pt_over_bj_pt);
        ID(dr_bc_bj);
        
        current_weight = e.get(h_weight);

        auto & bcands = e.get<vector<Bcand> >(h_selected_bcands);
        auto & b_jets = e.get(h_bjets);
        
        fill(nbj, b_jets.size());
        
        for(const auto & bjet : b_jets){
            fill(bj_pt, bjet.p4.pt());
            fill(bj_eta, bjet.p4.eta());
            int nb = 0;
            const Bcand * matching_bcand = 0;
            for(const auto & bc: bcands){
                if(deltaR(bc.flightdir, bjet.p4) < 0.5){
                    ++nb;
                    matching_bcand = &bc;
                }
            }
            fill(nbh, nb);
            if(nb==1){
                fill(bc_pt_over_bj_pt, matching_bcand->p4.pt() / bjet.p4.pt());
                fill(dr_bc_bj, deltaR(matching_bcand->flightdir, bjet.p4));
            }
        }
    }
    
private:
    double current_weight;
    Event::Handle<double> h_weight;
    Event::Handle<vector<jet>> h_bjets;
    Event::Handle<vector<Bcand>> h_selected_bcands;
};

REGISTER_HISTS(BJHists)
