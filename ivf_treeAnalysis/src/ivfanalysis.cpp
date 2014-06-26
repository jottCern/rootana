#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"

#include "ivftree.hpp"
#include "ivfutils.hpp"



using namespace ivf;
using namespace ra;
using namespace std;




class SetProperFlags: public AnalysisModule{
public:
    SetProperFlags(const ptree & cfg) {	
   
    }
    
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out) {}
    
    virtual void process(Event & event){
	ID(thisProcessFlags);
	ID(geantflags);
	ID(decayflags);
	
	Flags flags = event.get<Flags>(thisProcessFlags);
	
	Flags new_geantflags;
	Flags new_decayflags;
	
	double sumgeantdecay = 0.;
	double sumalldecay = 0.;
	
	
	for (int i = 1; i < GEANT_ARRAY_SIZE; ++i)
	{
	    new_geantflags.push_back(flags[i]);
	    
	    if (i == 4 || i == 5)
		sumgeantdecay += flags[i];
	}
	
	for (int i = GEANT_ARRAY_SIZE; i < CAT_ARRAY_SIZE; ++i)
	{
	    if (i != 19)
	    {
		new_decayflags.push_back(flags[i]);
		sumalldecay += flags[i];
	    }
	}
	
	if (flags[18] != 0 && sumalldecay < sumgeantdecay-0.00001)
	{
	    new_decayflags[0] += (sumgeantdecay-sumalldecay);
// 	    cout << endl << "+++++++PRIMARY DECAY+++++++++";
	}
	
// 	if (sumalldecay > sumgeantdecay+0.00001)
// 	{
// 	    cout << "++++SUM DECAY > SUM GEANT DECAY+++++++" << endl;
// 	    double reweight = sumgeantdecay/sumalldecay;
// 	    for (int i = 0; i < new_decayflags.size(); ++i)
// 		new_decayflags[i] = new_decayflags[i]*reweight;
// 	    
// 	    cout << "   Old Flags:";
// 	    for (double val : flags) cout << " " << val;
// 	    
// 	    cout << endl << "   Geant Flags:";
// 	    for (double val : new_geantflags) cout << " " << val;
// 	    
// 	    cout << endl << "   Decay Flags:";
// 	    for (double i : new_geantflags) cout << "  ";
// 	    for (double val : new_decayflags) cout << " " << val;
// 	    
// 	    cout << endl << endl;
// 	}
	
	
	
	event.set<Flags>(geantflags, new_geantflags);
	event.set<Flags>(decayflags, new_decayflags);
	
        
    }
    
private:

};

REGISTER_ANALYSIS_MODULE(SetProperFlags)