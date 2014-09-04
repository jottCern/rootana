#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/selections.hpp"

#include "zsvtree.hpp"

using namespace ra;
using namespace std;

/** \brief Write new mcparticle-vector to the event with selected input particles based on pt and eta cuts
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
 * Will put a new collection of type vector<mcparticle> with name \c output to the event that contains all mcparticles read from 
 * \c input of the Event container which pass ptmin and etamax cuts (the etamax is applied on |eta|).
 *
 */
class select_mcparticles: public AnalysisModule {
public:
    explicit select_mcparticles(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
        h_input = in.get_handle<vector<mcparticle>>(s_input);
        h_output = in.get_handle<vector<mcparticle>>(s_output);
    }
    virtual void process(Event & event);
    
private:
    string s_input, s_output;
    double ptmin, etamax;
    
    Event::Handle<vector<mcparticle>> h_input, h_output;
};


select_mcparticles::select_mcparticles(const ptree & cfg){
    ptmin = ptree_get<double>(cfg, "ptmin");
    etamax = ptree_get<double>(cfg, "etamax");
    s_input =  ptree_get<string>(cfg, "input");
    s_output =  ptree_get<string>(cfg, "output");
}

void select_mcparticles::process(Event & event){
    const auto & input_particles = event.get(h_input);
    vector<mcparticle> output_particles;
    for(const auto & p : input_particles){
        if(p.p4.pt() > ptmin && fabs(p.p4.eta()) < etamax){
            output_particles.push_back(p);
        }
    }
    event.set(h_output, std::move(output_particles));
}

REGISTER_ANALYSIS_MODULE(select_mcparticles)


/** \brief gen-level selection to define the phase space for the unfolding
 * 
 * Visible phase space for this analysis is defined as events with exactly two opposite-sign selected leptons with 
 * mll_max > mll > mll_min and the given number of B hadrons (usually two). This module only tests the lepton and B multiplicity and the 
 * mll criterion. The lepton and B kinematic selection has to be performed outside of thise module (e.g. with select_mcparticles).
 * 
 * \code
 * mllmin    81
 * mllmax   101
 * input_bs    selected_mc_bs
 * input_leps  selected_mc_leps
 * nb_min   2 ; optional: default: 2
 * nb_max   2 ; optional: default: 2
 * \endcode
 * 
 * If input_leps is the empty string, the lepton cut (mllmin and mllmax) is not applied.
 */
class gen_phasespace: public Selection {
public:
    gen_phasespace(const ptree & cfg, InputManager & in, OutputManager &);
    virtual bool operator()(const Event & e);
    
private:
    int nb_min, nb_max;
    double mllmin, mllmax;
    
    Event::Handle<vector<mcparticle>> h_input_bs, h_input_leps;
};

gen_phasespace::gen_phasespace(const ptree & cfg, InputManager & in, OutputManager &){
    nb_min = ptree_get<int>(cfg, "nb_min", 2);
    nb_max = ptree_get<int>(cfg, "nb_max", 2);
    h_input_bs = in.get_handle<vector<mcparticle>>(ptree_get<string>(cfg, "input_bs"));
    string s_input_leps = ptree_get<string>(cfg, "input_leps", "");
    if(!s_input_leps.empty()){
        h_input_leps = in.get_handle<vector<mcparticle>>(s_input_leps);
        mllmin = ptree_get<double>(cfg, "mllmin");
        mllmax = ptree_get<double>(cfg, "mllmax");
    }
    else{
        mllmin = mllmax = -1.0;
    }
    
    
}


bool gen_phasespace::operator()(const Event & e){
    // b cut:
    auto & bs = e.get<vector<mcparticle>>(h_input_bs);
    int nbs = bs.size();
    if(nbs < nb_min or nbs > nb_max) return false;
    
    // lepton cut (only if enabled)
    if(mllmin != -1.0 or mllmax != -1.0){
        auto & leps = e.get<vector<mcparticle>>(h_input_leps);
        if(leps.size() != 2) return false;
        if(leps[0].pdgid * leps[1].pdgid > 0) return false;
        double mll = (leps[0].p4 + leps[1].p4).M();
        if(mll < mllmin or mll < mllmax) return false;
    }
    return true;
}

REGISTER_SELECTION(gen_phasespace)
