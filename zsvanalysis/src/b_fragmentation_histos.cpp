#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/hists.hpp"
#include "ra/include/utils.hpp"

#include <iostream>
#include <set>
#include "TH1D.h"
#include "TH2D.h"
#include "zsvtree.hpp"

using namespace ra;
using namespace std;

namespace{


LorentzVector dir(const mcparticle & p){
    return p.p4;
}

LorentzVector dir(const LorentzVector & p){
    return p;
}

decltype(Bcand::flightdir) dir(const Bcand & p) {
    return p.flightdir;
}

LorentzVector dir(const jet & p){
    return p.p4;
}

struct match_info {
    size_t index;
    double dr;
};


// match a given B to one (or more) jets. Return all matches within dr_max, sorted by increasing dr.
// As Bs can be either gen or reco and jets can be gen or reco, make as template.
// jettype must have LorentzVector 'p4' member.
template<typename Btype, typename jettype>
vector<match_info> find_matching_jets(const Btype & b, const vector<jettype> & jets, double dr_max = 0.5){
    vector<match_info> result;
    for(size_t i=0; i<jets.size(); ++i){
        double dr = deltaR(dir(jets[i]), dir(b));
        if(dr < dr_max){
            result.push_back(match_info{i, dr});
        }
    }
    sort(result.begin(), result.end(), [](const match_info & m1, const match_info & m2){ return m1.dr < m2.dr;});
    return result;
}

vector<match_info> merge_matches(const vector<match_info> & m1, const vector<match_info> & m2){
    vector<match_info> result;
    if(m1.empty() || m2.empty()) return result; // empty result if not both are matched
    if(m1[0].index == m2[0].index){
        result.push_back(m1[0]);
    }
    else{
        result.push_back(m1[0]);
        result.push_back(m2[0]);
    }
    return result;
}


// try to match both B candidates to actual B hadrons.
// returns:
// 2 if both B candidates can be matched to two different B hadrons
// 1 if both B candidates can be matched to the same B hadron
// 0 otherwise.
// Matching is done with Delta R only (flightdir of B cand vs. p4 of MC B hadron) using dr < dr_max
int match_bc_mcb(const vector<Bcand> & bcands, const vector<mcparticle> & mc_bs, double dr_max = 0.3){
    assert(bcands.size() == 2);
    std::set<size_t> matching_bhads;
    bool bc0_matched = false, bc1_matched = false;
    for(size_t i=0; i< mc_bs.size(); ++i){
        if(deltaR(mc_bs[i].p4, bcands[0].flightdir) < dr_max){
            bc0_matched = true;
            matching_bhads.insert(i);
        }
        if(deltaR(mc_bs[i].p4, bcands[1].flightdir) < dr_max){
            bc1_matched = true;
            matching_bhads.insert(i);
        }
    }
    if(bc0_matched && bc1_matched){
        if(matching_bhads.size() >= 2) return 2;
        else if(matching_bhads.size() == 1) return 1;
        assert(false);
    }
    return 0;
}

    
}


