#include "zsvanalysis/MuScleFitCorrector_v4_3/MuScleFitCorrector.h"
#include "ra/include/analysis.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/event.hpp"
#include "ra/include/utils.hpp"

#include "eventids.hpp"
#include "zsvtree.hpp"
#include "TH1D.h"

using namespace ra;
using namespace std;

class muscale: public AnalysisModule {
public:
    
    explicit muscale(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    void correct_lepton(lepton & lep);
    
    bool is_real_data;
    std::unique_ptr<MuScleFitCorrector> corr;
    
    TH1D * muscale_control;
};

muscale::muscale(const ptree & cfg){
}

void muscale::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    is_real_data = dataset.tags.get<bool>("is_real_data");
    string filename;
    if(is_real_data){
        string run = dataset.tags.get<string>("run");
        if(run=="2012A" || run=="2012B" || run=="2012C"){
            filename = resolve_file("zsvanalysis/MuScleFitCorrector_v4_3/MuScleFit_2012ABC_DATA_ReReco_53X.txt");
        }
        else if(run=="2012D"){
            filename = resolve_file("zsvanalysis/MuScleFitCorrector_v4_3/MuScleFit_2012D_DATA_ReReco_53X.txt");
        }
    }
    else{
        filename = resolve_file("zsvanalysis/MuScleFitCorrector_v4_3/MuScleFit_2012_MC_53X_smearReReco.txt");
    }
    if(filename.empty()){
        throw runtime_error("could not find mu scale fit txt file!");
    }
    corr.reset(new MuScleFitCorrector(filename.c_str()));
    muscale_control = new TH1D("muscale_control", "muscale_control", 100, 0.9, 1.1);
    out.put("muscale_control", muscale_control);
}

void muscale::correct_lepton(lepton & lep){
    // convert to TLorentzVector and apply scale/smear to it:
    TLorentzVector lp4(lep.p4.px(), lep.p4.py(), lep.p4.pz(), lep.p4.E());
    int charge = lep.pdgid < 0 ? +1 : -1;
    corr->applyPtCorrection(lp4, charge);
    if(!is_real_data){
        corr->applyPtSmearing(lp4, charge, true);
    }
    muscale_control->Fill(lp4.Pt() / lep.p4.pt());
    // now set lepton p4 from TLorentzVector again ...
    lep.p4.SetPxPyPzE(lp4.Px(), lp4.Py(), lp4.Pz(), lp4.E());
}

void muscale::process(Event & event){
    correct_lepton(event.get<lepton>(lepton_plus));
    correct_lepton(event.get<lepton>(lepton_minus));
}

REGISTER_ANALYSIS_MODULE(muscale)
