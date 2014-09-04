#include "ra/include/selections.hpp"
#include "ra/include/context.hpp"
#include "zsvtree.hpp"

using namespace ra;
using namespace std;

class NBCandSelection: public Selection{
public:
    NBCandSelection(const ptree & cfg, InputManager & in, OutputManager &){
        h_input = in.get_handle<vector<Bcand>>(ptree_get<string>(cfg, "input", "selected_bcands"));
        nmin = ptree_get<int>(cfg, "nmin", -1);
        nmax = ptree_get<int>(cfg, "nmax", -1);
    }
    
    bool operator()(const Event & event){
        const vector<Bcand> & bcands = event.get<vector<Bcand>>(h_input);
        return (nmin < 0 || bcands.size() >= static_cast<size_t>(nmin)) && (nmax < 0 || bcands.size() <= static_cast<size_t>(nmax));
    }
    
private:
    int nmin, nmax;
    Event::Handle<vector<Bcand>> h_input;
};


REGISTER_SELECTION(NBCandSelection)


class NJetSelection: public Selection{
public:
    NJetSelection(const ptree & cfg, InputManager & in, OutputManager &){
        nmin = ptree_get<int>(cfg, "nmin", -1);
        nmax = ptree_get<int>(cfg, "nmax", -1);
        h_input = in.get_handle<vector<jet>>(ptree_get<string>(cfg, "input"));
    }
    
    bool operator()(const Event & event){
        const vector<jet> & jets = event.get<vector<jet>>(h_input);
        return (nmin < 0 || jets.size() >= static_cast<size_t>(nmin)) && (nmax < 0 || jets.size() <= static_cast<size_t>(nmax));
    }
    
private:
    int nmin, nmax;
    Event::Handle<vector<jet>> h_input;
};


REGISTER_SELECTION(NJetSelection)


class PtZSelection: public Selection{
public:
    PtZSelection(const ptree & cfg, InputManager & in, OutputManager &){
        ptmin = ptree_get<double>(cfg, "ptmin", -1.0);
        ptmax = ptree_get<double>(cfg, "ptmax", -1.0);
        h_zp4 = in.get_handle<LorentzVector>("zp4");
    }
    
    bool operator()(const Event & event){
        double zpt = event.get<LorentzVector>(h_zp4).pt();
        return (ptmin < 0 || zpt >= ptmin) && (ptmax < 0 || zpt <= ptmax);
    }
    
private:
    double ptmin, ptmax;
    Event::Handle<LorentzVector> h_zp4;
};


REGISTER_SELECTION(PtZSelection)


class MllSelection: public Selection{
public:
    MllSelection(const ptree & cfg, InputManager & in, OutputManager &){
        mllmin = ptree_get<double>(cfg, "mllmin", -1.0);
        mllmax = ptree_get<double>(cfg, "mllmax", -1.0);
        h_zp4 = in.get_handle<LorentzVector>("zp4");
    }
    
    bool operator()(const Event & event){
        double mll = event.get<LorentzVector>(h_zp4).M();
        return (mllmin < 0 || mll >= mllmin) && (mllmax < 0 || mll <= mllmax);
    }
    
private:
    double mllmin, mllmax;
    Event::Handle<LorentzVector> h_zp4;
};

REGISTER_SELECTION(MllSelection)

class MetSelection: public Selection{
public:
    MetSelection(const ptree & cfg, InputManager & in, OutputManager &){
        metmin = ptree_get<float>(cfg, "metmin", -1.0);
        metmax = ptree_get<float>(cfg, "metmax", -1.0);
        h_met = in.get_handle<float>("met");
    }
    
    bool operator()(const Event & event){
        float metvalue = event.get<float>(h_met);
        return (metmin < 0 || metvalue >= metmin) && (metmax < 0 || metvalue <= metmax);
    }
    
private:
    float metmin, metmax;
    Event::Handle<float> h_met;
};


REGISTER_SELECTION(MetSelection)

class LeptonFlavorSelection: public Selection{
public:
    LeptonFlavorSelection(const ptree & cfg, InputManager & in, OutputManager &){
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
        
        h_lepton_minus = in.get_handle<lepton>("lepton_minus");
        h_lepton_plus = in.get_handle<lepton>("lepton_plus");
    }
    
    bool operator()(const Event & event){
        const auto & lp = event.get<lepton>(h_lepton_plus);
        const auto & lm = event.get<lepton>(h_lepton_minus);
        int flsum = abs(lp.pdgid) + abs(lm.pdgid);
        return (select_flavor == ee && flsum == 22) || (select_flavor == mm && flsum == 26) || (select_flavor == me && flsum == 24);
    }
    
private:
    enum e_select_flavor {ee, mm, me };
    e_select_flavor select_flavor;
    
    Event::Handle<lepton> h_lepton_plus, h_lepton_minus;
};


REGISTER_SELECTION(LeptonFlavorSelection)