// B fragmentation, i.e. pt(B hadron) / pt(B jet) for both gen and reco.
// B fragmentation could be used to separate (on a statistical level) unmerged B->D chains reconstructed as B+B from actual B+B,
// as the former should have much smaller fragmentation values than the latter.
class BFragmentationHists: public Hists{
public:
    BFragmentationHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, InputManager & in, OutputManager & out): Hists(dirname, dataset, out){
        h_mc_bs = in.get_handle<vector<mcparticle>>("mc_bs");
        h_mc_jets = in.get_handle<vector<LorentzVector>>("mc_jets");
        h_weight = in.get_handle<double>("weight");
        h_selected_bcands = in.get_handle<vector<Bcand>>("selected_bcands");
        //h_lepton_plus = in.get_handle<lepton>("lepton_plus");
        //h_lepton_minus = in.get_handle<lepton>("lepton_minus");
        h_jets = in.get_handle<vector<jet>>("jets");
        
        // fragmentation on gen level, i.e. B hadron pt over genjet pt, for 'isolated' B hadrons, i.e. no other B hadron
        // within DR < 0.5, to avoid the 'overlapping jets' topology issues. Filled for each B hadron; in case no jet is
        // matched, set to 2.0 - epsilon ("infinity" in this plot). Filled for each B hadron with pt > 15GeV.
        // y-axis = bfrag, x-axis = pt of either the jet or the B hadron
        book<TH2D>("bfrag_jpt_mc_isob", 100, 0.0, 200.0, 100, 0.0, 2.0);
        book<TH2D>("bfrag_bpt_mc_isob", 100, 0.0, 200.0, 100, 0.0, 2.0);
        
        // MC B fragmentation for 2 Bs versus Delta R: match both B hadrons to jets (also allow only one matching jet) and 
        // fill y-axis = sum of B hadron pt over sum of matched jet pt over x-axis = Delta R(B hadrons).
        // Filled for each event with exactly 2 B hadrons with pt > 15GeV
        book<TH2D>("bfrag2_dr_mc", 100, 0.0, 5.0, 100, 0.0, 2.0);
        book<TH2D>("bfrag2_nj_dr_mc", 100, 0.0, 5.0, 3, 0, 3); // number of jets used in bfrag2_dr_mc
        book<TH2D>("drj_drb_mc", 100, 0, 5, 100, 0, 5); // y = DR(j,j) x = DR(b,b)
        
        
        // same plots on reco level. B hadrons -> B candidates. mc jets -> reco jets
        book<TH2D>("bfrag_jpt_isob", 100, 0.0, 200.0, 100, 0.0, 2.0);
        book<TH2D>("bfrag_bpt_isob", 100, 0.0, 200.0, 100, 0.0, 2.0);
        book<TH2D>("drj_drb", 100, 0, 5, 100, 0, 5); // y = DR(j,j) x = DR(b,b)
        
        book<TH2D>("bfrag2_dr", 100, 0.0, 5.0, 100, 0.0, 2.0); // all events
        book<TH2D>("bfrag2_nj_dr", 100, 0.0, 5.0, 3, 0.0, 3.0);
        book<TH2D>("bfrag2_dr_mcbb", 100, 0.0, 5.0, 100, 0.0, 2.0); // same as bfrag2_dr, but only events identified as correctly reconstructed BB events (=both B cands match to two different Bs)
        book<TH2D>("bfrag2_dr_mcbd", 100, 0.0, 5.0, 100, 0.0, 2.0); // same as bfrag2_dr, but only events identified as wrongly reconstructed BD events (=both B cands match to the same B)
    }
    
    void fill(const identifier & id, double value){
        get(id)->Fill(value, current_weight);
    }
    
    void fill(const identifier & id, double xvalue, double yvalue){
        ((TH2*)get(id))->Fill(xvalue, yvalue, current_weight);
    }
    
    virtual void process(Event & e){
        current_weight = e.get<double>(h_weight);

        auto & bcands = e.get<vector<Bcand> >(h_selected_bcands);
        auto & all_mc_bs = e.get<vector<mcparticle> >(h_mc_bs);
        auto & jets = e.get(h_jets);
        auto & mc_jets = e.get(h_mc_jets);
        
        ID(bfrag_jpt_mc_isob); ID(bfrag_bpt_mc_isob);
        ID(bfrag2_dr_mc); ID(bfrag2_nj_dr_mc);
        ID(drj_drb_mc);
        
        vector<mcparticle> mc_bs; // visible Bs
        
        for(const auto & mc_b : all_mc_bs){
            if(mc_b.p4.pt() > 15) mc_bs.push_back(mc_b);
            // try to match a jet:
            for(const auto & jet : mc_jets){
                if(deltaR(mc_b.p4, jet) < 0.3){
                    double bfrag = mc_b.p4.pt() / jet.pt();
                    fill(bfrag_jpt_mc_isob, jet.pt(), bfrag);
                    fill(bfrag_bpt_mc_isob, mc_b.p4.pt(), bfrag);
                }
            }
        }
        
        if(mc_bs.size() == 2){
            auto b0_matches = find_matching_jets(mc_bs[0], mc_jets);
            //bool bs_isolated = deltaR(mc_bs[0].p4, mc_bs[1].p4) > 0.5;
            auto b1_matches = find_matching_jets(mc_bs[1], mc_jets);
            double drbb = deltaR(mc_bs[0].p4, mc_bs[1].p4);
            double drjj = 0.1; // = not both can be matched
            if(!b0_matches.empty() && !b1_matches.empty()){
                drjj = deltaR(mc_jets[b0_matches[0].index], mc_jets[b1_matches[0].index]); // can be 0.0 in case both match to the same jet
            }
            fill(drj_drb_mc, drbb, drjj);
            // fill bfrag2 plots:
            auto all_matches = merge_matches(b0_matches, b1_matches);
            double sum_jpt = 0.0;
            if(all_matches.size() > 0){
                sum_jpt += mc_jets[all_matches[0].index].pt();
            }
            if(all_matches.size() > 1){
                sum_jpt += mc_jets[all_matches[1].index].pt();
            }
            double bfrac2 = (mc_bs[0].p4.pt() + mc_bs[1].p4.pt()) / sum_jpt;
            fill(bfrag2_dr_mc, drbb, bfrac2);
            fill(bfrag2_nj_dr_mc, drbb, all_matches.size());
        }
        
        // reco plots:
        ID(bfrag_jpt_isob); ID(bfrag_bpt_isob);
        ID(bfrag2_dr); ID(bfrag2_nj_dr);
        ID(bfrag2_dr_mcbb); ID(bfrag2_dr_mcbd);
        ID(drj_drb);
        if(bcands.size() == 2){
            bool bs_isolated = deltaR(bcands[0].flightdir, bcands[1].flightdir) > 0.5;
            auto b0_matches = find_matching_jets(bcands[0], jets);
            auto b1_matches = find_matching_jets(bcands[1], jets);
            if(bs_isolated){
                // fill iso B plots:
                if(b0_matches.size() > 0){
                    double bfrag = bcands[0].p4.pt() / jets[b0_matches[0].index].p4.pt();
                    fill(bfrag_jpt_isob, jets[b0_matches[0].index].p4.pt(), bfrag);
                    fill(bfrag_bpt_isob, bcands[0].p4.pt(), bfrag);
                }
                if(b1_matches.size() > 0){
                    double bfrag = bcands[1].p4.pt() / jets[b1_matches[0].index].p4.pt();
                    fill(bfrag_jpt_isob, jets[b1_matches[0].index].p4.pt(), bfrag);
                    fill(bfrag_bpt_isob, bcands[1].p4.pt(), bfrag);
                }
            }
            double drbb = deltaR(bcands[0].flightdir, bcands[1].flightdir);
            double drjj = 0.1; // = not both can be matched
            if(!b0_matches.empty() && !b1_matches.empty()){
                drjj = deltaR(jets[b0_matches[0].index].p4, jets[b1_matches[0].index].p4); // can be 0.0 in case both match to the same jet
            }
            fill(drj_drb, drbb, drjj);
            // fill bfrag2 plots:
            auto all_matches = merge_matches(b0_matches, b1_matches);
            double sum_jpt = 0.0;
            if(all_matches.size() > 0){
                sum_jpt += jets[all_matches[0].index].p4.pt();
            }
            if(all_matches.size() > 1){
                sum_jpt += jets[all_matches[1].index].p4.pt();
            }
            double bfrac2 = (bcands[0].p4.pt() + bcands[1].p4.pt()) / sum_jpt;
            fill(bfrag2_dr, drbb, bfrac2);
            fill(bfrag2_nj_dr, drbb, all_matches.size());
            // find out truth by trying to match Bcands to B hadrons
            int n = match_bc_mcb(bcands, mc_bs);
            if(n==2){
                fill(bfrag2_dr_mcbb, drbb, bfrac2);
            }
            else if(n==1){
                fill(bfrag2_dr_mcbd, drbb, bfrac2);
            }
            
        }
    }
    
private:
    double current_weight;
    Event::Handle<vector<mcparticle>> h_mc_bs;
    Event::Handle<vector<LorentzVector>> h_mc_jets;
    Event::Handle<double> h_weight;
    Event::Handle<vector<Bcand> > h_selected_bcands;
    Event::Handle<vector<jet> > h_jets;
    //Event::Handle<lepton> h_lepton_plus, h_lepton_minus;
};

REGISTER_HISTS(BFragmentationHists)
