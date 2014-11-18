#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/hists.hpp"

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <set>
#include "TH1D.h"
#include "TH2D.h"
#include "zsvtree.hpp"

using namespace ra;
using namespace std;

namespace{

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
    BaseHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, InputManager & in, OutputManager & out): Hists(dirname, dataset, out){
        auto h_lepton_plus = in.get_handle<lepton>("lepton_plus");
        auto h_lepton_minus = in.get_handle<lepton>("lepton_minus");
        h_zp4 = in.get_handle<LorentzVector>("zp4");
        auto h_met = in.get_handle<float>("met");
        auto h_npv = in.get_handle<int>("npv");
        auto h_mc_n_me_finalstate = in.get_handle<int>("mc_n_me_finalstate");
        auto h_selected_bcands = in.get_handle<vector<Bcand>>("selected_bcands");
        
        auto h_pileupsf = in.get_handle<double>("pileupsf");
        auto h_elesf = in.get_handle<double>("elesf");
        auto h_musf = in.get_handle<double>("musf");
        
        h_one = in.get_handle<double>("one");
        h_weight = in.get_handle<double>("weight");
        h_mc_partons = in.get_handle<vector<mcparticle>>("mc_partons");
        
        book_1d_autofill([=](Event & e){const auto & lep = e.get(h_lepton_plus); return abs(lep.pdgid)==13 ? lep.p4.pt() : NAN; }, "pt_mup", 100, 0, 200);
        book_1d_autofill([=](Event & e){const auto & lep = e.get(h_lepton_minus); return abs(lep.pdgid)==13 ? lep.p4.pt() : NAN; }, "pt_mum", 100, 0, 200);
        book_1d_autofill([=](Event & e){const auto & lep = e.get(h_lepton_plus); return abs(lep.pdgid)==11 ? lep.p4.pt() : NAN; }, "pt_elep", 100, 0, 200);
        book_1d_autofill([=](Event & e){const auto & lep = e.get(h_lepton_minus); return abs(lep.pdgid)==11 ? lep.p4.pt() : NAN; }, "pt_elem", 100, 0, 200);
        
        book_1d_autofill([=](Event & e){const auto & lep = e.get(h_lepton_plus); return abs(lep.pdgid)==13 ? lep.p4.eta() : NAN;}, "eta_mup", 60, -3, 3);
        book_1d_autofill([=](Event & e){const auto & lep = e.get(h_lepton_minus); return abs(lep.pdgid)==13 ? lep.p4.eta() : NAN;}, "eta_mum", 60, -3, 3);
        book_1d_autofill([=](Event & e){const auto & lep = e.get(h_lepton_plus); return abs(lep.pdgid)==11 ? lep.p4.eta() : NAN;}, "eta_elep", 60, -3, 3);
        book_1d_autofill([=](Event & e){const auto & lep = e.get(h_lepton_minus); return abs(lep.pdgid)==11 ? lep.p4.eta() : NAN;}, "eta_elem", 60, -3, 3);
        
        book_1d_autofill([=](Event & e){return e.get<LorentzVector>(h_zp4).pt();}, "ptz", 100, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<LorentzVector>(h_zp4).eta();}, "etaz", 200, -5, 5);
        book_1d_autofill([=](Event & e){return e.get<float>(h_met);}, "met", 200, 0, 200);
        book_1d_autofill([=](Event & e){return e.get<vector<Bcand> >(h_selected_bcands).size();}, "nbcands", 10, 0, 10);
        book_1d_autofill([=](Event & e){return e.get<int>(h_mc_n_me_finalstate);}, "mc_n_me_finalstate", 10, 0, 10);
        book_1d_autofill([=](Event & e){return e.get<int>(h_npv);}, "npv", 60, 0, 60);
        
        book_1d_autofill([=](Event & e){return e.get_default<double>(h_pileupsf, NAN);}, "pileupsf", 100, 0, 4, h_one);
        book_1d_autofill([=](Event & e){return e.get_default<double>(h_elesf, NAN);}, "elesf", 100, 0, 4, h_one);
        book_1d_autofill([=](Event & e){return e.get_default<double>(h_musf, NAN);}, "musf", 100, 0, 4, h_one);
        
        // mll histos + systematics:
        book<TH1D>("mll", 200, 0, 200);
        string systs = ptree_get<string>(cfg, "systs", "");
        vector<string> vsysts;
        boost::split(vsysts, systs, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
        for(const auto s : vsysts){
            if(s.empty()) continue;
            h_systs.emplace_back(in.get_handle<double>(s));
            syst_mll_histos.emplace_back("mll__" + s);
            book<TH1D>(syst_mll_histos.back(), 200, 0, 200);
        }
        
        h_ttdec = in.get_handle<int>("ttdec");
        book<TH1D>("ttdec", 10, 0, 10);
    }
    
    virtual void process(Event & event){
        event.set(h_one, 1.0);
        double m_ll = event.get(h_zp4).M();
        auto weight = event.get(h_weight);
        ID(mll);
        get(mll)->Fill(m_ll, weight);
        for(size_t i=0; i<h_systs.size(); ++i){
            // fill histograms with modified weight by factor 1 + syst, as filled by the resp. modules.
            double weight_syst = event.get_default(h_systs[i], 0.0);
            get(syst_mll_histos[i])->Fill(m_ll, weight * (1.0 + weight_syst));
        }
        ID(ttdec);
        auto ttdec_value = event.get(h_ttdec);
        get(ttdec)->Fill(ttdec_value);
    }
    
