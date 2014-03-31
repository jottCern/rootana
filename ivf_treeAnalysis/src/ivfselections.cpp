#include "ra/include/selections.hpp"
#include "ivftree.hpp"

using namespace ra;
using namespace std;

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



class DecaySelection: public Selection{
public:
    DecaySelection(const ptree & cfg, OutputManager &){
        disc = get<float>(cfg, "disc", 0.);
    }
    
    bool operator()(const Event & event){
        static identifier tvquality("tvquality");
        static identifier thisProcessFlags("thisProcessFlags");
        Flags tp_flag = event.get<Flags>(thisProcessFlags);
	float prim_flag = tp_flag[4];
	float dec_flag = tp_flag[5];
        return ((prim_flag+dec_flag) > disc);
    }
    
private:
    float disc;
};


REGISTER_SELECTION(DecaySelection)

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



// class PtZSelection: public Selection{
// public:
//     PtZSelection(const ptree & cfg, OutputManager &){
//         ptmin = get<double>(cfg, "ptmin", -1.0);
//         ptmax = get<double>(cfg, "ptmax", -1.0);
//     }
//     
//     bool operator()(const Event & event){
//         ID(zp4);
//         double zpt = event.get<LorentzVector>(zp4).pt();
//         return (ptmin < 0 || zpt >= ptmin) && (ptmax < 0 || zpt <= ptmax);
//     }
//     
// private:
//     double ptmin, ptmax;
// };
// 
// 
// REGISTER_SELECTION(PtZSelection)
// 
// 
// 
// class MllSelection: public Selection{
// public:
//     MllSelection(const ptree & cfg, OutputManager &){
//         mllmin = get<double>(cfg, "mllmin", -1.0);
//         mllmax = get<double>(cfg, "mllmax", -1.0);
//     }
//     
//     bool operator()(const Event & event){
//         ID(zp4);
//         double mll = event.get<LorentzVector>(zp4).M();
//         return (mllmin < 0 || mll >= mllmin) && (mllmax < 0 || mll <= mllmax);
//     }
//     
// private:
//     double mllmin, mllmax;
// };
// 
// REGISTER_SELECTION(MllSelection)
