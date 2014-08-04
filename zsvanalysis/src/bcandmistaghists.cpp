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

namespace {
    ID(mistag_bpt_over_jetpt);
    ID(mistag_jetpt);
    ID(mistag_matched_dr);
    ID(tag_bpt_over_jetpt);
    ID(tag_jetpt);
    ID(tag_matched_dr);
    
    ID(jetpt_l);
    ID(jetpt_b);
    ID(jetpt_c);
    
    ID(jetpt_l_tagged);
    ID(jetpt_b_tagged);
    ID(jetpt_c_tagged);
    
    ID(jeteta_l);
    ID(jeteta_b);
    ID(jeteta_c);
    
    ID(jeteta_l_tagged);
    ID(jeteta_b_tagged);
    ID(jeteta_c_tagged);
    
    
}



class BcandMistagHists: public Hists{
public:
    BcandMistagHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, OutputManager & out);
    
    void fill(const identifier & id, double value){
        get(id)->Fill(value, current_weight);
    }
    
    void fill(const identifier & id, double xvalue, double yvalue){
        ((TH2*)get(id))->Fill(xvalue, yvalue, current_weight);
    }
    
    virtual void process(Event & e) override;
    
private:
    double current_weight;
};

BcandMistagHists::BcandMistagHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
    book<TH1D>(mistag_bpt_over_jetpt, 100, 0, 2);
    book<TH1D>(mistag_jetpt, 100, 0, 200);
    book<TH1D>(mistag_matched_dr, 60, 0, 0.6);
    
    book<TH1D>(tag_bpt_over_jetpt, 100, 0, 2);
    book<TH1D>(tag_jetpt, 100, 0, 200);
    book<TH1D>(tag_matched_dr, 60, 0, 0.6);
    
    book<TH1D>(jetpt_l, 100, 0, 200);
    book<TH1D>(jetpt_c, 100, 0, 200);
    book<TH1D>(jetpt_b, 100, 0, 200);
    
    book<TH1D>(jetpt_l_tagged, 100, 0, 200);
    book<TH1D>(jetpt_c_tagged, 100, 0, 200);
    book<TH1D>(jetpt_b_tagged, 100, 0, 200);
    
    book<TH1D>(jeteta_l, 60, -3, 3);
    book<TH1D>(jeteta_c, 60, -3, 3);
    book<TH1D>(jeteta_b, 60, -3, 3);
    
    book<TH1D>(jeteta_l_tagged, 60, -3, 3);
    book<TH1D>(jeteta_c_tagged, 60, -3, 3);
    book<TH1D>(jeteta_b_tagged, 60, -3, 3);
}

void BcandMistagHists::process(Event & e){
    if(!e.get<bool>(id::passed_reco_selection)){
         return;
    }
    current_weight = e.weight();
    const auto & bcands = e.get<vector<Bcand> >(id::selected_bcands);
    const auto & mc_bs = e.get<vector<mcparticle> >(id::mc_bs);
    const auto & mc_cs = e.get<vector<mcparticle> >(id::mc_cs);
    const auto & jets = e.get<vector<jet>>(id::jets);
    const auto & lp = e.get<lepton>(id::lepton_plus);
    const auto & lm = e.get<lepton>(id::lepton_minus);
    
    for(const auto & bcand : bcands){
        bool is_fake = true;
        for(const auto & mc_b : mc_bs){
            if(deltaR2(mc_b.p4, bcand.p4) < 0.1*0.1){
                is_fake = false; break;
            }
        }
        // Look for matching jet:
        int matched_ijet = -1;
        float matched_drbj = 0.5f; // note: unmatched will have matched_dr = 0.5
        for(size_t ij=0; ij < jets.size(); ++ij){
            float dr = deltaR(jets[ij].p4, bcand.p4);
            if(dr < 0.5f){
                matched_ijet = ij;
                matched_drbj = dr;
                break;
            }
        }
        fill(is_fake ? mistag_matched_dr : tag_matched_dr, matched_drbj);
        if(matched_ijet >= 0){
            const auto & jet = jets[matched_ijet];
            fill(is_fake ? mistag_bpt_over_jetpt : tag_bpt_over_jetpt, bcand.p4.pt() / jet.p4.pt());
            fill(is_fake ? mistag_jetpt : tag_jetpt, jet.p4.pt());
        }
    }
    
    // now start from the jets (only eta < 2.0; do not pay attention to transition effects right now):
    for(const auto & jet : jets){
        if(fabs(jet.p4.eta()) > 2.0) continue;
        // do not use jets that match to the primary muons:
        if(deltaR(jet.p4, lp.p4) < 0.5f || deltaR(jet.p4, lm.p4) < 0.5f) continue;
        
        // jet flavor: try to match b/c hadrons within DR < 0.5
        int jetfl = -1;
        for(const auto & mc_b : mc_bs){
            if(deltaR(mc_b.p4, jet.p4) < 0.5f){
                jetfl = 5;
                break;
            }
        }
        if(jetfl == -1){
            for(const auto & mc_c : mc_cs){
                if(deltaR(mc_c.p4, jet.p4) < 0.5f){
                    jetfl = 4;
                    break;
                }
            }
        }
        // now look if it's "tagged":
        bool tagged = false;
        for(const auto & bcand : bcands){
            if(deltaR(bcand.flightdir, jet.p4) < 0.5f){
                tagged = true;
                break;
            }
        }
        
        if(jetfl == 5){
            fill(jetpt_b, jet.p4.pt());
            fill(jeteta_b, jet.p4.eta());
            if(tagged){
                fill(jetpt_b_tagged, jet.p4.pt());
                fill(jeteta_b_tagged, jet.p4.eta());
            }
        }
        else if(jetfl == 4){
            fill(jetpt_c, jet.p4.pt());
            fill(jeteta_c, jet.p4.eta());
            if(tagged){
                fill(jetpt_c_tagged, jet.p4.pt());
                fill(jeteta_c_tagged, jet.p4.eta());
            }
        }
        else{
            fill(jetpt_l, jet.p4.pt());
            fill(jeteta_l, jet.p4.eta());
            if(tagged){
                fill(jetpt_l_tagged, jet.p4.pt());
                fill(jeteta_l_tagged, jet.p4.eta());
            }
            
        }
        
    }
    
    
}

REGISTER_HISTS(BcandMistagHists)
