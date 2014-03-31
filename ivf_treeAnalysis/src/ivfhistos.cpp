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
    const char* catchar[] = {"Fake", "Hadronic", "Unknown", "Undefined", "GeantPrimary", "GeantDecay", "Compton", "Annihilation", "EIoni", "HIoni", "MuIoni", "Photon", "MuPairProd", "Conversions", "EBrem", "SynchrotronRadiation", "MuBrem", "MuNucl", "PrimaryDecay", "Proton", "BWeak", "CWeak", "FromBWeakDecayMuon", "FromCWeakDecayMuon", "ChargePion", "ChargeKaon", "Tau", "Ks", "Lambda", "Jpsi", "Xi", "SigmaPlus", "SigmaMinus"};
    
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
    
    struct CatContainer {
	std::vector<Category> geant_cat;
	std::vector<Category> decay_cat;
	
	unsigned int geant_cat_size;
	unsigned int decay_cat_size;
	
	CatContainer() {
	geant_cat.push_back(GeantPrimary); geant_cat.push_back(Decay); geant_cat.push_back(Conversions); geant_cat.push_back(Hadronic); geant_cat.push_back(EBrem); geant_cat.push_back(Unknown); geant_cat.push_back(Undefined); geant_cat.push_back(Compton); geant_cat.push_back(Annihilation); geant_cat.push_back(EIoni); geant_cat.push_back(HIoni); geant_cat.push_back(MuIoni); geant_cat.push_back(Photon); geant_cat.push_back(MuPairProd); geant_cat.push_back(SynchrotronRadiation); geant_cat.push_back(MuBrem); geant_cat.push_back(MuNucl);
	
	decay_cat.push_back(BWeak); decay_cat.push_back(CWeak); decay_cat.push_back(Primary); decay_cat.push_back(Ks); decay_cat.push_back(ChargePion); decay_cat.push_back(ChargeKaon); decay_cat.push_back(Tau); decay_cat.push_back(Lambda); decay_cat.push_back(Jpsi); decay_cat.push_back(Xi); decay_cat.push_back(SigmaPlus); decay_cat.push_back(SigmaMinus);
	
	geant_cat_size = geant_cat.size();
	decay_cat_size = decay_cat.size();
	}
    } cats;
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

class IvfZRhoGeantHists: public Hists{
public:
    IvfZRhoGeantHists(const ptree & xyz, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
	for (unsigned int i = 0; i < ARRAY_SIZE; ++i)
	    categories.push_back(catchar[i]);
	
	disc_string_ = dirname.substr(dirname.size()-2);
	discriminator_ = std::atof(disc_string_.c_str())/10.;
	
	book<TH2D>("ZRho_GeantAll"+disc_string_, 560, -70, 70, 300, 0, 30);
	
	for (unsigned int i = 0; i < cats.geant_cat_size; ++i)
	{
	    identifier id("ZRho_"+categories[cats.geant_cat[i]]+disc_string_);
	    book<TH2D>(id, 560, -70, 70, 300, 0, 30);
	    cat_id.push_back(id);
	}
	
	book<TH2D>("ZRho_GeantRest"+disc_string_, 560, -70, 70, 300, 0, 30);
	book<TH2D>("ZRho_GeantRestPlot"+disc_string_, 560, -70, 70, 300, 0, 30);
   
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
	
	identifier All("ZRho_GeantAll"+disc_string_);
	identifier Rest("ZRho_GeantRest"+disc_string_);
	identifier RestPlot("ZRho_GeantRestPlot"+disc_string_);
	
	//begin filling histograms
	
	bool hist_filled = false;
	
	fill(All, position.z(), position.rho());
	
	for (unsigned int i = 0; i < cats.geant_cat_size; ++i) // alternative: for (auto & id : cat_id) *then*
	{
	    if (flags[cats.geant_cat[i]] > discriminator_)
	    {
		fill(cat_id[i], position.z(), position.rho());
		hist_filled = true;
	    }
	}
	
	if (!hist_filled && (flags[GeantPrimary]+flags[Decay]) > discriminator_)
	{
	    fill(cat_id[4], position.z(), position.rho()); // make nicer access, e.g. using a map of enums+index instead of a vector
	    hist_filled = true;
	}
	
	if (!hist_filled)
	    fill(Rest, position.z(), position.rho());
	
	if (flags[Unknown] > discriminator_ || flags[Undefined] > discriminator_ || flags[Compton] > discriminator_ || flags[Annihilation] > discriminator_ || flags[EIoni] > discriminator_ || flags[HIoni] > discriminator_ || flags[MuIoni] > discriminator_ || flags[Photon] > discriminator_ || flags[MuPairProd] > discriminator_ || flags[SynchrotronRadiation] > discriminator_ || flags[MuBrem] > discriminator_ || flags[MuNucl] > discriminator_)
	    fill(RestPlot, position.z(), position.rho());
        
    }
    
private:
    
    std::vector<std::string> categories;
//     std::vector<std::string> cat_disc;
    std::vector<identifier> cat_id;
    
