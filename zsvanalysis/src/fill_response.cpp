#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/selections.hpp"

#include "zsvtree.hpp"
#include "eventids.hpp"

#include "TH1D.h"
#include "TH2D.h"

using namespace ra;
using namespace std;

/** \brief Write new mcparticle-vector to the event with selected input particles
 * 
 * configuration:
 * \code
 * type select_mcparticles
 * input mc_bs
 * output selected_mc_bs
 * ptmin 15
 * etamax 2.0
 * \endcode
 * 
 * Will put a new collection of type vector<mcparticle> with name \c name to the event that contains all mc_bs of the input
 * event that pass ptmin and etamax cuts.
 *
 */
class select_mcparticles: public AnalysisModule {
public:
    explicit select_mcparticles(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){}
    virtual void process(Event & event);
    
private:
    identifier input, output;
    double ptmin, etamax;
};


select_mcparticles::select_mcparticles(const ptree & cfg){
    ptmin = ptree_get<double>(cfg, "ptmin");
    etamax = ptree_get<double>(cfg, "etamax");
    input =  ptree_get<string>(cfg, "input");
    output =  ptree_get<string>(cfg, "output");
}

void select_mcparticles::process(Event & event){
    const auto & input_particles = event.get<vector<mcparticle> >(input);
    vector<mcparticle> output_particles;
    for(const auto & p : input_particles){
        if(p.p4.pt() > ptmin && fabs(p.p4.eta()) < etamax){
            output_particles.push_back(p);
        }
    }
    event.set<vector<mcparticle>>(output, output_particles);
}

REGISTER_ANALYSIS_MODULE(select_mcparticles)


/** \brief gen-level selection to define the phase space for the unfolding
 * 
 * Visible phase space for this analysis is defined as events with exactly two opposite-sign selected leptons with 
 * mll_max > mll > mll_min and exactly two B hadrons. Thjis module only tests the lepton and B multiplicity and the 
 * mll criterion. The lepton and B kinematic selection has to be performed outside of thise module (e.g. with select_mcparticles).
 * 
 * \code
 * mllmin    81
 * mllmax   101
 * input_bs    selected_mc_bs
 * input_leps  selected_mc_leps
 * \endcode
 */
class gen_phasespace: public Selection {
public:
    gen_phasespace(const ptree & cfg, OutputManager &);
    virtual bool operator()(const Event & e);
    
private:
    identifier input_bs, input_leps;
    double mllmin, mllmax;
};

gen_phasespace::gen_phasespace(const ptree & cfg, OutputManager &){
    mllmin = ptree_get<double>(cfg, "mllmin");
    mllmax = ptree_get<double>(cfg, "mllmax");
    input_bs = ptree_get<string>(cfg, "input_bs");
    input_leps = ptree_get<string>(cfg, "input_leps");
}


bool gen_phasespace::operator()(const Event & e){
    auto & bs = e.get<vector<mcparticle>>(input_bs);
    auto & leps = e.get<vector<mcparticle>>(input_leps);
    if(bs.size() != 2 || leps.size() != 2) return false;
    if(leps[0].pdgid * leps[1].pdgid > 0) return false;
    double mll = (leps[0].p4 + leps[1].p4).M();
    return mll > mllmin and mll < mllmax;
}

REGISTER_SELECTION(gen_phasespace)

/** \brief Fill the response matrices for unfolding B angular variables.
 *
 *
 * \code
 * type fill_response
 * genselection gen
 * recoselection full_selection
 * input_mc_bs selected_mc_bs
 * genonly_weight lumiweight
 * \endcode
 * 
 * Produces some the 2D histograms of the gen versus reco.
 * recostructed but not generated events (in particular all backgrounds) are filled in the gen overflow bin.
 * x-axis = reco, y-axis = gen
 * 
 * In addition, 1D genonly histograms are filled for signal (those will be empty for background). They correspond to
 * the generator-level distribution of the quantity. As such, they use a different weight than the overall event weight
 * as e.g. the overall event weight contains data/MC correction factors for different efficiencies which cannot (at least not
 * easily) be calculated for the non-reconstructed events. Therefore, for this histogram, the event weight saved as double
 * as genonly_weight is used; typically, this is the weight only based on mc/data luminosity ratio.
 */
class fill_response: public AnalysisModule {
public:
    
    explicit fill_response(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    identifier genselection, recoselection;
    identifier input_mc_bs;
    identifier genonly_weight;
        
    TH2D * reco_gen_dphi, *reco_gen_dr;
    TH1D * genonly_dphi, *genonly_dr;
};

fill_response::fill_response(const ptree & cfg){
    genselection = ptree_get<string>(cfg, "genselection");
    recoselection = ptree_get<string>(cfg, "recoselection");
    input_mc_bs = ptree_get<string>(cfg, "input_mc_bs");
    genonly_weight = ptree_get<string>(cfg, "genonly_weight");
}

namespace {
    void put(OutputManager & out, TH1 * t){
        out.put(t->GetName(), t);
    }
}

void fill_response::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    reco_gen_dphi = new TH2D("reco_gen_dphi", "reco_gen_dphi", 30, 0, M_PI, 30, 0, M_PI);
    genonly_dphi = new TH1D("genonly_dphi", "genonly_dphi", 30, 0, M_PI);
    reco_gen_dr = new TH2D("reco_gen_dr", "reco_gen_dr", 30, 0, 6, 30, 0, 6);
    genonly_dr = new TH1D("genonly_dr", "genonly_dr", 30, 0, 6);
    put(out, reco_gen_dphi);
    put(out, genonly_dphi);
    put(out, reco_gen_dr);
    put(out, genonly_dr);
}


void fill_response::process(Event & event){
    auto gen = event.get<bool>(genselection);
    auto reco = event.get<bool>(recoselection);
    if(!gen and !reco) return;
    
    double dphi_gen = numeric_limits<double>::infinity();
    double dr_gen = numeric_limits<double>::infinity();
    
    if(gen){
        auto & mc_bs = event.get<vector<mcparticle>>(input_mc_bs);
        if(mc_bs.size() != 2) throw runtime_error("gen-selected event but mc_bs.size() != 2!");
        dphi_gen = fabs(deltaPhi(mc_bs[0].p4, mc_bs[1].p4));
        dr_gen = deltaR(mc_bs[0].p4, mc_bs[1].p4);
        double w = event.get<double>(genonly_weight);
        genonly_dphi->Fill(dphi_gen, w);
        genonly_dr->Fill(dr_gen, w);
    }
    if(reco){
        auto & reco_bs = event.get<vector<Bcand>>(selected_bcands);
        if(reco_bs.size() != 2) throw runtime_error("reco-selected event but reco_bs.size() != 2!");
        double dphi_reco = fabs(deltaPhi(reco_bs[0].flightdir, reco_bs[1].flightdir));
        double dr_reco = deltaR(reco_bs[0].flightdir, reco_bs[1].flightdir);
        
        // this is ok also if !gen, as gen is +infinity then, and those events end up in the gen overflow bin
        reco_gen_dphi->Fill(dphi_reco, dphi_gen, event.weight());
        reco_gen_dr->Fill(dr_reco, dr_gen, event.weight());
    }
}

REGISTER_ANALYSIS_MODULE(fill_response)
