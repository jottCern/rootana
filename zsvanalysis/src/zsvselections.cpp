#include "ra/include/selections.hpp"
#include "zsvtree.hpp"
#include "eventids.hpp"

using namespace ra;
using namespace std;

class NBCandSelection: public Selection{
public:
    NBCandSelection(const ptree & cfg, OutputManager &){
        nmin = ptree_get<int>(cfg, "nmin", -1);
        nmax = ptree_get<int>(cfg, "nmax", -1);
    }
    
    bool operator()(const Event & event){
        const vector<Bcand> & bcands = event.get<vector<Bcand>>(selected_bcands);
        return (nmin < 0 || bcands.size() >= static_cast<size_t>(nmin)) && (nmax < 0 || bcands.size() <= static_cast<size_t>(nmax));
    }
    
private:
    int nmin, nmax;
};


REGISTER_SELECTION(NBCandSelection)



class PtZSelection: public Selection{
public:
    PtZSelection(const ptree & cfg, OutputManager &){
        ptmin = ptree_get<double>(cfg, "ptmin", -1.0);
        ptmax = ptree_get<double>(cfg, "ptmax", -1.0);
    }
    
    bool operator()(const Event & event){
        double zpt = event.get<LorentzVector>(zp4).pt();
        return (ptmin < 0 || zpt >= ptmin) && (ptmax < 0 || zpt <= ptmax);
    }
    
private:
    double ptmin, ptmax;
};


REGISTER_SELECTION(PtZSelection)


class MllSelection: public Selection{
public:
    MllSelection(const ptree & cfg, OutputManager &){
        mllmin = ptree_get<double>(cfg, "mllmin");
        mllmax = ptree_get<double>(cfg, "mllmax");
    }
    
    bool operator()(const Event & event){
        double mll = event.get<LorentzVector>(zp4).M();
        return (mllmin < 0 || mll >= mllmin) && (mllmax < 0 || mll <= mllmax);
    }
    
private:
    double mllmin, mllmax;
};

REGISTER_SELECTION(MllSelection)

class MetSelection: public Selection{
public:
    MetSelection(const ptree & cfg, OutputManager &){
        metmin = ptree_get<float>(cfg, "metmin");
        metmax = ptree_get<float>(cfg, "metmax");
    }
    
    bool operator()(const Event & event){
        float metvalue = event.get<float>(met);
        return (metmin < 0 || metvalue >= metmin) && (metmax < 0 || metvalue <= metmax);
    }
    
private:
    float metmin, metmax;
};


REGISTER_SELECTION(MetSelection)


