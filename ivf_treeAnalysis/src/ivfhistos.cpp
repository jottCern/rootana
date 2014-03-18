#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/hists.hpp"

#include <iostream>
#include "TH1D.h"
#include "TH2D.h"
#include "ivftree.hpp"

#define ARRAY_SIZE 33


using namespace ra;
using namespace std;

namespace
{
    enum Category
      {
	  Fake = 0,
	  Hadronic,
	  Unknown,
	  Undefined,
	  GeantPrimary,
	  Decay,
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
}

// class BaseHists: public Hists {
// public:
//     BaseHists(const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
//         ID(zp4);
//         ID(met);
//         ID(selected_bcands);
//         book_1d_autofill([=](Event & e){return e.get<LorentzVector>(zp4).M();}, "mll", 200, 0, 200);
//         book_1d_autofill([=](Event & e){return e.get<LorentzVector>(zp4).pt();}, "ptz", 200, 0, 200);
//         book_1d_autofill([=](Event & e){return e.get<float>(met);}, "met", 200, 0, 200);
//         book_1d_autofill([=](Event & e){return e.get<vector<Bcand> >(selected_bcands).size();}, "nbcands", 10, 0, 10);
//     }
// };
// 
// REGISTER_HISTS(BaseHists)

class IvfHists: public Hists{
public:
    IvfHists(const ptree & xyz, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
	const char* catchar[] = {"Fake", "Hadronic", "Unknown", "Undefined", "GeantPrimary", "Decay", "Compton", "Annihilation", "EIoni", "HIoni", "MuIoni", "Photon", "MuPairProd", "Conversions", "EBrem", "SynchrotronRadiation", "MuBrem", "MuNucl", "Primary", "Proton", "BWeak", "CWeak", "FromBWeakDecayMuon", "FromCWeakDecayMuon", "ChargePion", "ChargeKaon", "Tau", "Ks", "Lambda", "Jpsi", "Xi", "SigmaPlus", "SigmaMinus"};
	for (unsigned int i = 0; i < ARRAY_SIZE; ++i)
	    categories.push_back(catchar[i]);
	
	disc_string_ = dirname.substr(dirname.size()-2);
	discriminator_ = std::atof(disc_string_.c_str())/10.;
	ZRho_ = false;
	Rho1D_ = false;
	
	
	if (dirname.find("ZRho") != std::string::npos) ZRho_ = true;
	if (dirname.find("Rho1D") != std::string::npos) Rho1D_ = true;
	    
	if (ZRho_)
	{
	    book<TH2D>("ZRho_All"+disc_string_, 560, -70, 70, 300, 0, 30);
	    
	    for (unsigned int i = 0; i < ARRAY_SIZE; ++i)
	    {
		identifier id("ZRho_"+categories[i]+disc_string_);
		book<TH2D>(id, 560, -70, 70, 300, 0, 30);
		cat_id.push_back(id);
		
	    }
	}
	
	if (Rho1D_)
	{
	    book<TH1D>("Rho1D_All"+disc_string_, 300, 0, 30);
	    
	    for (unsigned int i = 0; i < ARRAY_SIZE; ++i)
	    {
		identifier id("Rho1D_"+categories[i]+disc_string_);
		book<TH1D>(id, 300, 0, 30);
		cat_id.push_back(id);
		
	    }
	}
    }
    
    void fill(const identifier & id, double value){
        get(id)->Fill(value);
    }
    
    void fill(const identifier & id, double x, double y){
        get(id)->Fill(x, y);
    }
    
