#include "ra/include/selections.hpp"
#include "zsvtree.hpp"
#include "eventids.hpp"

using namespace ra;
using namespace std;
using namespace zsv;

class NBCandSelection: public Selection{
public:
    NBCandSelection(const ptree & cfg, OutputManager &){
        input = ptree_get<string>(cfg, "input", "selected_bcands");
        nmin = ptree_get<int>(cfg, "nmin", -1);
        nmax = ptree_get<int>(cfg, "nmax", -1);
    }
    
    bool operator()(const Event & event){
        const vector<Bcand> & bcands = event.get<vector<Bcand>>(input);
        return (nmin < 0 || bcands.size() >= static_cast<size_t>(nmin)) && (nmax < 0 || bcands.size() <= static_cast<size_t>(nmax));
    }
    
private:
    int nmin, nmax;
    identifier input;
};


REGISTER_SELECTION(NBCandSelection)


class NJetSelection: public Selection{
public:
    NJetSelection(const ptree & cfg, OutputManager &){
        nmin = ptree_get<int>(cfg, "nmin", -1);
        nmax = ptree_get<int>(cfg, "nmax", -1);
        input = ptree_get<string>(cfg, "input");
    }
    
    bool operator()(const Event & event){
        const vector<jet> & jets = event.get<vector<jet>>(input);
        return (nmin < 0 || jets.size() >= static_cast<size_t>(nmin)) && (nmax < 0 || jets.size() <= static_cast<size_t>(nmax));
    }
    
private:
    int nmin, nmax;
    identifier input;
};


REGISTER_SELECTION(NJetSelection)


class PtZSelection: public Selection{
public:
    PtZSelection(const ptree & cfg, OutputManager &){
        ptmin = ptree_get<double>(cfg, "ptmin", -1.0);
        ptmax = ptree_get<double>(cfg, "ptmax", -1.0);
    }
    
    bool operator()(const Event & event){
        double zpt = event.get<LorentzVector>(id::zp4).pt();
        return (ptmin < 0 || zpt >= ptmin) && (ptmax < 0 || zpt <= ptmax);
    }
    
private:
    double ptmin, ptmax;
};


REGISTER_SELECTION(PtZSelection)


class MllSelection: public Selection{
public:
    MllSelection(const ptree & cfg, OutputManager &){
        mllmin = ptree_get<double>(cfg, "mllmin", -1.0);
        mllmax = ptree_get<double>(cfg, "mllmax", -1.0);
    }
    
    bool operator()(const Event & event){
        double mll = event.get<LorentzVector>(id::zp4).M();
        return (mllmin < 0 || mll >= mllmin) && (mllmax < 0 || mll <= mllmax);
    }
    
private:
    double mllmin, mllmax;
};

REGISTER_SELECTION(MllSelection)

class MetSelection: public Selection{
public:
    MetSelection(const ptree & cfg, OutputManager &){
        metmin = ptree_get<float>(cfg, "metmin", -1.0);
        metmax = ptree_get<float>(cfg, "metmax", -1.0);
    }
    
    bool operator()(const Event & event){
        float metvalue = event.get<float>(id::met);
        return (metmin < 0 || metvalue >= metmin) && (metmax < 0 || metvalue <= metmax);
    }
    
private:
    float metmin, metmax;
};


REGISTER_SELECTION(MetSelection)

class LeptonFlavorSelection: public Selection{
public:
    LeptonFlavorSelection(const ptree & cfg, OutputManager &){
        string sf = ptree_get<string>(cfg, "select_flavor");
        if(sf=="ee"){
            select_flavor = ee;
        }
        else if(sf=="mm"){
            select_flavor=mm;
        }
        else if (sf=="me"){
            select_flavor = me;
        }
        else{
            throw runtime_error("invalid select_flavor='" + sf + "' (allowed: ee, mm, me)");
        }
    }
    
    bool operator()(const Event & event){
        const auto & lp = event.get<lepton>(id::lepton_plus);
        const auto & lm = event.get<lepton>(id::lepton_minus);
        int flsum = abs(lp.pdgid) + abs(lm.pdgid);
        return (select_flavor == ee && flsum == 22) || (select_flavor == mm && flsum == 26) || (select_flavor == me && flsum == 24);
    }
    
private:
    enum e_select_flavor {ee, mm, me };
    e_select_flavor select_flavor;
};


REGISTER_SELECTION(LeptonFlavorSelection)
