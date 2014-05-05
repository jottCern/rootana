#include "ra/include/selections.hpp"
#include "ivftree.hpp"
#include "ivfutils.hpp"

#include <boost/algorithm/string.hpp>

using namespace ra;
using namespace std;
using namespace ivf;

class MinTvQualSelection: public Selection{
public:
    MinTvQualSelection(const ptree & cfg, OutputManager &){
        disc = ptree_get<float>(cfg, "disc", .0);
    }
    
    bool operator()(const Event & event){
        static identifier tvquality("tvquality");
        float qual = event.get<float>(tvquality);
        return (qual > disc);
    }
    
private:
    float disc;
};


REGISTER_SELECTION(MinTvQualSelection)



class GeneralDecaySelection: public Selection{
public:
    GeneralDecaySelection(const ptree & cfg, OutputManager &){
        disc = ptree_get<float>(cfg, "disc", .0);
    }
    
    bool operator()(const Event & event){
        static identifier thisProcessFlags("thisProcessFlags");
        Flags tp_flag = event.get<Flags>(thisProcessFlags);
	float prim_flag = tp_flag[GeantPrimary];
	float dec_flag = tp_flag[GeantDecay];
        return ((prim_flag+dec_flag) > disc);
    }
    
private:
    float disc;
};


REGISTER_SELECTION(GeneralDecaySelection)




class ZPosSelection: public Selection{
public:
    ZPosSelection(const ptree & cfg, OutputManager &){
	minz = ptree_get<float>(cfg, "minz", -100.);
	maxz = ptree_get<float>(cfg, "maxz", 100.);
    }
        
    bool operator()(const Event & event){
	static identifier recoPos("recoPos");
	Point position = event.get<Point>(recoPos);
	float vertz = position.z();
	return (minz <= vertz && vertz <= maxz );
    }
	        
private:
    float minz;
    float maxz;
};


REGISTER_SELECTION(ZPosSelection)


class NTrackSelection: public Selection{
public:
    NTrackSelection(const ptree & cfg, OutputManager &){
        minnt = ptree_get<int>(cfg, "minnt", 0);
        maxnt = ptree_get<int>(cfg, "maxnt", 200);
    }
        
    bool operator()(const Event & event){
        ID(numberTracks);
        int numTracks = event.get<int>(numberTracks);
        return (minnt <= numTracks && numTracks <= maxnt );
    }
                
private:
    int minnt;
    int maxnt;
};


REGISTER_SELECTION(NTrackSelection)


class MassSelection: public Selection{
public:
    MassSelection(const ptree & cfg, OutputManager &){
        minm = ptree_get<float>(cfg, "minm", 0.);
        maxm = ptree_get<float>(cfg, "maxm", 200.);
    }
        
    bool operator()(const Event & event){
        ID(p4daughters);
        LorentzVector p4 = event.get<LorentzVector>(p4daughters);
        float mass = p4.mass();
        return (minm < mass && mass < maxm );
    }
                
private:
    float minm;
    float maxm;
};


REGISTER_SELECTION(MassSelection)


class PtSelection: public Selection{
public:
    PtSelection(const ptree & cfg, OutputManager &){
        minpt = ptree_get<float>(cfg, "minpt", 0.);
        maxpt = ptree_get<float>(cfg, "maxpt", 1000.);
    }
        
    bool operator()(const Event & event){
        ID(p4daughters);
        LorentzVector p4 = event.get<LorentzVector>(p4daughters);
        float pt = p4.Pt();
        return (minpt < pt && pt < maxpt );
    }
                
private:
    float minpt;
    float maxpt;
};


REGISTER_SELECTION(PtSelection)


class EtaSelection : public Selection{
public:
    EtaSelection(const ptree & cfg, OutputManager &){
        mineta = ptree_get<float>(cfg, "mineta", -100.);
        maxeta = ptree_get<float>(cfg, "maxeta", 100.);
    }
        
    bool operator()(const Event & event){
        ID(p4daughters);
        LorentzVector p4 = event.get<LorentzVector>(p4daughters);
        float eta = p4.Eta();
        return (mineta < eta && eta < maxeta );
    }
                
private:
    float mineta;
    float maxeta;
};


REGISTER_SELECTION(EtaSelection)


//===========CALCULATE ERRORS FIRST, BUT HAVE ONLY THE ONE FROM SV!!!=============     

// class FLengthSigSelection: public Selection{
// public:
//     FLengthSigSelection(const ptree & cfg, OutputManager &){
//         minnt = ptree_get<float>(cfg, "minnt", 0);
//         maxnt = ptree_get<float>(cfg, "maxnt", 200);
//     }
//         
//     bool operator()(const Event & event){
//         ID(numberTracks);
//         int numTracks = event.get<int>(numberTracks);
//         return (minz <= numTracks && numTracks <= maxz );
//     }
//                 
// private:
//     float minnt;
//     float maxnt;
// };
// 
// 
// REGISTER_SELECTION(FLengthSigSelection)