    virtual void process(Event & e){
	ID(thisProcessFlags);
	ID(recoPos);

        auto flags = e.get<Flags>(thisProcessFlags);
        const Point & position = e.get<Point>(recoPos);
	
	if (ZRho_)
	{
	    identifier ZRho_All("ZRho_All"+disc_string_);
	    fill(ZRho_All, position.z(), position.rho());
	    
	    for (unsigned int i = 0; i < flags.size(); ++i) // alternative: for (auto & id : cat_id) *then*
	    {
		if (flags[i] > discriminator_)
		    fill(cat_id[i], position.z(), position.rho());
	    }
	}
	
	if (Rho1D_)
	{
	    identifier Rho1D_All("Rho1D_All"+disc_string_);
	    if (std::abs(position.z()) < 29)
	    {
		fill(Rho1D_All, position.rho());
		
		for (unsigned int i = 0; i < flags.size(); ++i) // alternative: for (auto & id : cat_id) *then*
		{
		    if (flags[i] > discriminator_)
			fill(cat_id[i], position.rho());
		}
	    }
	}
	
	
	
//         for(auto & b : bcands){
//             fill(Bmass, b.p4.M());
//             fill(Bpt, b.p4.pt());
//             fill(Beta, b.p4.eta());
//             
//             fill(svdist2d, b.dist2D);
//             fill(svdist3d, b.dist3D);
//             fill(svdist2dsig, b.distSig2D);
//             fill(svdist3dsig, b.distSig3D);
//             
//             fill(DR_ZB, deltaR(b.p4, p4Z));
//         }
//         
//         if(bcands.size()==2){
//             const Bcand & b0 = bcands[0];
//             const Bcand & b1 = bcands[1];
//             
//             fill(DR_BB, deltaR(b0.p4, b1.p4));
//             fill(DPhi_BB, deltaPhi(b0.p4, b1.p4));
//             fill(m_BB, (b0.p4 + b1.p4).M());
//             
//             fill(DR_VV, deltaR(b0.flightdir, b1.flightdir));
//             fill(DPhi_VV, deltaPhi(b0.flightdir, b1.flightdir));
//             fill(dist_VV, sqrt((b0.flightdir -  b1.flightdir).Mag2()));
//             
//             double dr0 = deltaR(p4Z, b0.p4);
//             double dr1 = deltaR(p4Z, b1.p4);
//             double min_dr = min(dr0, dr1);
//             double max_dr = max(dr0, dr1);
//             
//             fill(DR_ZB, dr0);
//             fill(DR_ZB, dr1);
//             fill(min_DR_ZB, min_dr);
//             fill(A_ZBB, (max_dr - min_dr) / (max_dr + min_dr));
//         }
        
    }
    
private:
    
    std::vector<std::string> categories;
//     std::vector<std::string> cat_disc;
    std::vector<identifier> cat_id;
    
    float discriminator_;
    std::string disc_string_;
    
    bool ZRho_;
    bool Rho1D_;

};

REGISTER_HISTS(IvfHists)



/*
class zsvhistos: public AnalysisModule {
public:
    
    explicit zsvhistos(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    HistogramHelper h;
};

#define ID(name) identifier name(#name);

namespace{
    // event container:
    ID(eventNo); //identifier eventNo("eventNo");
    ID(lepton_plus);
    ID(lepton_minus);
    ID(selected_bcands);
    
    // histograms:
    ID(lep_plus_pt);
    ID(lep_plus_eta);
    ID(lep_minus_pt);
    ID(lep_minus_eta);
    ID(mll);
    ID(ptz);
    
    ID(n_bcands);
    
    
    // for exactly two B candidates:
    
}



zsvhistos::zsvhistos(const ptree & cfg) {
}

void zsvhistos::begin_dataset(const s_dataset & dataset, InputManager & in){
    h.create<TH1D>(lep_plus_pt, 100, 0, 100);
    h.create<TH1D>(lep_minus_pt, 100, 0, 100);
    h.create<TH1D>(lep_plus_eta, 100, -3, 3);
    h.create<TH1D>(lep_minus_eta, 100, -3, 3);
    h.create<TH1D>(mll, 200, 0, 200);
    h.create<TH1D>(ptz, 100, 0, 300);
    
    h.create<TH1D>(n_bcands, 5, 0, 5);
    
    
}

void zsvhistos::process(Event & event, OutputManager & out){
    //cout << "event number: " << event.get<int>(eventNo) << endl;
    h.process(event, out);
    
    const lepton & lplus = event.get<lepton>(lepton_plus);
    const lepton & lminus = event.get<lepton>(lepton_minus);
    
    h.fill(lep_plus_pt, lplus.p4.pt());
    h.fill(lep_plus_eta, lplus.p4.eta());
    h.fill(lep_minus_pt, lminus.p4.pt());
    h.fill(lep_minus_eta, lminus.p4.eta());
    
    LorentzVector Zp4 = lplus.p4 + lminus.p4;
    h.fill(mll, Zp4.M());
    h.fill(ptz, Zp4.pt());
    
    const vector<Bcand> & bcands = event.get<vector<Bcand> >(selected_bcands);
    h.fill(n_bcands, bcands.size());
    
    
}

REGISTER_ANALYSIS_MODULE(zsvhistos)
*/


