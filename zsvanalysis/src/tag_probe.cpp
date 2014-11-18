#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/hists.hpp"

#include "TH1D.h"
#include "zsvtree.hpp"

using namespace ra;
using namespace std;


namespace{

struct probe_info {
    float pt, eta;
    bool is_bcand_tagged; // "passing" probe
    bool is_csv_tagged; // control for probe purity
    bool mc_isb; // MC truth: matches to B hadron
};

struct tp_parameters {
    // matching parameters: Delta R between jet and MC B hadron / reco B candidate
    float dr_mcb_jet;
    float dr_bcand_jet;
    float tag_csv_threshold; // default: medium
    float tag_ptmin; // default: 20.0
    float probe_csv_threshold; // default: loose
    
    bool require_doubletag;
    
    tp_parameters(): dr_mcb_jet(0.3), dr_bcand_jet(0.3), tag_csv_threshold(0.679), tag_ptmin(20.0), probe_csv_threshold(0.244), require_doubletag(false) {}
};

// can use unfiltered jets; must be sorted by pt.
// Note that eta and pt cuts on probe have te be done on the result, if needed.
vector<probe_info> tag_probe(const vector<jet> & jets, const vector<Bcand> & bcands, const vector<mcparticle> & mc_bs, const tp_parameters & pars){
    vector<probe_info> result;
    if(jets.size() < 2) return result;
    // only consider the leading two jets:
    for(size_t itag=0; itag < 2; ++itag){
        if(jets[itag].btag < pars.tag_csv_threshold) continue;
        for(size_t iprobe=0; iprobe < 2; ++iprobe){
            if(itag == iprobe) continue;
            probe_info pi{float(jets[iprobe].p4.pt()), float(jets[iprobe].p4.eta()), false, jets[iprobe].btag > pars.probe_csv_threshold, false};
            for(const auto & mcb: mc_bs){
                if(deltaR(mcb.p4, jets[iprobe].p4) < pars.dr_mcb_jet){
                    pi.mc_isb = true;
                    break;
                }
            }
            int nbc = 0;
            for(const auto & bc : bcands){
                if(deltaR(bc.flightdir, jets[iprobe].p4) < pars.dr_bcand_jet){
                    ++nbc;
                }
            }
            if((nbc >= 1 && !pars.require_doubletag) || (nbc >= 2 && pars.require_doubletag)){
                pi.is_bcand_tagged = true;
            }
            result.push_back(pi);
        }
    }
    return result;
}

    
    
}



class TagProbeHists: public Hists {
public:
    TagProbeHists(const ptree &, const std::string & dirname, const s_dataset & dataset, InputManager & in, OutputManager & out): Hists(dirname, dataset, out){
        book<TH1D>("tp_ptp", 60, 0, 180);
        book<TH1D>("tp_ptp_mcb", 60, 0, 180);
        book<TH1D>("tp_ptp_bcb", 60, 0, 180);
        book<TH1D>("tp_ptp_csvb", 60, 0, 180);

        book<TH1D>("tp_etap", 50, -2.5, 2.5);
        book<TH1D>("tp_etap_mcb", 50, -2.5, 2.5);
        book<TH1D>("tp_etap_bcb", 50, -2.5, 2.5);
        book<TH1D>("tp_etap_csvb", 50, -2.5, 2.5);
        
        book<TH1D>("tp2_ptp", 60, 0, 180);
        book<TH1D>("tp2_ptp_mcb", 60, 0, 180);
        book<TH1D>("tp2_ptp_bcb", 60, 0, 180);
        book<TH1D>("tp2_ptp_csvb", 60, 0, 180);
        
        h_weight = in.get_handle<double>("weight");
        h_jets = in.get_handle<vector<jet>>("jets");
        h_lepton_plus = in.get_handle<lepton>("lepton_plus");
        h_lepton_minus = in.get_handle<lepton>("lepton_minus");
        h_mc_bs = in.get_handle<vector<mcparticle>>("mc_bs");
        h_selected_bcands = in.get_handle<vector<Bcand>>("selected_bcands");
        
        pars_dt.require_doubletag = true;
        pars_dt.dr_bcand_jet = 0.5f;
        pars_dt.dr_mcb_jet = 0.5f;
    }
    
    void fill(const identifier & id, double value){
        get(id)->Fill(value, current_weight);
    }
    
    virtual void process(Event & e) override;

private:
    double current_weight;
    
    tp_parameters pars, pars_dt; // TODO: make configurable
    
    Event::Handle<double> h_weight;
    Event::Handle<vector<jet>> h_jets;
    Event::Handle<lepton> h_lepton_plus, h_lepton_minus;
    Event::Handle<vector<mcparticle>> h_mc_bs;
    Event::Handle<vector<Bcand>> h_selected_bcands;
};


void TagProbeHists::process(Event & e){
    current_weight = e.get(h_weight);
    const auto & all_jets = e.get<vector<jet>>(h_jets);
    const auto & lepton_plus = e.get<lepton>(h_lepton_plus);
    const auto & lepton_minus = e.get<lepton>(h_lepton_minus);
    vector<jet> jets;
    for(size_t i=0; i<all_jets.size(); ++i){
        if(fabs(all_jets[i].p4.eta()) < 2.4 && all_jets[i].p4.pt() > 20.0 && deltaR(all_jets[i].p4, lepton_plus.p4) > 0.5 && deltaR(all_jets[i].p4, lepton_minus.p4) > 0.5){
            jets.push_back(all_jets[i]);
        }
    }
    sort(jets.begin(), jets.end(), [](const jet & j1, const jet & j2){return j1.p4.pt () > j2.p4.pt();});
    const auto & mc_bs = e.get(h_mc_bs);
    const auto & bcands = e.get(h_selected_bcands);
    
    auto probe_infos = tag_probe(jets, bcands, mc_bs, pars);
    ID(tp_ptp);
    ID(tp_ptp_mcb);
    ID(tp_ptp_bcb);
    ID(tp_ptp_csvb);
    ID(tp_etap);
    ID(tp_etap_mcb);
    ID(tp_etap_bcb);
    ID(tp_etap_csvb);
    for(auto & pi : probe_infos){
        if(fabs(pi.eta) > 1.8) continue; // ignore probes 'too far' out (B cands only good until eta = 2.0)
        fill(tp_ptp, pi.pt);
        fill(tp_etap, pi.eta);
        if(pi.is_bcand_tagged){
            fill(tp_ptp_bcb, pi.pt);
            fill(tp_etap_bcb, pi.eta);
        }
        if(pi.is_csv_tagged){
            fill(tp_ptp_csvb, pi.pt);
            fill(tp_etap_csvb, pi.eta);
        }
        if(pi.mc_isb){
            fill(tp_ptp_mcb, pi.pt);
            fill(tp_etap_mcb, pi.eta);
        }
    }
    
    ID(tp2_ptp);
    ID(tp2_ptp_mcb);
    ID(tp2_ptp_bcb);
    ID(tp2_ptp_csvb);
    probe_infos = tag_probe(jets, bcands, mc_bs, pars_dt);
    for(auto & pi : probe_infos){
        if(fabs(pi.eta) > 1.8) continue; // ignore probes 'too far' out (B cands only good until eta = 2.0)
        fill(tp2_ptp, pi.pt);
        if(pi.is_bcand_tagged){
            fill(tp2_ptp_bcb, pi.pt);
        }
        if(pi.is_csv_tagged){
            fill(tp2_ptp_csvb, pi.pt);
        }
        if(pi.mc_isb){
            fill(tp2_ptp_mcb, pi.pt);
        }
    }
}


REGISTER_HISTS(TagProbeHists)