class ProcessOrSelection: public Selection{
public:
    ProcessOrSelection(const ptree & cfg, OutputManager &){
	std::string catstr = ptree_get<std::string>(cfg, "categories");
	boost::split(catstrings, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
	for(const string & str : catstrings){
	    cats.push_back(strtocat(str));
	}
	discriminator = ptree_get<float>(cfg, "discriminator", .0);
    }
        
    bool operator()(const Event & event){
	ID(thisProcessFlags);
	Flags flags = event.get<Flags>(thisProcessFlags);
	
// 	bool result = false;
	
	for (Category cat : cats)
	{
	    if (flags[cat] > discriminator)
	    {
		return true;
	    }
	}
	
	return false;
    }
	        
private:
    std::vector<std::string> catstrings;
    std::vector<Category> cats;
    float discriminator;
};


REGISTER_SELECTION(ProcessOrSelection)




class ProcessAndSelection: public Selection{
public:
    ProcessAndSelection(const ptree & cfg, OutputManager &){
	discriminator = ptree_get<float>(cfg, "discriminator", .0);
	std::string catstr = ptree_get<std::string>(cfg, "categories");
	boost::split(catstrings, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
	for(const string & str : catstrings){
	    cats.push_back(strtocat(str));
	}
    }
        
    bool operator()(const Event & event){
	ID(thisProcessFlags);
	Flags flags = event.get<Flags>(thisProcessFlags);
	
	float sum = .0;
	
	for (Category cat : cats)
	{
// 	    cout << cattostr(cat) << " ";
	    sum += flags[cat];
	}
	
// 	cout << endl << "Sum: " << sum << endl << endl;
	
	return (sum > discriminator);
    }
	        
private:
    std::vector<std::string> catstrings;
    std::vector<Category> cats;
    float discriminator;
};


REGISTER_SELECTION(ProcessAndSelection)




class DecaySelection: public Selection{
public:
    DecaySelection(const ptree & cfg, OutputManager &){
	catstr = ptree_get<std::string>(cfg, "categories");
	boost::split(catstrings, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
	for(const string & str : catstrings){
	    cats.push_back(newcat::strtocat(str));
	}
    }
        
    bool operator()(const Event & event){
	ID(decayflags);
// 	ID(thisProcessFlags);
// 	Flags flags = event.get<Flags>(thisProcessFlags);
	Flags decflags = event.get<Flags>(decayflags);
	
	double sumcats = .0;
	
	for (newcat::DecayCategory cat : cats)
	    sumcats += decflags[cat];
	
// 	cout << "Sumcats: " << sumcats << endl;
	
	if (sumcats == 0) return false;
	
	bool result = true;
	int equalresult = 0;
	
	for (double flagval : decflags)
	{
	    if (flagval > sumcats)
	    {
// 		cout << catstr << ": X" << endl;
		result = false;
	    } else if (flagval == sumcats) equalresult++;
		
	}
	
// 	cout << catstr << ": O" << endl;
	
	return (result && equalresult < 2);
    }
	        
private:
    std::string catstr;
    std::vector<std::string> catstrings;
    std::vector<newcat::DecayCategory> cats;
};


REGISTER_SELECTION(DecaySelection)





class DecayNotSelection: public Selection{
public:
    DecayNotSelection(const ptree & cfg, OutputManager &){
	std::string catstr = ptree_get<std::string>(cfg, "categories");
	boost::split(catstrings, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
	for(const string & str : catstrings){
	    cats.push_back(strtocat(str));
	}
    }
        
    bool operator()(const Event & event){
	static identifier thisProcessFlags("thisProcessFlags");
	Flags flags = event.get<Flags>(thisProcessFlags);
	
	for (Category cat : cats)
	{
	    if (flags[cat] != 0)
		return false;
	}
	
	return true;
    }
	        
private:
    std::vector<std::string> catstrings;
    std::vector<Category> cats;
};


REGISTER_SELECTION(DecayNotSelection)




class NoDoubleSelection: public Selection{
public:
    NoDoubleSelection(const ptree & cfg, OutputManager &){
	numbersels = cfg.get<int>("numbersels", 2);
	std::string ids = cfg.get<string>("selections");
	vector<string> vids;
	boost::split(vids, ids, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
	for(const string & id : vids){
	    selids.emplace_back(id);
	}
    }
        
    bool operator()(const Event & event){
	
	int truesels = 0;
	
	for(const auto & id : selids){
	    if(event.get<bool>(id)){
		truesels++;
	    }
	}
	return (truesels < numbersels);
    }
	        
private:
    std::vector<identifier> selids;
    int numbersels;
};


REGISTER_SELECTION(NoDoubleSelection)



