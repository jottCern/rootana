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
}

void BcandMistagHists::process(Event & e){
    current_weight = e.weight();
    const auto & bcands = e.get<vector<Bcand> >(id::selected_bcands);
    const auto & mc_bs = e.get<vector<mcparticle> >(id::mc_bs);
    const auto & jets = e.get<vector<jet>>(id::jets);
    
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
            if(dr < 0.3f){
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
}

REGISTER_HISTS(BcandMistagHists)
