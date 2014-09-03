#ifndef ZSVANALYSIS_EVENTIDS_HPP
#define ZSVANALYSIS_EVENTIDS_HPP

#include "ra/include/identifier.hpp"

#define DECLARE_ID(name) extern const ra::identifier name

namespace zsv {
namespace id {
    
// ttree input:
DECLARE_ID(lumiNo);
DECLARE_ID(eventNo);
DECLARE_ID(runNo);
DECLARE_ID(passed_reco_selection);
  
DECLARE_ID(met);
DECLARE_ID(met_phi);
  
DECLARE_ID(lepton_minus);
DECLARE_ID(lepton_plus);
  
DECLARE_ID(selected_bcands);
DECLARE_ID(additional_bcands);
  
DECLARE_ID(jets);
DECLARE_ID(npv);
 
DECLARE_ID(mc_true_pileup);
DECLARE_ID(mc_jets);
DECLARE_ID(mc_leptons);
DECLARE_ID(mc_n_me_finalstate);
DECLARE_ID(mc_bs);
DECLARE_ID(mc_cs);

// calculated:
DECLARE_ID(zp4);
DECLARE_ID(elesf);
DECLARE_ID(musf);
DECLARE_ID(pileupsf);

}
}

#undef DECLARE_ID

#endif
