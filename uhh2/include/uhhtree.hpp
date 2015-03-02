#ifndef UHHSFRAME_UHHANALYSISMODULE_HPP
#define UHHSFRAME_UHHANALYSISMODULE_HPP

#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "base/include/log.hpp"

#include <boost/property_tree/ptree.hpp>

#include "UHH2/core/include/Event.h"
#include "UHH2/core/include/AnalysisModule.h"
#include "UHH2/core/include/EventHelper.h"

/** \brief Setup the UHH event content from the input tree
 *
 * This class runs a number of UHHAnalysisModules: It calls the UHHAnalysisModule::process(uhh2::Event &) methods
 * 
 * Configuration:
 * \code
 *  type UHHAnalysisModuleRunner
 *  AnalysisModule {
 *      name ExampleModule2 ; C++ classname of uhh2::AnalysisModule
 *      ; optional: additional key/value pairs the AnalysisModule can read vis uhh2::Context::get
 *  }
 * 
 *  read_trigger false  ; default: true
 *  write_outtree false ; default: true
 * 
 *  ; branch names of collections to read to the event container; see
 *  ; member names of uhh2::Event:
 *  pvs offlineSlimmedPrimaryVertices
 *  electrons slimmedElectrons
 *  muons slimmedMuons
 *  taus slimmedTaus
 *  photons slimmedPhotons
 *  jets slimmedJets
 *  topjets patJetsCMSTopTagCHSPacked
 *  MET slimmedMETs
 * 
 *  genInfo 
 *  topjetsgen
 *  genparticles
 *  genjets
 * \endcode
 * 
 * Note that all datasets must have a "type" tag which should be "MC" for Monte-Carlo, as this controls whether
 * to read gen information or not; this corresponds to the \c Type attribute of \c InputData in the sframe xml config.
 * In addition, the dataset tags can specify "version" and "lumi" such that the "dataset_"-keys in context are the same.
 * 
 * NOTE: the current implementation of AnalysisRunner does not support as much as the sframe version. In particular:
 *   - metadata I/O is not supported, i.e. no metadata is read and no metadata is written (one can still run code relying on passing metadata between different modules running in the same job).
 *   - there is not setting corresponding to \c additionalBranches in sframe, which allows to read in arbitrary additional event content via the configuration
 * 
 * Apart from these limitations, most things work, in particular writing out the event tree, as well as creating histograms, where the
 * output tree and the histograms are merged into a single file in case of parallelization.
 */
class AnalysisModuleRunner: public ra::AnalysisModule {
public:
    AnalysisModuleRunner(const ptree & cfg);
    
    virtual void begin_dataset(const ra::s_dataset & dataset, ra::InputManager & in, ra::OutputManager & out) override;
    
    virtual void begin_in_file(const std::string & infile) override;
    
    virtual void process(ra::Event & event) override;
    
    ~AnalysisModuleRunner();
    
private:
    class Context; // the implementation of uhh2::Context
    
    std::shared_ptr<Logger> logger;
    
    bool write_outtree, read_trigger;
    // branch names. Note that run, lumi, event, etc. are hard-coded.
    std::string pvs, electrons, muons, taus, photons, jets, topjets, met, genInfo, gentopjets, genparticles, genjets;
    std::string module_name;
    
    ptree module_cfg;
    
    bool is_mc;
    
    // per-dataset information, in order of initialization:
    ra::Event::Handle<bool> h_stop;
    bool first_event_in_dataset;
    std::unique_ptr<uhh2::GenericEventStructure> ges;
    std::unique_ptr<Context> context;
    std::unique_ptr<uhh2::detail::EventHelper> helper;
    std::unique_ptr<uhh2::AnalysisModule> module;
    std::unique_ptr<uhh2::Event> uevent;
    
    // trigger output handling:
    int last_runid; // last run id for which uhh2::Event has been updated
    int last_runid_out; // last runid for which trigger names have been written to output
    std::map<int, std::vector<std::string>> runid_to_triggernames;
};

#endif