    float discriminator_;
    std::string disc_string_;

};

REGISTER_HISTS(IvfZRhoGeantHists)




class IvfZRhoDecayHists: public Hists{
public:
    IvfZRhoDecayHists(const ptree & xyz, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
	for (unsigned int i = 0; i < ARRAY_SIZE; ++i)
	    categories.push_back(catchar[i]);
	
	disc_string_ = dirname.substr(dirname.size()-2);
	discriminator_ = std::atof(disc_string_.c_str())/10.;
	
	book<TH2D>("ZRho_DecayAll"+disc_string_, 560, -70, 70, 300, 0, 30);
	
	for (unsigned int i = 0; i < cats.decay_cat_size; ++i)
	{
	    identifier id("ZRho_"+categories[cats.decay_cat[i]]+disc_string_);
	    book<TH2D>(id, 560, -70, 70, 300, 0, 30);
	    cat_id.push_back(id);
	}
	
	book<TH2D>("ZRho_DecayRest"+disc_string_, 560, -70, 70, 300, 0, 30);
	book<TH2D>("ZRho_DecayRestPlot"+disc_string_, 560, -70, 70, 300, 0, 30);
   
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
	
	identifier RestPlot("ZRho_DecayRestPlot"+disc_string_);
	
	if (flags[GeantPrimary]+flags[Decay] <= discriminator_)
	{
	    fill(RestPlot, position.z(), position.rho());
	    return;
	}
	
	identifier All("ZRho_DecayAll"+disc_string_);
	identifier Rest("ZRho_DecayRest"+disc_string_);
	
// 	bool hist_filled = false;
	
	fill(All, position.z(), position.rho());
	
	identifier dec_id;
	std::pair<float, identifier> best_decay = std::make_pair(0., dec_id);
	int cat = -1;
	
	for (unsigned int i = 0; i < cats.decay_cat_size; ++i) // alternative: for (auto & id : cat_id) *then*
	{
	    if (cats.decay_cat[i] == CWeak)
	    {
		if (flags[CWeak] > best_decay.first && flags[BWeak] == 0.)
		{
		    best_decay = std::make_pair(flags[CWeak], cat_id[1]);
		    cat = CWeak;
// 		    hist_filled = true;
// 		    fill(cat_id[1], position.z(), position.rho());
		}
		else if (flags[CWeak]+flags[BWeak] > best_decay.first && flags[BWeak] > 0.)
		{
		    best_decay = std::make_pair(flags[CWeak]+flags[BWeak], cat_id[0]);
		    cat = BWeak;
		}
		
	    }
	    else
	    {
		if (flags[cats.decay_cat[i]] > best_decay.first)
		{
		    best_decay = std::make_pair(flags[cats.decay_cat[i]], cat_id[i]);
		    cat = cats.decay_cat[i];
// 		    hist_filled = true;
// 		    fill(cat_id[i], position.z(), position.rho());
		}
	    }
	}
	
	if (best_decay.second.id() == -1)
	{
	    best_decay = std::make_pair(0., Rest);
// 	    fill(0, position.z(), position.rho()); // make nicer access, e.g. using a map of enums+index instead of a vector
// 	    hist_filled = true;
	}
	
	fill(best_decay.second, position.z(), position.rho());
	
	if (cat == ChargePion || cat == ChargeKaon || cat == Tau || cat == Lambda || cat == Jpsi || cat == Xi || cat == SigmaPlus || cat == SigmaMinus || cat == -1)
	    fill(RestPlot, position.z(), position.rho());
        
    }
    
private:
    
    std::vector<std::string> categories;
//     std::vector<std::string> cat_disc;
    std::vector<identifier> cat_id;
    
    float discriminator_;
    std::string disc_string_;

};

REGISTER_HISTS(IvfZRhoDecayHists)




class IvfRho1DGeantHists: public Hists{
public:
    IvfRho1DGeantHists(const ptree & xyz, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
	
	for (unsigned int i = 0; i < ARRAY_SIZE; ++i)
	    categories.push_back(catchar[i]);
	
	disc_string_ = dirname.substr(dirname.size()-2);
	discriminator_ = std::atof(disc_string_.c_str())/10.;
	
	book<TH1D>("Rho1D_GeantAll"+disc_string_, 150, 0, 15);
	
	for (unsigned int i = 0; i < cats.geant_cat_size; ++i)
	{
	    identifier id("Rho1D_"+categories[cats.geant_cat[i]]+disc_string_);
	    book<TH1D>(id, 150, 0, 15);
	    cat_id.push_back(id);
	}
	
	book<TH1D>("Rho1D_GeantRest"+disc_string_, 150, 0, 15);
	book<TH1D>("Rho1D_GeantRestPlot"+disc_string_, 150, 0, 15);
	
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
	
	identifier All("Rho1D_GeantAll"+disc_string_);
	identifier Rest("Rho1D_GeantRest"+disc_string_);
	identifier RestPlot("Rho1D_GeantRestPlot"+disc_string_);
	
	//begin filling histograms
	
	bool hist_filled = false;
	
	fill(All, position.rho());
	
	for (unsigned int i = 0; i < cats.geant_cat_size; ++i) // alternative: for (auto & id : cat_id) *then*
	{
	    if (flags[cats.geant_cat[i]] > discriminator_)
	    {
		fill(cat_id[i], position.rho());
		hist_filled = true;
	    }
	}
	
	if (!hist_filled && (flags[GeantPrimary]+flags[Decay]) > discriminator_)
	{
	    fill(cat_id[4], position.rho()); // make nicer access, e.g. using a map of enums+index instead of a vector
	    hist_filled = true;
	}
	
	if (!hist_filled)
	    fill(Rest, position.rho());
	
	if (flags[Unknown] > discriminator_ || flags[Undefined] > discriminator_ || flags[Compton] > discriminator_ || flags[Annihilation] > discriminator_ || flags[EIoni] > discriminator_ || flags[HIoni] > discriminator_ || flags[MuIoni] > discriminator_ || flags[Photon] > discriminator_ || flags[MuPairProd] > discriminator_ || flags[SynchrotronRadiation] > discriminator_ || flags[MuBrem] > discriminator_ || flags[MuNucl] > discriminator_)
	    fill(RestPlot, position.rho());
        
    }
    
private:
    
