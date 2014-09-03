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

namespace{
    
// get the maximum pt of the two leptons lepton_plus and lepton_minus
double maxpt_lepton(const Event & e){
    return max(e.get<lepton>(id::lepton_plus).p4.pt(), e.get<lepton>(id::lepton_minus).p4.pt());
}

// return the maximum |eta| of the two leptons
double max_abs_eta(const Event & e){
    double eta1 = e.get<lepton>(id::lepton_plus).p4.eta();
    double eta2 = e.get<lepton>(id::lepton_minus).p4.eta();
    return max(fabs(eta1), fabs(eta2));
}

// given a generator-level b mcb, find the matching reconstructed B candidates from bcands. Matching is done via
// delta R < maxdr. In case of multiple matches, they are sorted by increasing distance (i.e. the closest is at index 0).
//
// B candidates with indices in already_matched_bcands are excluded from matching; matches found are added to already_matches_bcands.
// This allows to call matching_bcands with the same already_matched_bcands repeatedly, which then avoids double matches.
vector<const Bcand*> matching_bcands(const mcparticle & mcb, const vector<Bcand> & bcands, float maxdr, set<unsigned int> & already_matched_bcands){
    map<double, const Bcand*> matches;
    unsigned int i = 0;
    for (const auto & bcand : bcands){
        double dr = deltaR(mcb.p4, bcand.flightdir);
        if (dr < maxdr){
            if (already_matched_bcands.find(i) != already_matched_bcands.end()) continue;
            matches.insert(make_pair(dr, &bcand));
            already_matched_bcands.insert(i);
        }
        ++i;
    }
    // convert to vector; iterating over the map yields the desired order.
    vector<const Bcand*> result;
    result.reserve(matches.size());
    for(const auto & m : matches){
        result.push_back(m.second);
    }
    return result;
}    
    
}

class BaseHists: public Hists {
public:
    BaseHists(const ptree &, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){

        book_1d_autofill([=](Event & e){return e.get<lepton>(id::lepton_plus).p4.pt();}, "pt_lp", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<lepton>(id::lepton_minus).p4.pt();}, "pt_lm", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<lepton>(id::lepton_plus).p4.eta();}, "eta_lp", 100, -2.5, 2.5);
        book_1d_autofill([=](Event & e){return e.get<lepton>(id::lepton_minus).p4.eta();}, "eta_lm", 100, -2.5, 2.5);
        
        //book_1d_autofill(&maxpt_lepton, "max_pt_lep", 200, 0, 200);
        //book_1d_autofill(&max_abs_eta, "max_aeta_lep", 100, 0, 2.5);
        
        book_1d_autofill([=](Event & e){return e.get<LorentzVector>(id::zp4).M();}, "mll", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<LorentzVector>(id::zp4).pt();}, "ptz", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<LorentzVector>(id::zp4).eta();}, "etaz", 200, -5, 5);
        book_1d_autofill([=](Event & e){return e.get<float>(id::met);}, "met", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<vector<Bcand> >(id::selected_bcands).size();}, "nbcands", 10, 0, 10);
        book_1d_autofill([=](Event & e){return e.get<int>(id::mc_n_me_finalstate);}, "mc_n_me_finalstate", 10, 0, 10);
        book_1d_autofill([=](Event & e){return e.get<int>(id::npv);}, "npv", 60, 0, 60);
        
        ID(one);
        book_1d_autofill([=](Event & e){return e.get_default<float>(id::pileupsf, NAN);}, "pileupsf", 100, 0, 4, one);
        book_1d_autofill([=](Event & e){return e.get_default<float>(id::elesf, NAN);}, "elesf", 100, 0, 4, one);
        book_1d_autofill([=](Event & e){return e.get_default<float>(id::musf, NAN);}, "musf", 100, 0, 4, one);
    }
    
    virtual void process(Event & event){
        ID(one);
        event.set<double>(one, 1.0);
    }
};

REGISTER_HISTS(BaseHists)

