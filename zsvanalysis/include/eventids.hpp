#ifndef ZSVANALYSIS_EVENTIDS_HPP
#define ZSVANALYSIS_EVENTIDS_HPP

#include "ra/include/identifier.hpp"

#define DECLARE_ID(name) extern const ra::identifier name

DECLARE_ID(zp4);

DECLARE_ID(met);
DECLARE_ID(selected_bcands);
DECLARE_ID(mc_n_me_finalstate);
DECLARE_ID(lepton_plus);
DECLARE_ID(lepton_minus);

DECLARE_ID(mc_true_pileup);
DECLARE_ID(npv);

DECLARE_ID(mc_bs);

#undef DECLARE_ID

#endif
