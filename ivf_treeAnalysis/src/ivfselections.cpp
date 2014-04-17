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
        disc = get<float>(cfg, "disc", .0);
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
        disc = get<float>(cfg, "disc", .0);
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
	minz = get<float>(cfg, "minz", -100.);
	maxz = get<float>(cfg, "maxz", 100.);
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




class ProcessOrSelection: public Selection{
public:
    ProcessOrSelection(const ptree & cfg, OutputManager &){
	std::string catstr = get<std::string>(cfg, "categories");
	boost::split(catstrings, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
	for(const string & str : catstrings){
	    cats.push_back(strtocat(str));
	}
	discriminator = get<float>(cfg, "discriminator", .0);
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
	discriminator = get<float>(cfg, "discriminator", .0);
	std::string catstr = get<std::string>(cfg, "categories");
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
	catstr = get<std::string>(cfg, "categories");
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
	std::string catstr = get<std::string>(cfg, "categories");
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



