#ifndef IVFUTILS_HPP
#define IVFUTILS_HPP

#include <string>

#define CAT_ARRAY_SIZE 33
#define GEANT_ARRAY_SIZE 18

namespace ivf
{
    
    /// maybe think about making a category class instead of just an enum??
    enum Category
    {
	  Fake = 0,
	  Hadronic,
	  Unknown,
	  Undefined,
	  GeantPrimary,
	  GeantDecay,
	  Compton,
	  Annihilation,
	  EIoni,
	  HIoni,
	  MuIoni,
	  Photon,
	  MuPairProd,
	  Conversions,
	  EBrem,
	  SynchrotronRadiation,
	  MuBrem,
	  MuNucl,
	  Primary,
	  Proton,
	  BWeak,
	  CWeak,
	  FromBWeakDecayMuon,
	  FromCWeakDecayMuon,
	  ChargePion,
	  ChargeKaon,
	  Tau,
	  Ks,
	  Lambda,
	  Jpsi,
	  Xi,
	  SigmaPlus,
	  SigmaMinus
    };
    
    const char* cattostr(Category cat);
    const char* cattostr(int cat);
    
    Category strtocat(std::string str);
    
//     std::string listcat(std::string instr, std::string & outstr);
}

#endif