class JetHists: public Hists {
public:
    JetHists(const ptree &, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
        // jets:
        book<TH1D>("nj", 10, 0, 10); // pt > 20, |\eta| < 2.4
        book<TH1D>("ptj0", 60, 0, 180);
        book<TH1D>("ptj1", 60, 0, 180);
        book<TH1D>("etaj0", 60, -3, 3);
        book<TH1D>("etaj1", 60, -3, 3);
        
        // does the leading jet match to a mc B with pt10/15/20 DR<0.2/0.3?
        book<TH1D>("ptj0_mcb_r2", 60, 0, 180); // only filled if eta(j) < 1.8
        book<TH1D>("ptj0_mcb_r3", 60, 0, 180);
        book<TH1D>("etaj0_mcb_r2", 60, -3, 3);
        book<TH1D>("etaj0_mcb_r3", 60, -3, 3);
        
        // does the second leading jet match to a mc B?
        book<TH1D>("ptj1_mcb_r2", 60, 0, 180); // only filled if with eta(j) < 1.8
        book<TH1D>("ptj1_mcb_r3", 60, 0, 180);
        book<TH1D>("etaj1_mcb_r2", 60, -3, 3); // ptj > 20
        book<TH1D>("etaj1_mcb_r3", 60, -3, 3);
        
        // actually found reco b:
        book<TH1D>("ptj0_bc_r2", 60, 0, 180);
        book<TH1D>("ptj0_bc_r3", 60, 0, 180);
        book<TH1D>("etaj0_bc_r2", 60, -3, 3);
        book<TH1D>("etaj0_bc_r3", 60, -3, 3);
        
        book<TH1D>("ptj1_bc_r2", 60, 0, 180);
        book<TH1D>("ptj1_bc_r3", 60, 0, 180);
        book<TH1D>("etaj1_bc_r2", 60, -3, 3);
        book<TH1D>("etaj1_bc_r3", 60, -3, 3);
        
        
        // tag + probe: in >= 2jet events, ask 1 jet to be b-tagged (dr < 0.2, 'tight')
        // If it is, look at the other jet.
        // Record the overall 'probe' jets and the tagged ones of those:
        book<TH1D>("ptjp", 60, 0, 180); // all probes
        book<TH1D>("etajp", 60, -3, 3);
        book<TH1D>("ptjp_bc_r2", 60, 0, 180); // passing probes with 0.2
        book<TH1D>("etajp_bc_r2", 60, -3, 3);
        book<TH1D>("ptjp_bc_r3", 60, 0, 180);
        book<TH1D>("etajp_bc_r3", 60, -3, 3);
        
        // control plot: how many probes are actually b?
        book<TH1D>("ptjp_mcb_r2", 60, 0, 180);
        book<TH1D>("etajp_mcb_r2", 60, -3, 3);
        book<TH1D>("ptjp_mcb_r3", 60, 0, 180);
        book<TH1D>("etajp_mcb_r3", 60, -3, 3);
    }
    
    void fill(const identifier & id, double value){
        get(id)->Fill(value, current_weight);
    }
    
    virtual void process(Event & e);
    
private:
    double current_weight;
};

