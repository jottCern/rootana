#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/selections.hpp"

#include "zsvtree.hpp"

#include "TH1D.h"
#include "TH2D.h"

using namespace ra;
using namespace std;

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
 * reconstructed but not generated events (in particular all backgrounds) are filled in the gen overflow bin.
 * x-axis = reco, y-axis = gen
 * 
 * In addition, 1D genonly histograms are filled for signal (those will be empty for background). They correspond to
 * the generator-level distribution of the quantity. As such, they use a different weight than the overall event weight
 * as e.g. the overall event weight contains data/MC correction factors for different efficiencies which cannot (at least not
 * easily) be calculated for the non-reconstructed events. Therefore, for this histogram, the event weight saved as double
 * as genonly_weight is used; typically, this weight is simply the mc/data luminosity ratio.
 */
class fill_response: public AnalysisModule {
public:
    
    explicit fill_response(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    string genselection, recoselection;
    string input_mc_bs;
    string genonly_weight;
        
    TH2D * reco_gen_dphi, *reco_gen_dr;
    TH1D * genonly_dphi, *genonly_dr;
    
    Event::Handle<double> h_weight, h_genonly_weight;
    Event::Handle<bool> h_genselection, h_recoselection;
    Event::Handle<vector<mcparticle>> h_input_mc_bs;
    Event::Handle<vector<Bcand>> h_selected_bcands;
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
    
    h_genonly_weight = in.get_handle<double>(genonly_weight);
    h_weight = in.get_handle<double>("weight");
    h_genselection = in.get_handle<bool>(genselection);
    h_recoselection = in.get_handle<bool>(recoselection);
    h_input_mc_bs = in.get_handle<vector<mcparticle>>(input_mc_bs);
    h_selected_bcands = in.get_handle<vector<Bcand>>("selected_bcands");
}


void fill_response::process(Event & event){
    auto gen = event.get(h_genselection);
    auto reco = event.get(h_recoselection);
    if(!gen and !reco) return;
    
    double dphi_gen = numeric_limits<double>::infinity();
    double dr_gen = numeric_limits<double>::infinity();
    
    double weight = event.get(h_weight);
    
    if(gen){
        auto & mc_bs = event.get<vector<mcparticle>>(h_input_mc_bs);
        if(mc_bs.size() != 2) throw runtime_error("gen-selected event but mc_bs.size() != 2!");
        dphi_gen = fabs(deltaPhi(mc_bs[0].p4, mc_bs[1].p4));
        dr_gen = deltaR(mc_bs[0].p4, mc_bs[1].p4);
        double w = event.get<double>(h_genonly_weight);
        genonly_dphi->Fill(dphi_gen, w);
        genonly_dr->Fill(dr_gen, w);
    }
    if(reco){
        auto & reco_bs = event.get(h_selected_bcands);
        if(reco_bs.size() != 2) throw runtime_error("reco-selected event but reco_bs.size() != 2!");
        double dphi_reco = fabs(deltaPhi(reco_bs[0].flightdir, reco_bs[1].flightdir));
        double dr_reco = deltaR(reco_bs[0].flightdir, reco_bs[1].flightdir);
        
        // this is ok also if !gen, as gen is +infinity then, and those events end up in the gen overflow bin
        reco_gen_dphi->Fill(dphi_reco, dphi_gen, weight);
        reco_gen_dr->Fill(dr_reco, dr_gen, weight);
    }
}

REGISTER_ANALYSIS_MODULE(fill_response)
