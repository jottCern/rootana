#include "zsvanalysis/MuScleFitCorrector_v4_3/MuScleFitCorrector.h"
#include "ra/include/analysis.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/event.hpp"
#include "ra/include/utils.hpp"
#include "ra/include/root-utils.hpp"

#include "eventids.hpp"
#include "zsvtree.hpp"
#include "TH1D.h"
#include "TH2F.h"
#include "TFile.h"

using namespace ra;
using namespace std;
using namespace zsv;


// apply electron data/MC efficiency correction factors for tight MVA ID, see
// https://twiki.cern.ch/twiki/bin/view/CMS/MultivariateElectronIdentification#Triggering_MVA
//
// works for events with any number of electrons.
// TODO: trigger efficiency currently missing, but is not documented on twiki(?!)
class elesf: public AnalysisModule {
public:
    
    explicit elesf(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    double getsf(const lepton & lep);
    
    bool is_real_data;
    unique_ptr<TH2F> sfs;
};

elesf::elesf(const ptree & cfg){
    string filename = resolve_file("electrons_scale_factors.root");
    if(filename.empty()) throw runtime_error("could not find electrons scale factor file");
    TFile f(filename.c_str(), "read");
    sfs = gethisto<TH2F>(f, "electronsDATAMCratio_FO_ID_ISO");
    sfs->SetDirectory(0);
    // leaving here closes f, disposes histo
}

void elesf::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    is_real_data = dataset.tags.get<bool>("is_real_data");
}

double elesf::getsf(const lepton & lep){
    assert(isfinite(lep.sc_eta)); // to catch accidental muons or other problems
    int ibin_x = sfs->GetXaxis()->FindBin(fabs(lep.sc_eta));
    if(ibin_x < 1 || ibin_x > sfs->GetXaxis()->GetNbins()){
        cout << "sc_eta=" << lep.sc_eta << endl;
        throw runtime_error("x bin (eta) outside of histo range for ele sf");
    }
    int ibin_y = sfs->GetYaxis()->FindBin(lep.p4.pt());
    if(ibin_y > sfs->GetYaxis()->GetNbins()){
        ibin_y = sfs->GetYaxis()->GetNbins();
    }
    return sfs->GetBinContent(ibin_x, ibin_y);
}

void elesf::process(Event & event){
    if(is_real_data){
        return;
    }
    const auto & lp = event.get<lepton>(id::lepton_plus);
    const auto & lm = event.get<lepton>(id::lepton_minus);
    
    float sf = 1.0f;
    // x-axis is eta:
    if(abs(lp.pdgid)==11){
        sf *= getsf(lp);
    }
    if(abs(lm.pdgid)==11){
        sf *= getsf(lm);
    }
    event.set(id::elesf, sf);
    event.get<double>(fwid::weight) *= sf;
}

REGISTER_ANALYSIS_MODULE(elesf)