void JetHists::process(Event & e){
    current_weight = e.get<double>(fwid::weight);
    auto jets = e.get<vector<jet>>(id::jets);
    const auto & lepton_plus = e.get<lepton>(id::lepton_plus);
    const auto & lepton_minus = e.get<lepton>(id::lepton_minus);
    sort(jets.begin(), jets.end(), [](const jet & j1, const jet & j2){return j1.p4.pt () > j2.p4.pt();});
    for(size_t i=0; i<jets.size(); ++i){
        if(fabs(jets[i].p4.eta()) > 2.4 || deltaR(jets[i].p4, lepton_plus.p4) < 0.5 || deltaR(jets[i].p4, lepton_minus.p4) < 0.5){
            jets.erase(jets.begin() + i);
            --i;
            continue;
        }
        if(jets[i].p4.pt() < 20.0){
            jets.erase(jets.begin() + i, jets.end());
            break;
        }
    }
    ID(nj);
    size_t njets = jets.size();
    fill(nj, njets);
    if(njets==0) return;
    
    ID(ptj0); ID(etaj0);
    ID(ptj1); ID(etaj1);
    if(jets.size() >= 1){
        fill(ptj0, jets[0].p4.pt());
        fill(etaj0, jets[0].p4.eta());
    }
    if(jets.size() >= 2){
        fill(ptj1, jets[1].p4.pt());
        fill(etaj1, jets[1].p4.eta());
    }
    
    

    // match to mc bs:
    double dr_mcb_j0 = numeric_limits<double>::infinity();
    double dr_mcb_j1 = dr_mcb_j0;
    auto & mc_bhads = e.get<vector<mcparticle> >(id::mc_bs);
    for(const auto & mcb: mc_bhads){
        dr_mcb_j0 = min(dr_mcb_j0, deltaR(mcb.p4, jets[0].p4));
        if(njets > 1){
            dr_mcb_j1 = min(dr_mcb_j1, deltaR(mcb.p4, jets[1].p4));
        }
    }
    ID(ptj0_mcb_r2); ID(ptj0_mcb_r3);ID(etaj0_mcb_r2); ID(etaj0_mcb_r3);
    if(dr_mcb_j0 < 0.2){
        fill(ptj0_mcb_r2, jets[0].p4.pt());
        fill(etaj0_mcb_r2, jets[0].p4.eta());
    }
    if(dr_mcb_j0 < 0.3){
        fill(ptj0_mcb_r3, jets[0].p4.pt());
        fill(etaj0_mcb_r3, jets[0].p4.eta());
    }
    if(jets.size() > 1){
        ID(ptj1_mcb_r2); ID(ptj1_mcb_r3);ID(etaj1_mcb_r2); ID(etaj1_mcb_r3);
        if(dr_mcb_j1 < 0.2){
            fill(ptj1_mcb_r2, jets[1].p4.pt());
            fill(etaj1_mcb_r2, jets[1].p4.eta());
        }
        if(dr_mcb_j1 < 0.3){
            fill(ptj1_mcb_r3, jets[1].p4.pt());
            fill(etaj1_mcb_r3, jets[1].p4.eta());
        }
    }
    
    // match to reco bs:
    double dr_bc_j0 = numeric_limits<double>::infinity();
    double dr_bc_j1 = dr_bc_j0;
    auto & bcands = e.get<vector<Bcand> >(id::selected_bcands);
    for(const auto & bc: bcands){
        dr_bc_j0 = min(dr_bc_j0, deltaR(bc.p4, jets[0].p4));
        if(njets > 1){
            dr_bc_j1 = min(dr_bc_j1, deltaR(bc.p4, jets[1].p4));
        }
    }
    ID(ptj0_bc_r2); ID(ptj0_bc_r3);ID(etaj0_bc_r2); ID(etaj0_bc_r3);
    if(dr_bc_j0 < 0.2){
        fill(ptj0_bc_r2, jets[0].p4.pt());
        fill(etaj0_bc_r2, jets[0].p4.eta());
    }
    if(dr_bc_j0 < 0.3){
        fill(ptj0_bc_r3, jets[0].p4.pt());
        fill(etaj0_bc_r3, jets[0].p4.eta());
    }
    if(jets.size() > 1){
        ID(ptj1_bc_r2); ID(ptj1_bc_r3);ID(etaj1_bc_r2); ID(etaj1_bc_r3);
        if(dr_bc_j1 < 0.2){
            fill(ptj1_bc_r2, jets[1].p4.pt());
            fill(etaj1_bc_r2, jets[1].p4.eta());
        }
        if(dr_bc_j1 < 0.3){
            fill(ptj1_bc_r3, jets[1].p4.pt());
            fill(etaj1_bc_r3, jets[1].p4.eta());
        }
    }
    
    // tag and probe:
    if(njets >= 2){
        ID(ptjp); ID(etajp);
        ID(ptjp_bc_r2); ID(etajp_bc_r2);ID(ptjp_bc_r3); ID(etajp_bc_r3);
        ID(ptjp_mcb_r2); ID(etajp_mcb_r2); ID(ptjp_mcb_r3); ID(etajp_mcb_r3);
        // j0 is tag?
        if(dr_bc_j0 < 0.2){
            // j1 is probe:
            fill(ptjp, jets[1].p4.pt());
            fill(etajp, jets[1].p4.eta());
            if(dr_bc_j1 < 0.2){
                fill(ptjp_bc_r2, jets[1].p4.pt());
                fill(etajp_bc_r2, jets[1].p4.eta());
            }
            if(dr_bc_j1 < 0.3){
                fill(ptjp_bc_r3, jets[1].p4.pt());
                fill(etajp_bc_r3, jets[1].p4.eta());
            }
            if(dr_mcb_j1 < 0.2){
                fill(ptjp_mcb_r2, jets[1].p4.pt());
                fill(etajp_mcb_r2, jets[1].p4.eta());
            }
            if(dr_mcb_j1 < 0.3){
                fill(ptjp_mcb_r3, jets[1].p4.pt());
                fill(etajp_mcb_r3, jets[1].p4.eta());
            }
        }
        // j1 is tag?
        if(dr_bc_j1 < 0.2){
            // j0 is probe:
            fill(ptjp, jets[0].p4.pt());
            fill(etajp, jets[0].p4.eta());
            if(dr_bc_j0 < 0.2){
                fill(ptjp_bc_r2, jets[0].p4.pt());
                fill(etajp_bc_r2, jets[0].p4.eta());
            }
            if(dr_bc_j0 < 0.3){
                fill(ptjp_bc_r3, jets[0].p4.pt());
                fill(etajp_bc_r3, jets[0].p4.eta());
            }
            if(dr_mcb_j0 < 0.2){
                fill(ptjp_mcb_r2, jets[0].p4.pt());
                fill(etajp_mcb_r2, jets[0].p4.eta());
            }
            if(dr_mcb_j0 < 0.3){
                fill(ptjp_mcb_r3, jets[0].p4.pt());
                fill(etajp_mcb_r3, jets[0].p4.eta());
            }
        }
    }
}


