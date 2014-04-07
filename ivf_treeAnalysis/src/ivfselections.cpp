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
        disc = get<float>(cfg, "disc", 0.);
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
        disc = get<float>(cfg, "disc", 0.);
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

/**
 * implement something like this in ProcessSelection (?)
 * \code
 * if (!hist_filled && (flags[GeantPrimary]+flags[Decay]) > discriminator_)
 * {
 *    process_id = cat_id[4];
 *    hist_filled = true;
 * }
 * \endcode
 * 
*/



class ProcessOrSelection: public Selection{
public:
    ProcessOrSelection(const ptree & cfg, OutputManager &){
	std::string catstr = get<std::string>(cfg, "categories");
	boost::split(catstrings, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
	for(const string & str : catstrings){
	    cats.push_back(strtocat(str));
	}
	discriminator = get<float>(cfg, "discriminator", 0.);
    }
        
    bool operator()(const Event & event){
	static identifier thisProcessFlags("thisProcessFlags");
	Flags flags = event.get<Flags>(thisProcessFlags);
	
	bool result = false;
	
	for (Category cat : cats)
	{
	    if (flags[cat] > discriminator)
	    {
		result = true;
		break;
	    }
	}
	
	return (result);
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
	std::string catstr = get<std::string>(cfg, "categories");
	boost::split(catstrings, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
	for(const string & str : catstrings){
	    cats.push_back(strtocat(str));
	}
	discriminator = get<float>(cfg, "discriminator", 0.);
    }
        
    bool operator()(const Event & event){
	static identifier thisProcessFlags("thisProcessFlags");
	Flags flags = event.get<Flags>(thisProcessFlags);
	
	float sum = 0.;
	float sumgeantflags = 0.;
	float sumdecayflags = 0.;
	static int count = 0;
	bool somethingwrong = false;
	bool geantflaggreaterzero = false;
	bool decayflaggreaterzero = false;
	
	for (unsigned int i = 1; i < GEANT_ARRAY_SIZE; ++i)
	{
	    if (flags[i] > 1) geantflaggreaterzero = true;
	    sumgeantflags += flags[i];
	}
	
	for (unsigned int i = GEANT_ARRAY_SIZE; i < CAT_ARRAY_SIZE; ++i)
	{
	    if (flags[i] > 1) decayflaggreaterzero = true;
	    sumdecayflags += flags[i];
	}
	
	if ((flags[Fake] > 0 && flags[PrimaryDecay] == 0) || geantflaggreaterzero || sumgeantflags < 1)
	{
	    somethingwrong = true;
	    count++;
	    
	    cout << "=====NEW VERTEX=====" << endl;
	    
	    if ()
	    
	
	for (unsigned int i = 0; i < flags.size(); ++i)
	    cout << cattostr(i) << " " << flags[i] << endl;
	
	cout << endl << count << endl << endl;
	
	}
	
	if (decayflaggreaterzero || sumdecayflags < 1)
	{
	    if (!somethingwrong)
		count++;
	cout << "======DECAY DECAY DECAY======" << endl << endl;
	
	for (unsigned int i = 0; i < flags.size(); ++i)
	    cout << cattostr(i) << " " << flags[i] << endl;
	
	cout << endl << count << endl << endl;
	}
	
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