#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/hists.hpp"
#include <boost/algorithm/string.hpp>

// #include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include "TH1D.h"
#include "TH2D.h"
#include "ivftree.hpp"
#include "ivfutils.hpp"


using namespace ra;
using namespace std;
using namespace ivf;




class IvfPosHists: public Hists{
public:
    IvfPosHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
	
	book<TH2D>("ZRho", 560, -70, 70, 300, 0, 30);
	book<TH1D>("Rho1D", 300, 0, 30);
   
    }
    
    virtual void fill(const identifier & id, double x){
        get(id)->Fill(x);
    }
    
    virtual void fill(const identifier & id, double x, double y){
        get(id)->Fill(x, y);
    }
    
    virtual void process(Event & e){
	ID(recoPos);
	ID(ZRho);
	ID(Rho1D);
	
        const Point & position = e.get<Point>(recoPos);
	
	fill(ZRho, position.z(), position.rho());
	fill(Rho1D, position.rho());
        
    }
    
private:

};

REGISTER_HISTS(IvfPosHists)




class IvfCatHists: public Hists{
public:
    IvfCatHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, OutputManager & out): Hists(dirname, dataset, out){
	discriminator_ = ra::get<float>(cfg, "discriminator", 0.);
	stringstream disc_str;
	disc_str << discriminator_;
	histname = disc_str.str();
	std::string catstr = ra::get<std::string>(cfg, "categories", "All");
	ID(All);
	identifier catid(catstr);
	
	if (All == catid)
	{
	    for (int i = 0; i < GEANT_ARRAY_SIZE; ++i)
		catstrings_.push_back(cattostr(i));
	}
	else {
	    boost::split(catstrings_, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
	}
	
	num_cat_ = catstrings_.size();
	
	book<TH1D>("Processes"+histname, num_cat_, 0, num_cat_);
	
	for (int i = 1; i <= num_cat_; ++i)
	    get("Processes"+histname)->GetXaxis()->SetBinLabel(i, catstrings_[i-1].c_str());
   
    }
    
    virtual void fill(const identifier & id, double x){
        get(id)->Fill(x);
    }
    
    virtual void process(Event & e){
	identifier Processes("Processes"+histname);
	ID(thisProcessFlags);
	
	Flags & flags = e.get<Flags>(thisProcessFlags);
	
	for (int i = 0; i < num_cat_; ++i)
	{
	    if (flags[strtocat(catstrings_[i])] > discriminator_)
		fill(Processes, i);
	}
        
    }
    
private:
    
    std::vector<string> catstrings_;
    std::string histname;
    int num_cat_;
    float discriminator_;

};

REGISTER_HISTS(IvfCatHists)