REGISTER_HISTS(JetHists)


class BcandHists: public Hists{
public:
    BcandHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
        mcb_input = ptree_get<string>(cfg, "mcb_input", "mc_bs");
        
        book<TH1D>("Bmass", 60, 0, 6);
        book<TH1D>("Bpt", 200, 0, 200);
        book<TH1D>("Beta", 100, -3, 3);
        book<TH1D>("Bntracks", 20, 0, 20);
        book<TH1D>("Bnsv", 5, 0, 5);
        
        book<TH1D>("svdist3d", 100, 0, 10);
        book<TH1D>("svdist2d", 100, 0, 10);
        book<TH1D>("svdist3dsig", 100, 0, 200);
        book<TH1D>("svdist2dsig", 100, 0, 200);
        book<TH1D>("DR_ZB", 50, 0, 5);
    
        book<TH1D>("DR_BB", 100, 0, 5);
        book<TH1D>("DPhi_BB", 64, 0, 3.2);
        book<TH1D>("m_BB", 100, 0, 200);
    
        book<TH1D>("min_DR_ZB", 100, 0, 5);
        book<TH1D>("A_ZBB", 50, 0, 1);

        // b efficiency:
        book<TH1D>("number_mcbs", 10, 0, 10);
        
        book<TH1D>("mcbs_pt", 200, 0, 200);
        book<TH1D>("mcbs_eta", 120, -3, 3);
        
        book<TH1D>("matched_mcb_pt", 200, 0, 200);
        book<TH1D>("matched_mcb_eta", 120, -3, 3);
        book<TH1D>("mcb_mass", 80, 0, 8);
        book<TH1D>("found_bcand_matches", 10, 0, 10);
        book<TH1D>("mcb_bcand_pt_ratio", 50, 0, 1);
        
        book<TH1D>("mcb_bcand_dr", 100, 0, 0.2);
        book<TH1D>("mcb_bcand_dphi", 100, 0, 0.2);
        book<TH1D>("mcb_bcand_angle", 100, 0, 0.2);
        book<TH1D>("mcb_bcand_deta", 100, 0, 0.2);
        
