#include "ra/include/analysis.hpp"
#include "ra/include/root-utils.hpp"
#include "ra/include/utils.hpp"
#include "ra/include/context.hpp"
#include "ra/include/config.hpp"

#include "eventids.hpp"

#include "TH1D.h"

#include <boost/algorithm/string.hpp>

using namespace ra;
using namespace std;
using namespace zsv;

/** \brief Pileup reweighting module
 * 
 * Reweights Monte-Carlo based on its generated pileup distribution and a 'target' pileup
 * distribution (of data).
 * 
 * Configuration:
 * \code
 * type pileup_reweight
 * target /path/to/file:data_pileup_histogram
 * strict false ; optional, default is true
 * \endcode
 * 
 * The generated pileup distribution for each sample is given in its dataset definition as tag "pileup". The only valid value
 * so far is "Summer12_S10". If for a MC sample, no "pileup" tag is given, an exception is thrown, unless \c strict is set to false. An
 * exception is also thrown if the value of the "pileup" tag is unknown for a dataset.
 * 
 * see also: https://twiki.cern.ch/twiki/bin/view/CMS/PileupMCReweightingUtilities
 */
class pileup_reweight: public AnalysisModule {
public:
    
    explicit pileup_reweight(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    
    bool strict;
    std::unique_ptr<TH1D> target;
    std::unique_ptr<TH1D> weights;
    
    TH1D * hpileup_reweight;
};

namespace {

// pileup distribution from: https://twiki.cern.ch/twiki/bin/view/CMS/Pileup_MC_Gen_Scenarios
double Summer12_S10[60] = {
                         2.560E-06,  5.239E-06, 1.420E-05, 5.005E-05, 1.001E-04, 2.705E-04, 1.999E-03, 6.097E-03, 1.046E-02,
                         1.383E-02,  1.685E-02, 2.055E-02, 2.572E-02, 3.262E-02, 4.121E-02, 4.977E-02, 5.539E-02, 5.725E-02,
                         5.607E-02,  5.312E-02, 5.008E-02, 4.763E-02, 4.558E-02, 4.363E-02, 4.159E-02, 3.933E-02, 3.681E-02,
                         3.406E-02,  3.116E-02, 2.818E-02, 2.519E-02, 2.226E-02, 1.946E-02, 1.682E-02, 1.437E-02, 1.215E-02,
                         1.016E-02,  8.400E-03, 6.873E-03, 5.564E-03, 4.457E-03, 3.533E-03, 2.772E-03, 2.154E-03, 1.656E-03,
                         1.261E-03,  9.513E-04, 7.107E-04, 5.259E-04, 3.856E-04, 2.801E-04, 2.017E-04, 1.439E-04, 1.017E-04,
                         7.126E-05,  4.948E-05, 3.405E-05, 2.322E-05, 1.570E-05, 5.005E-06 };
}



pileup_reweight::pileup_reweight(const ptree & cfg){
    strict = ptree_get<bool>(cfg, "strict", true);
    string starget = ptree_get<string>(cfg, "target");
    vector<string> filename_hname;
    boost::split(filename_hname, starget, [](char c){return c == ':';});
    if(filename_hname.size() != 2){
        throw runtime_error("invalid setting for target (must have form filename:histoname).");
    }
    string filename = resolve_file(filename_hname[0]);
    if(filename==""){
        throw runtime_error("could not resolve file '" + filename_hname[0] + "'");
    }
    TFile infile(filename.c_str(), "read");
    if(!infile.IsOpen()){
        throw runtime_error("could not open file '" + filename + "'");
    }
    target = gethisto<TH1D>(infile, filename_hname[1]);
}

void pileup_reweight::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    weights.reset(0);
    hpileup_reweight = new TH1D("pileup_reweight", "pileup_reweight", 200, 0, 5);
    out.put("pileup_reweight", hpileup_reweight);
    bool is_real_data = dataset.tags.get<bool>("is_real_data");
    if(is_real_data) return;
    string pileup = dataset.tags.get<string>("pileup", "");
    if(pileup.empty()){
        if(strict){
            throw runtime_error("no 'pileup' tag given for dataset '" + dataset.name + "'");
        }
        return; // and do not reweight ...
    }
    // now fill the 'weights' histogram. In general, this depends on the value of pileup, but so far we only have one ...
    if(pileup=="Summer12_S10"){
        // rebin the target histogram to binsize = 1, as this is what we have in Summer12_S10:
        std::unique_ptr<TH1D> target_rebinned((TH1D*)target->Clone()); 
        int rebin_factor = target->GetNbinsX() / target->GetXaxis()->GetXmax();
        target_rebinned->Rebin(rebin_factor);
        assert(target_rebinned->GetNbinsX() == target_rebinned->GetXaxis()->GetXmax());
        assert(target_rebinned->GetXaxis()->GetXmin() == 0);
        target_rebinned->Scale(1.0 / target_rebinned->Integral());
        int imax = min(60, target_rebinned->GetNbinsX());
        
        // now set the weights histogram:
        weights.reset(new TH1D("w", "w", imax, 0, imax));
        for(int i=0; i<imax; ++i){
            weights->SetBinContent(i+1, target_rebinned->GetBinContent(i+1) / Summer12_S10[i]);
        }
    }
    else{
        throw runtime_error("unknown pileup scenario '" + pileup + "'");
    }
}

void pileup_reweight::process(Event & event){
    if(!weights) return;
    float tp = event.get<float>(id::mc_true_pileup);
    int ibin = weights->FindBin(tp);
    float weight = weights->GetBinContent(ibin);
    hpileup_reweight->Fill(weight);
    event.set_weight(event.weight() * weight);
}

REGISTER_ANALYSIS_MODULE(pileup_reweight)
