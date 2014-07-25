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
        
//     double get_dr_mcb_bcand(const mcparticle & mcb, const Bcand & bcand) {
//         Vector mcb_flightdir = mcb.p4.Vect();
//         Vector bcand_flightdir = bcand.flightdir;
//         
//         return deltaR(mcb_flightdir, bcand_flightdir);
//     }


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


// count the number of mc_bs with |eta| < 2.4 and pt > 0
int get_reco_bhad(const vector<mcparticle> & mc_bs){
    int count = 0;
    for (auto & mcb : mc_bs){
        if (mcb.p4.eta() < 2.4 && mcb.p4.pt() > 0)
            count++;
    }
    
    return count;
}
    
    
}

class BaseHists: public Hists {
public:
    BaseHists(const ptree &, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){

        book_1d_autofill([=](Event & e){return e.get<lepton>(id::lepton_plus).p4.pt();}, "pt_lp", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<lepton>(id::lepton_minus).p4.pt();}, "pt_lm", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<lepton>(id::lepton_plus).p4.eta();}, "eta_lp", 100, -2.5, 2.5);
        book_1d_autofill([=](Event & e){return e.get<lepton>(id::lepton_minus).p4.eta();}, "eta_lm", 100, -2.5, 2.5);
        
        book_1d_autofill(&maxpt_lepton, "max_pt_lep", 200, 0, 200);
        book_1d_autofill(&max_abs_eta, "max_aeta_lep", 100, 0, 2.5);
        
        book_1d_autofill([=](Event & e){return e.get<LorentzVector>(id::zp4).M();}, "mll", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<LorentzVector>(id::zp4).pt();}, "ptz", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<LorentzVector>(id::zp4).eta();}, "etaz", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<float>(id::met);}, "met", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<vector<Bcand> >(id::selected_bcands).size();}, "nbcands", 10, 0, 10);
        book_1d_autofill([=](Event & e){return e.get<int>(id::mc_n_me_finalstate);}, "mc_n_me_finalstate", 10, 0, 10);
        
        book_1d_autofill([=](Event & e){return e.get<int>(id::npv);}, "npv", 60, 0, 60);
    }
};

REGISTER_HISTS(BaseHists)



class BcandHists: public Hists{
public:
    BcandHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
        mcb_input = ptree_get<string>(cfg, "mcb_input");
        
        book<TH1D>("Bmass", 60, 0, 6);
        book<TH1D>("Bpt", 200, 0, 200);
        book<TH1D>("Beta", 100, -3, 3);
        
        book<TH1D>("svdist3d", 100, 0, 10);
        book<TH1D>("svdist2d", 100, 0, 10);
        book<TH1D>("svdist3dsig", 100, 0, 200);
        book<TH1D>("svdist2dsig", 100, 0, 200);
        book<TH1D>("DR_ZB", 50, 0, 5);
    
        book<TH1D>("DR_BB", 50, 0, 5);
        book<TH1D>("DPhi_BB", 32, 0, 3.2);
        book<TH1D>("m_BB", 100, 0, 200);
        book<TH1D>("m_all", 100, 0, 800);
    
        book<TH1D>("DR_VV", 50, 0, 5);
        book<TH1D>("DPhi_VV", 32, 0, 3.2);
        book<TH1D>("dist_VV", 50, 0, 10);
    
        book<TH1D>("min_DR_ZB", 50, 0, 5);
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
    
    void divide(const identifier & id_new, const identifier & id_num, const identifier & id_den){
        get(id_new)->Divide(get(id_num), get(id_den));
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
        ID(m_all);
    
        ID(DR_VV);
        ID(DPhi_VV);
        ID(dist_VV);
    
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
        
        /*ID(double_mcb_lower_pt);
        ID(double_mcb_higher_pt);
        ID(double_mcb_dPhi);
        ID(double_mcb_dR);
        ID(double_matchedb_lower_pt);
        ID(double_matchedb_higher_pt);
        ID(double_matchedb_dPhi);
        ID(double_matchedb_dR);
        
        ID(dR_mc_reco_diff);*/
        
        current_weight = e.weight();

        auto & bcands = e.get<vector<Bcand> >(id::selected_bcands);
        auto & mc_bhads = e.get<vector<mcparticle> >(mcb_input);
        
        const LorentzVector & p4Z = e.get<LorentzVector>(id::zp4);
        for(auto & b : bcands){
            fill(Bmass, b.p4.M());
            fill(Bpt, b.p4.pt());
            fill(Beta, b.p4.eta());
            
            fill(svdist2d, b.dist2D);
            fill(svdist3d, b.dist3D);
            fill(svdist2dsig, b.distSig2D);
            fill(svdist3dsig, b.distSig3D);
            
            fill(DR_ZB, deltaR(b.p4, p4Z));
        }
        
        if(bcands.size()==2){
            const Bcand & b0 = bcands[0];
            const Bcand & b1 = bcands[1];
            
            fill(DR_BB, deltaR(b0.p4, b1.p4));
            fill(DPhi_BB, deltaPhi(b0.p4, b1.p4));
            fill(m_BB, (b0.p4 + b1.p4).M());
            fill(m_all, (p4Z+b0.p4+b1.p4).M());
            
            fill(DR_VV, deltaR(b0.flightdir, b1.flightdir));
            fill(DPhi_VV, deltaPhi(b0.flightdir, b1.flightdir));
            fill(dist_VV, sqrt((b0.flightdir -  b1.flightdir).Mag2()));
            
            double dr0 = deltaR(p4Z, b0.p4);
            double dr1 = deltaR(p4Z, b1.p4);
            double min_dr = min(dr0, dr1);
            double max_dr = max(dr0, dr1);
            
            fill(DR_ZB, dr0);
            fill(DR_ZB, dr1);
            fill(min_DR_ZB, min_dr);
            fill(A_ZBB, (max_dr - min_dr) / (max_dr + min_dr));
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