private:
    Event::Handle<double> h_one, h_weight;
    Event::Handle<LorentzVector> h_zp4;
    Event::Handle<int> h_ttdec;
    Event::Handle<vector<mcparticle>> h_mc_partons;
    std::vector<Event::Handle<double>> h_systs;
    std::vector<identifier> syst_mll_histos;
};

REGISTER_HISTS(BaseHists)

class BcandHists: public Hists{
public:
    BcandHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, InputManager & in, OutputManager & out): Hists(dirname, dataset, out){
        string mcb_input = ptree_get<string>(cfg, "mcb_input", "mc_bs");
        h_mcb_input = in.get_handle<vector<mcparticle>>(mcb_input);
        h_weight = in.get_handle<double>("weight");
        string bcands_input = ptree_get<string>(cfg, "bcands_input", "selected_bcands");
        h_selected_bcands = in.get_handle<vector<Bcand>>(bcands_input);
        h_zp4 = in.get_handle<LorentzVector>("zp4");
        h_lepton_plus = in.get_handle<lepton>("lepton_plus");
        h_lepton_minus = in.get_handle<lepton>("lepton_minus");
        h_jets = in.get_handle<vector<jet>>("jets");
        
        book<TH1D>("Bmass", 60, 0, 6);
        book<TH1D>("Bpt", 200, 0, 200);
        book<TH1D>("Beta", 100, -3, 3);
        book<TH1D>("Bntracks", 20, 0, 20);
        book<TH1D>("Bnsv", 5, 0, 5);
        
        book<TH1D>("Bpt_over_jetpt", 150, 0, 1.5);
        
        book<TH1D>("svdist3d", 100, 0, 10);
        book<TH1D>("svdist2d", 100, 0, 10);
        book<TH1D>("svdist3dsig", 100, 0, 200);
        book<TH1D>("svdist2dsig", 100, 0, 200);
        book<TH1D>("DR_ZB", 50, 0, 5);
    
        book<TH1D>("DR_BB", 200, 0, 5);
        book<TH1D>("DPhi_BB", 128, 0, 3.2);
        book<TH1D>("m_BB", 100, 0, 200);
    
        book<TH1D>("min_DR_ZB", 200, 0, 5);
        book<TH1D>("min_DR_Blep", 200, 0, 5);
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
        
        // cross-check of pt(B) modeling: plot y = pt(B) versus x = DR(B,B) (and x=DPhi(B,B))
        // to make sure that this is modeled correctly (if not, it means that the efficiency
        // in the DR / DPhi bins is off, as the efficiency depends on pt(B)).
        // Use pt of bcand0 (leading in pt) and bcand1 (subleading in pt).
        book<TH2D>("bcand0pt_drbb", 50, 0, 5, 100, 0, 100);
        book<TH2D>("bcand1pt_drbb", 50, 0, 5, 100, 0, 100);
        book<TH2D>("bcand0pt_dphibb", 32, 0, 3.2, 100, 0, 64);
        book<TH2D>("bcand1pt_dphibb", 32, 0, 3.2, 100, 0, 64);
        
        // same for MC bs:
        book<TH2D>("mcb0pt_drbb", 50, 0, 5, 100, 0, 100);
        book<TH2D>("mcb1pt_drbb", 50, 0, 5, 100, 0, 100);
        book<TH2D>("mcb0pt_dphibb", 32, 0, 3.2, 100, 0, 64);
        book<TH2D>("mcb1pt_dphibb", 32, 0, 3.2, 100, 0, 64);
        
        // y = mll versus x = DR(B,B) or DPhi(B,B). Can be used to estimate
        // purity of Z (?!)
        book<TH2D>("mll_drbb", 50, 0, 5, 120, 60, 180);
        book<TH2D>("mll_dphibb", 32, 0, 3.2, 120, 60, 180);
        
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
        
        current_weight = e.get<double>(h_weight);

        auto & bcands = e.get<vector<Bcand> >(h_selected_bcands);
        auto & mc_bhads = e.get<vector<mcparticle> >(h_mcb_input);
        auto & jets = e.get(h_jets);
        
        const LorentzVector & p4Z = e.get<LorentzVector>(h_zp4);
        for(auto & b : bcands){
            fill(Bmass, b.p4.M());
            fill(Bpt, b.p4.pt());
            fill(Beta, b.flightdir.eta());
            fill(Bntracks, b.ntracks);
            fill(Bnsv, b.nv);
            
            double jetpt = std::numeric_limits<double>::infinity();
            double drmin = jetpt;
            for(auto & jet : jets){
                double dr = deltaR(jet.p4, b.flightdir);
                if(dr < 0.5 && dr < drmin){
                    drmin = dr;
                    jetpt = jet.p4.pt();
                }
            }
            ID(Bpt_over_jetpt);
            fill(Bpt_over_jetpt, b.p4.pt() / jetpt); // note: if no jet matches, jetpt = infinity -> entry at 0.
            
            fill(svdist2d, b.dist2D);
            fill(svdist3d, b.dist3D);
            fill(svdist2dsig, b.distSig2D);
            fill(svdist3dsig, b.distSig3D);
            
            fill(DR_ZB, deltaR(b.flightdir, p4Z));
        }
        
        if(bcands.size()==2){
            const Bcand & b0 = bcands[0];
            const Bcand & b1 = bcands[1];
            const auto & lplus = e.get(h_lepton_plus);
            const auto & lminus = e.get(h_lepton_minus);
            
            auto drbb = deltaR(b0.flightdir, b1.flightdir);
            auto dphibb = deltaPhi(b0.flightdir, b1.flightdir);
            
            fill(DR_BB, drbb);
            fill(DPhi_BB, dphibb);
            fill(m_BB, (b0.p4 + b1.p4).M());
            
            double dr0 = deltaR(p4Z, b0.p4);
            double dr1 = deltaR(p4Z, b1.p4);
            double min_dr = min(dr0, dr1);
            double max_dr = max(dr0, dr1);
            
            double min_dr_b0lep = min(deltaR(lplus.p4, b0.flightdir), deltaR(lminus.p4, b0.flightdir));
            double min_dr_b1lep = min(deltaR(lplus.p4, b1.flightdir), deltaR(lminus.p4, b1.flightdir));
            
            fill(DR_ZB, dr0);
            fill(DR_ZB, dr1);
            fill(min_DR_ZB, min_dr);
            fill(A_ZBB, (max_dr - min_dr) / (max_dr + min_dr));
            ID(min_DR_Blep);
            fill(min_DR_Blep, min(min_dr_b0lep, min_dr_b1lep));
            
            ID(bcand0pt_drbb); ID(bcand1pt_drbb);
            ID(bcand0pt_dphibb); ID(bcand1pt_dphibb);
            
            auto b0pt = bcands[0].p4.pt();
            auto b1pt = bcands[1].p4.pt();
            if(b1pt > b0pt) swap(b0pt, b1pt);
            
            fill(bcand0pt_drbb, drbb, b0pt);
            fill(bcand1pt_drbb, drbb, b1pt);
            fill(bcand0pt_dphibb, dphibb, b0pt);
            fill(bcand1pt_dphibb, dphibb, b1pt);
            
            ID(mll_drbb); ID(mll_dphibb);
            auto mll = p4Z.M();
            fill(mll_drbb, drbb, mll);
            fill(mll_dphibb, dphibb, mll);
        }

        fill(number_mcbs, mc_bhads.size());
        
        auto mc_bhads_sorted = mc_bhads;
        sort(mc_bhads_sorted.begin(), mc_bhads_sorted.end(), [](const mcparticle & p0, const mcparticle & p1){return p0.p4.pt() > p1.p4.pt();});
        
        if(mc_bhads_sorted.size() >= 2){
            auto drbb = deltaR(mc_bhads_sorted[0].p4, mc_bhads_sorted[1].p4);
            auto dphibb = deltaPhi(mc_bhads_sorted[0].p4, mc_bhads_sorted[1].p4);
            ID(mcb0pt_dphibb); ID(mcb0pt_drbb);
            ID(mcb1pt_dphibb); ID(mcb1pt_drbb);
            fill(mcb0pt_drbb, drbb, mc_bhads_sorted[0].p4.pt());
            fill(mcb1pt_drbb, drbb, mc_bhads_sorted[1].p4.pt());
            fill(mcb0pt_dphibb, dphibb, mc_bhads_sorted[0].p4.pt());
            fill(mcb1pt_dphibb, dphibb, mc_bhads_sorted[1].p4.pt());
        }
        
        
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
    Event::Handle<vector<mcparticle>> h_mcb_input;
    Event::Handle<double> h_weight;
    Event::Handle<vector<Bcand> > h_selected_bcands;
    Event::Handle<vector<jet> > h_jets;
    Event::Handle<LorentzVector> h_zp4;
    Event::Handle<lepton> h_lepton_plus, h_lepton_minus;
};

REGISTER_HISTS(BcandHists)
