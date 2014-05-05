#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/hists.hpp"

// #include "DataFormats/Math/interface/deltaR.h"

#include "eventids.hpp"

#include <iostream>
#include "TH1D.h"
#include "TH2D.h"
#include "zsvtree.hpp"




using namespace ra;
using namespace std;

namespace{
    
    double maxpt_lepton(const Event & e){
        return max(e.get<lepton>(lepton_plus).p4.pt(), e.get<lepton>(lepton_minus).p4.pt());
    }
    
    double max_abs_eta(const Event & e){
        double eta1 = e.get<lepton>(lepton_plus).p4.eta();
        double eta2 = e.get<lepton>(lepton_plus).p4.eta();
        return max(fabs(eta1), fabs(eta2));
    }
    
//     double deltaPhi(double phi1, double phi2) {
//         double result = phi1 - phi2;
//         while (result > M_PI) result -= 2*M_PI;
//         while (result <= -M_PI) result += 2*M_PI;
//         return result;
//     }
//     
//     double deltaR(double eta1, double phi1, double eta2, double phi2) {
//         double deta = eta1 - eta2;
//         double dphi = deltaPhi(phi1, phi2);
//         return sqrt(deta*deta + dphi*dphi);
//     }
    
//     double get_dr_mcb_bcand(const mcparticle & mcb, const Bcand & bcand) {
//         Vector mcb_flightdir = mcb.p4.Vect();
//         Vector bcand_flightdir = bcand.flightdir;
//         
//         return deltaR(mcb_flightdir, bcand_flightdir);
//     }
    
    int matching_bcands(const mcparticle & mcb, const vector<Bcand> & bcands, float maxdr){
        
//         double result = 0.;
        int found_matches = 0;
        
        for (auto & bcand : bcands){
            if (deltaR(mcb.p4, bcand.flightdir) < maxdr){
                found_matches++;
//                 break;
            }
        }
                
        return found_matches;
    }
    
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

        book_1d_autofill([=](Event & e){return e.get<lepton>(lepton_plus).p4.pt();}, "pt_lp", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<lepton>(lepton_minus).p4.pt();}, "pt_lm", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<lepton>(lepton_plus).p4.eta();}, "eta_lp", 100, -2.5, 2.5);
        book_1d_autofill([=](Event & e){return e.get<lepton>(lepton_minus).p4.eta();}, "eta_lm", 100, -2.5, 2.5);
        
        book_1d_autofill(&maxpt_lepton, "max_pt_lep", 200, 0, 200);
        book_1d_autofill(&max_abs_eta, "max_aeta_lep", 100, 0, 2.5);
        
        book_1d_autofill([=](Event & e){return e.get<LorentzVector>(zp4).M();}, "mll", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<LorentzVector>(zp4).pt();}, "ptz", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<LorentzVector>(zp4).eta();}, "etaz", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<float>(met);}, "met", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<vector<Bcand> >(selected_bcands).size();}, "nbcands", 10, 0, 10);
        book_1d_autofill([=](Event & e){return e.get<int>(mc_n_me_finalstate);}, "mc_n_me_finalstate", 10, 0, 10);
        
        book_1d_autofill([=](Event & e){return e.get<int>(npv);}, "npv", 60, 0, 60);
    }
};

REGISTER_HISTS(BaseHists)

class BcandHists: public Hists{
public:
    BcandHists(const ptree &, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
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
    
        book<TH1D>("DR_VV", 50, 0, 5);
        book<TH1D>("DPhi_VV", 32, 0, 3.2);
        book<TH1D>("dist_VV", 50, 0, 10);
    
        book<TH1D>("min_DR_ZB", 50, 0, 5);
        book<TH1D>("A_ZBB", 50, 0, 1);
	
	// b efficiency plot
// 	book<TH2D>("sin_b_eff_pt", 20, 0, 200, 20, 0, 1);
        book<TH1D>("mcbs_pt", 40, 0, 200);
        book<TH1D>("matched_bcands_pt", 40, 0, 200);
        book<TH1D>("found_bcand_matches", 10, 0, 10);
//         book<TH1D>("sing_b_eff_pt", 40, 0, 200);
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
    
        ID(DR_VV);
        ID(DPhi_VV);
        ID(dist_VV);
    
        ID(min_DR_ZB);
        ID(A_ZBB);
	
	ID(sin_b_eff_pt);
        ID(mcbs_pt);
        ID(matched_bcands_pt);
        ID(found_bcand_matches);
//         ID(sing_b_eff_pt);
        
        current_weight = e.weight();

        auto & bcands = e.get<vector<Bcand> >(selected_bcands);
        auto & mc_bhads = e.get<vector<mcparticle> >(mc_bs);        
        
        const LorentzVector & p4Z = e.get<LorentzVector>(zp4);
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
        
//         int mcb_count = 0;
//         int matched_bcand_count = 0;
//         double mcb_maxpt = 0.;
        
        for (auto & mc_b : mc_bhads){
            if (mc_b.p4.eta() < 2.4 && mc_b.p4.pt() > 0){
                fill(mcbs_pt, mc_b.p4.pt());
                fill(found_bcand_matches, matching_bcands(mc_b, bcands, 0.10));
//                 mcb_count++;
//                 if (mc_b.p4.pt() > mcb_maxpt) mcb_maxpt = mc_b.p4.pt();
                if (matching_bcands(mc_b, bcands, 0.10) >= 1){
//                     matched_bcand_count++;
                    fill(matched_bcands_pt, mc_b.p4.pt());
                }
            }
        }
        
//         divide(sing_b_eff_pt,matched_bcands_pt, mcbs_pt);
        
//         if (mcb_count > 0){
//             fill(sin_b_eff_pt, mcb_maxpt, (double)matched_bcand_count/mcb_count);
//         }
        
    }
    
private:
    double current_weight;

};

REGISTER_HISTS(BcandHists)
