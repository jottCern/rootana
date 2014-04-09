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
	
// 	------DEBUGGING STUFF-------
// 	
// 	double sumgeantflags = 0.;
// 	double sumdecayflags = 0.;
// 	static int count = 0;
// 	bool somethingwrong = false;
// 	bool geantflaggreater1 = false;
// 	bool decayflaggreater1 = false;
// 	
// 	static int countgeantflaglower1 = 0;
// 	static int countfake = 0;
// 	static int countgeantflaggreater1 = 0;
// 	static int countdecayflaggreater1 = 0;
// 	static int countsumdecaylowersumgeant = 0;
// 	
// 	for (unsigned int i = 1; i < GEANT_ARRAY_SIZE; ++i)
// 	{
// 	    if (flags[i] > 1) geantflaggreater1 = true;
// 	    sumgeantflags += flags[i];
// 	}
// 	
// 	for (unsigned int i = GEANT_ARRAY_SIZE; i < CAT_ARRAY_SIZE; ++i)
// 	{
// 	    if (flags[i] > 1) decayflaggreater1 = true;
// 	    sumdecayflags += flags[i];
// 	}
// 	
// 	 if (sumgeantflags < 1)
// 	     countgeantflaglower1++;
// 	
// 	if ((flags[Fake] > 0 && flags[PrimaryDecay] == 0) || geantflaggreater1)
// 	{
// 	    somethingwrong = true;
// 	    count++;
// 	    
// 	    cout << "=====NEW VERTEX=====" << endl;
// 	    
// 	
// 	    for (unsigned int i = 0; i < flags.size(); ++i)
// 		cout << cattostr(i) << " " << flags[i] << endl;
// 	    
// 	    if (flags[Fake] > 0 && flags[PrimaryDecay] == 0)
// 	    {
// 		cout << "   ---Fake!" << endl << endl;
// 		countfake++;
// 	    }
// 	    
// 	    if (geantflaggreater1)
// 	    {
// 		cout << "   ---Geant Flag greater 1!" << endl << endl;
// 		countgeantflaggreater1++;
// 	    }
// 	    
// 	    cout << endl << "Fakes | GeantFlag > 1 | Sum(GeantFlag)<1 | DecayFlag > 1 | Sum(DecayFlag)<Sum(GeantFlag) : " << countfake << " " << countgeantflaggreater1 << " " << countgeantflaglower1 << " " << countdecayflaggreater1 << " " << countsumdecaylowersumgeant << endl << endl;
// 	
// 	    cout << endl << "ALL instances: " << count << endl << endl;
// 	
// 	}
// 	
// 	if (decayflaggreater1 || (-(sumdecayflags-((double)flags[GeantPrimary]+flags[GeantDecay])) > 0.00001 && flags[PrimaryDecay] == 0))
// 	{
// 	    if (!somethingwrong)
// 	    {
// 		count++;
// 		cout << "=====NEW VERTEX=====" << endl;
// 	
// 		for (unsigned int i = 0; i < flags.size(); ++i)
// 		    cout << cattostr(i) << " " << flags[i] << endl;
// 		
// 	    }
// 	    
// 	    if (decayflaggreater1)
// 	    {
// 		cout << "   ---Decay Flag greater 1!" << endl << endl;
// 		countdecayflaggreater1++;
// 	    }
// 	    
// 	    if (-(sumdecayflags-((double)flags[GeantPrimary]+flags[GeantDecay])) > 0.00001 && flags[PrimaryDecay] == 0)
// 	    {
// 		cout << "   ---Sum of Decay Flags smaller than sum of Geant Flags! " << sumdecayflags << " " << (flags[GeantPrimary]+flags[GeantDecay]) << endl << endl;
// 		countsumdecaylowersumgeant++;
// 	    }
// 	    
// 	    cout << endl << "Fakes | GeantFlag > 1 | Sum(GeantFlag)<1 | DecayFlag > 1 | Sum(DecayFlag)<Sum(GeantFlag) : " << countfake << " " << countgeantflaggreater1 << " " << countgeantflaglower1 << " " << countdecayflaggreater1 << " " << countsumdecaylowersumgeant << endl << endl;
// 	    
// 	    cout << endl << "ALL instances: " << count << endl << endl;
// 	
// 	
// 	}
// 	
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