        // g->bb study for ttbar: look how often we have more than one b-pair
        // as a function of DR(B,B). x-axis = DRBB, y-axis = number of mcb / 2
        book<TH2D>("Nmcbpair_vs_DRBB", 100, 0, 5, 4, 0, 4);
        
        // double b efficiency:
        /*book<TH1D>("double_mcb_lower_pt", 200, 0, 200);
        book<TH1D>("double_mcb_higher_pt", 200, 0, 200);
        book<TH1D>("double_mcb_dPhi", 32, 0, 3.2);
        book<TH1D>("double_mcb_dR", 50, 0, 5);
        book<TH1D>("double_matchedb_lower_pt", 200, 0, 200);
        book<TH1D>("double_matchedb_higher_pt", 200, 0, 200);
        book<TH1D>("double_matchedb_dPhi", 32, 0, 3.2);
        book<TH1D>("double_matchedb_dR", 50, 0, 5);
        
        book<TH1D>("dR_mc_reco_diff", 50, -0.5, 0.5);*/
    }
    
    void fill(const identifier & id, double value){
        get(id)->Fill(value, current_weight);
    }
    
    void fill(const identifier & id, double xvalue, double yvalue){
        ((TH2*)get(id))->Fill(xvalue, yvalue, current_weight);
    }
    
    virtual void process(Event & e){
        ID(Bmass);
        ID(Bpt);
        ID(Beta);
        
        ID(svdist3d);
        ID(svdist2d);
        ID(svdist3dsig);
        ID(svdist2dsig);
        ID(DR_ZB);
    
        ID(DR_BB);
        ID(DPhi_BB);
        ID(m_BB);
    
        ID(min_DR_ZB);
        ID(A_ZBB);
        
        ID(mcbs_pt);
        ID(mcbs_eta);
        ID(mcb_mass);
        
        ID(matched_mcb_pt);
        ID(matched_mcb_eta);
        ID(found_bcand_matches);
        ID(mcb_bcand_pt_ratio);
        ID(mcb_bcand_dr);
        ID(mcb_bcand_dphi);
        ID(mcb_bcand_deta);
        ID(mcb_bcand_angle);
        ID(number_mcbs);
        ID(Bntracks);
        ID(Bnsv);
        
        /*ID(double_mcb_lower_pt);
        ID(double_mcb_higher_pt);
        ID(double_mcb_dPhi);
        ID(double_mcb_dR);
        ID(double_matchedb_lower_pt);
        ID(double_matchedb_higher_pt);
        ID(double_matchedb_dPhi);
        ID(double_matchedb_dR);
        
        ID(dR_mc_reco_diff);*/
        
        current_weight = e.get<double>(fwid::weight);

        auto & bcands = e.get<vector<Bcand> >(id::selected_bcands);
        auto & mc_bhads = e.get<vector<mcparticle> >(mcb_input);
        
        const LorentzVector & p4Z = e.get<LorentzVector>(id::zp4);
        for(auto & b : bcands){
            fill(Bmass, b.p4.M());
            fill(Bpt, b.p4.pt());
            fill(Beta, b.flightdir.eta());
            fill(Bntracks, b.ntracks);
            fill(Bnsv, b.nv);
            
            fill(svdist2d, b.dist2D);
            fill(svdist3d, b.dist3D);
            fill(svdist2dsig, b.distSig2D);
            fill(svdist3dsig, b.distSig3D);
            
            fill(DR_ZB, deltaR(b.flightdir, p4Z));
        }
        
        if(bcands.size()==2){
            const Bcand & b0 = bcands[0];
            const Bcand & b1 = bcands[1];
            
            auto drbb = deltaR(b0.flightdir, b1.flightdir);
            
            fill(DR_BB, drbb);
            fill(DPhi_BB, deltaPhi(b0.flightdir, b1.flightdir));
            fill(m_BB, (b0.p4 + b1.p4).M());
            
            double dr0 = deltaR(p4Z, b0.p4);
            double dr1 = deltaR(p4Z, b1.p4);
            double min_dr = min(dr0, dr1);
            double max_dr = max(dr0, dr1);
            
            fill(DR_ZB, dr0);
            fill(DR_ZB, dr1);
            fill(min_DR_ZB, min_dr);
            fill(A_ZBB, (max_dr - min_dr) / (max_dr + min_dr));
            ID(Nmcbpair_vs_DRBB);
            fill(Nmcbpair_vs_DRBB, drbb, mc_bhads.size() / 2);
        }

        fill(number_mcbs, mc_bhads.size());
        
        set<unsigned int> already_matched_bcands;
        for (auto & mc_b : mc_bhads){
            fill(mcbs_pt, mc_b.p4.pt());
            fill(mcbs_eta, mc_b.p4.eta());
            fill(mcb_mass, mc_b.p4.M());
            vector<const Bcand*> found_matches = matching_bcands(mc_b, bcands, 0.10, already_matched_bcands);
            fill(found_bcand_matches, found_matches.size());
            // NOTE: if found matches is > 1, it means that there is an additional
            // B cand fake, so we count this situation still as efficiency. In that case, however,
            // it's not so clear which match to use. We use the closest match here. This should
            // not happen often anyway.
            if (found_matches.size() >= 1){
                fill(matched_mcb_pt, mc_b.p4.pt());
                fill(matched_mcb_eta, mc_b.p4.eta());
                fill(mcb_bcand_pt_ratio, found_matches[0]->p4.pt()/mc_b.p4.pt());
                fill(mcb_bcand_dr, deltaR(mc_b.p4, found_matches[0]->flightdir));
                fill(mcb_bcand_dphi, deltaPhi(mc_b.p4, found_matches[0]->flightdir));
                fill(mcb_bcand_angle, angle(mc_b.p4, found_matches[0]->flightdir));
                fill(mcb_bcand_deta, fabs(mc_b.p4.eta() - found_matches[0]->flightdir.eta()));
            }
        }
        
        /*if (mc_bhads.size() == 2){
            if (fabs(mc_bhads[0].p4.eta()) < 2.4 && mc_bhads[0].p4.pt() > mcb_minpt && fabs(mc_bhads[1].p4.eta()) < 2.4 && mc_bhads[1].p4.pt() > mcb_minpt) {
                const mcparticle & Mc_b0 = mc_bhads[0];
                const mcparticle & Mc_b1 = mc_bhads[1];
                
                double lower_pt, higher_pt;
                
                if (Mc_b0.p4.pt() >= Mc_b1.p4.pt()) {
                    lower_pt = Mc_b1.p4.pt();
                    higher_pt = Mc_b0.p4.pt();
                } else {
                    lower_pt = Mc_b0.p4.pt();
                    higher_pt = Mc_b1.p4.pt();
                }
                
                double dPhi_mcb, dR_mcb, dR_matchedbcand, dR_diff;
                
                dPhi_mcb = deltaPhi(Mc_b0.p4, Mc_b1.p4);
                dR_mcb = deltaR(Mc_b0.p4, Mc_b1.p4);
                
                fill(double_mcb_lower_pt, lower_pt);
                fill(double_mcb_higher_pt, higher_pt);
                fill(double_mcb_dPhi, dPhi_mcb);
                fill(double_mcb_dR, dR_mcb);
                
                std::set<unsigned int> already_matched_bcands;
                
                std::vector<const Bcand*> found_matches0 = matching_bcands(Mc_b0, bcands, 0.10, already_matched_bcands);
                std::vector<const Bcand*> found_matches1 = matching_bcands(Mc_b1, bcands, 0.10, already_matched_bcands);
                
                if (found_matches0.size() == 1 && found_matches1.size() == 1) {
                    dR_matchedbcand = deltaR(found_matches0[0]->p4, found_matches1[0]->p4);
                    dR_diff = dR_matchedbcand-dR_mcb;
                    
                    fill(dR_mc_reco_diff, dR_diff);
                    fill(double_matchedb_lower_pt, lower_pt);
                    fill(double_matchedb_higher_pt, higher_pt);
                    fill(double_matchedb_dPhi, dPhi_mcb);
                    fill(double_matchedb_dR, dR_mcb);
                }
            }
        }*/
        
        
    }
    
private:
    double current_weight;
    identifier mcb_input;
};

REGISTER_HISTS(BcandHists)
