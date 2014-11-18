#include "ra/include/analysis.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/event.hpp"
#include "ra/include/utils.hpp"
#include "ra/include/root-utils.hpp"

#include "zsvtree.hpp"
#include "TH1D.h"
#include "TH2F.h"
#include "TFile.h"

using namespace ra;
using namespace std;

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
    unique_ptr<TH2> sfs;
    
    string filename, histname;
    
    Event::Handle<double> h_weight, h_elesf;
    Event::Handle<lepton> h_lepton_minus, h_lepton_plus;
};

elesf::elesf(const ptree & cfg){
    string filename = ptree_get<string>(cfg, "filename", "electrons_scale_factors.root"); // default is for MVA
    string histname = ptree_get<string>(cfg, "histname", "electronsDATAMCratio_FO_ID_ISO");
    filename = resolve_file(filename);
    if(filename.empty()) throw runtime_error("could not find electrons scale factor file");
    TFile f(filename.c_str(), "read");
    sfs = gethisto<TH2>(f, histname);
    sfs->SetDirectory(0);
    // leaving here closes f, disposes histo
}

void elesf::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    is_real_data = dataset.tags.get<bool>("is_real_data");
    h_weight = in.get_handle<double>("weight");
    h_elesf = in.get_handle<double>("elesf");
    h_lepton_plus = in.get_handle<lepton>("lepton_plus");
    h_lepton_minus = in.get_handle<lepton>("lepton_minus");
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
    const auto & lp = event.get<lepton>(h_lepton_plus);
    const auto & lm = event.get<lepton>(h_lepton_minus);
    
    double sf = 1.0;
    if(abs(lp.pdgid)==11){
        sf *= getsf(lp);
    }
    if(abs(lm.pdgid)==11){
        sf *= getsf(lm);
    }
    event.set(h_elesf, sf);
    event.get<double>(h_weight) *= sf;
}

REGISTER_ANALYSIS_MODULE(elesf)
