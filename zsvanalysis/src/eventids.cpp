#include "eventids.hpp"

#define DEFINE_ID(name) const ra::identifier zsv::id::name(#name);

// ttree input:
DEFINE_ID(lumiNo);
DEFINE_ID(eventNo);
DEFINE_ID(runNo);
DEFINE_ID(passed_reco_selection);
  
DEFINE_ID(met);
DEFINE_ID(met_phi);
  
DEFINE_ID(lepton_minus);
DEFINE_ID(lepton_plus);
  
DEFINE_ID(selected_bcands);
DEFINE_ID(additional_bcands);
  
DEFINE_ID(jets);
DEFINE_ID(npv);
 
DEFINE_ID(mc_true_pileup);
DEFINE_ID(mc_jets);
DEFINE_ID(mc_leptons);
DEFINE_ID(mc_n_me_finalstate);
DEFINE_ID(mc_bs);
DEFINE_ID(mc_cs);

// calculated:
DEFINE_ID(zp4);

// framework:
DEFINE_ID(stop);