    std::vector<std::string> categories;
//     std::vector<std::string> cat_disc;
    std::vector<identifier> cat_id;
    
    float discriminator_;
    std::string disc_string_;

};

REGISTER_HISTS(IvfRho1DGeantHists)




class IvfRho1DDecayHists: public Hists{
public:
    IvfRho1DDecayHists(const ptree & xyz, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
	
	for (unsigned int i = 0; i < ARRAY_SIZE; ++i)
	    categories.push_back(catchar[i]);
	
	disc_string_ = dirname.substr(dirname.size()-2);
	discriminator_ = std::atof(disc_string_.c_str())/10.;
	
	book<TH1D>("Rho1D_DecayAll"+disc_string_, 150, 0, 15);
	
	for (unsigned int i = 0; i < cats.decay_cat_size; ++i)
	{
	    identifier id("Rho1D_"+categories[cats.decay_cat[i]]+disc_string_);
	    book<TH1D>(id, 150, 0, 15);
	    cat_id.push_back(id);
	}
	
	book<TH1D>("Rho1D_DecayRest"+disc_string_, 150, 0, 15);
	book<TH1D>("Rho1D_DecayRestPlot"+disc_string_, 150, 0, 15);
	
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
	
	identifier RestPlot("Rho1D_DecayRestPlot"+disc_string_);
	
	if (flags[GeantPrimary]+flags[Decay] <= discriminator_)
	{
	    fill(RestPlot, position.rho());
	    return;
	}
	
	identifier All("Rho1D_DecayAll"+disc_string_);
	identifier Rest("Rho1D_DecayRest"+disc_string_);
	
// 	bool hist_filled = false;
	
	fill(All, position.rho());
	
	identifier dec_id;
	std::pair<float, identifier> best_decay = std::make_pair(0., dec_id);
	int cat = 0;
	
	for (unsigned int i = 0; i < cats.decay_cat_size; ++i) // alternative: for (auto & id : cat_id) *then*
	{
	    if (cats.decay_cat[i] == CWeak)
	    {
		if (flags[CWeak] > best_decay.first && flags[BWeak] == 0.)
		{
		    best_decay = std::make_pair(flags[CWeak], cat_id[1]);
		    cat = CWeak;
// 		    hist_filled = true;
// 		    fill(cat_id[1], position.z(), position.rho());
		}
		else if (flags[CWeak]+flags[BWeak] > best_decay.first && flags[BWeak] > 0.)
		{
		    best_decay = std::make_pair(flags[CWeak]+flags[BWeak], cat_id[0]);
		    cat = BWeak;
		}
	    }
	    else
	    {
		if (flags[cats.decay_cat[i]] > best_decay.first)
		{
		    best_decay = std::make_pair(flags[cats.decay_cat[i]], cat_id[i]);
		    cat = cats.decay_cat[i];
// 		    hist_filled = true;
// 		    fill(cat_id[i], position.z(), position.rho());
		}
	    }
	}
	
	if (best_decay.second.id() == -1)
	{
	    best_decay = std::make_pair(0., Rest);
// 	    fill(0, position.z(), position.rho()); // make nicer access, e.g. using a map of enums+index instead of a vector
// 	    hist_filled = true;
	}
	
	fill(best_decay.second, position.rho());
	
	if (cat == ChargePion || cat == ChargeKaon || cat == Tau || cat == Lambda || cat == Jpsi || cat == Xi || cat == SigmaPlus || cat == SigmaMinus || cat == -1)
	    fill(RestPlot, position.z(), position.rho());
        
    }
			        
private:
    
    std::vector<std::string> categories;
    //     std::vector<std::string> cat_disc;
    std::vector<identifier> cat_id;
    
    float discriminator_;
    std::string disc_string_;
    
};

REGISTER_HISTS(IvfRho1DDecayHists)



