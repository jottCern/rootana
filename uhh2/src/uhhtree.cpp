#include "uhhtree.hpp"
#include "ra/include/context.hpp"
#include "ra/include/config.hpp"
#include "base/include/ptree-utils.hpp"

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"

using namespace std;

// NOTE: The code might be a bit confusing as uhh2 and this rootana share common ideas,
// but have (slightly) different interfaces and implementations. For example, ra::Event is almost the same
// as uhh2::GenericEvent. The same is true for uhh2::Context vs. ra::InputManager/ra::OutputManager.
// Therefore, this file does *not* use 'using namespace ra' and 'using namespace uhh2', s.t. it is easier to
// see which class is which.
//
// The data flow when reading an event is the following:
// 1. The ra framework reads into ra::Event
// 2. The AnalysisModuleRunner::Context makes sure that the pointers from ra::Event are the same
//    as the ones in uhh2::GenericEvent (so this is a 'zero-copy synchronization')
// 3. The uhh2::EventHelper class copies the content from the uhh2::GenericEvent to the uhh2::Event for those variables defined
//   in uhh2::Event.
//
// When writing, all that is done in reverse order:
// 1. First, uhh2::EventHelper sychronizes back the changes to uhh2::GenericEvent
// 2. The content of uhh2::GenericEvent and ra::Event are synchronized by the 'zero-copy synchronization' of item 2. above. Hence, this does not need
//    any explicit code(!)
// 3. The ra framework writes the event content from ra::Event

   
class AnalysisModuleRunner::Context: public uhh2::Context {
public:
    Context(uhh2::GenericEventStructure & ges_, ra::InputManager & in_, ra::OutputManager & out_, bool do_output_): uhh2::Context(ges_), in(in_), out(out_), do_output(do_output_) {}
    
    virtual void put(const std::string & path, TH1 * h) override {
        // create the full path: path name + histogram name:
        string fullpath = path;
        if(!fullpath.empty() && fullpath[fullpath.size() - 1] != '/') fullpath += '/';
        fullpath += h->GetName();
        out.put(fullpath.c_str(), h);
    }
    
    virtual void do_declare_event_input(const std::type_info & ti, const std::string & bname, const std::string & mname) override {
        // note: input is 'owned' by ra::Event. So what uhh2::GenericEvent has is an unmanaged copy of it.
        // inform ra framework to read the branch using a ra handle:
        event_input ei(ti);
        in.do_declare_event_input(ti, bname, mname);
        ei.ra_handle = in.get_raw_handle(ti, mname);
        ei.uhh2_handle = ges.get_raw_handle(ti, mname);
        event_inputs.push_back(ei);
    }
    
    // this should be called after the first event has been read (into ra::Event); it synchronizes
    // input to uhh2::Event. It mst only be called once per uhh2::Event, and it must be called
    // after the data in ra::Event have been allocated.
    void synchronize_event_containers(ra::Event & revent, uhh2::Event & uevent){
        for(const auto & ei : event_inputs){
            uhh2::EventAccess_::set_unmanaged(uevent, ei.ti, ei.uhh2_handle, revent.get(ei.ti, ei.ra_handle));
        }
    }
    
    virtual void do_declare_event_output(const std::type_info & ti, const std::string & bname, const std::string & mname) override {
        if(do_output){
            // just forward to OutputManager:
            out.declare_event_output(ti, bname, mname);
        }
    }
    
private:
    ra::InputManager & in;
    ra::OutputManager & out;
    bool do_output;
    
    struct event_input {
        const std::type_info & ti;
        ra::Event::RawHandle ra_handle;
        uhh2::Event::RawHandle uhh2_handle;
        
        explicit event_input(const std::type_info & ti_): ti(ti_){}
    };
    std::vector<event_input> event_inputs;
};


AnalysisModuleRunner::~AnalysisModuleRunner(){}

AnalysisModuleRunner::AnalysisModuleRunner(const ptree & cfg): logger(Logger::get("setup_uhhtree")) {
    write_outtree = ptree_get<bool>(cfg, "write_outtree", true);
    read_trigger = ptree_get<bool>(cfg, "read_trigger", true);
    
    pvs = ptree_get<string>(cfg, "pvs", "");
    electrons = ptree_get<string>(cfg, "electrons", "");
    muons = ptree_get<string>(cfg, "muons", "");
    taus = ptree_get<string>(cfg, "taus", "");
    photons = ptree_get<string>(cfg, "photons", "");
    jets = ptree_get<string>(cfg, "jets", "");
    topjets = ptree_get<string>(cfg, "topjets", "");
    met = ptree_get<string>(cfg, "met", "");
    
    genInfo = ptree_get<string>(cfg, "genInfo", "");
    gentopjets = ptree_get<string>(cfg, "gentopjets", "");
    genparticles = ptree_get<string>(cfg, "genparticles", "");
    genjets = ptree_get<string>(cfg, "genjets", "");
    
    module_cfg = cfg.get_child("AnalysisModule");
    
    module_name = ptree_get<string>(module_cfg, "name");
}

void AnalysisModuleRunner::begin_dataset(const ra::s_dataset & dataset, ra::InputManager & in, ra::OutputManager & out){
    h_stop = in.get_handle<bool>("stop");
    
    first_event_in_dataset = true;
    
    ges.reset(new uhh2::GenericEventStructure());
    context.reset(new Context(*ges, in, out, write_outtree));
    for(const auto & kv : dataset.tags.keyval){
        context->set("dataset_" + kv.first, kv.second);
    }
    for(const auto & kv : module_cfg){
        context->set(kv.first, kv.second.data());
    }
    
    helper.reset(new uhh2::detail::EventHelper(*context));
    
    if(!pvs.empty()) helper->setup_pvs(pvs);
    if(!electrons.empty()) helper->setup_electrons(electrons);
    if(!muons.empty()) helper->setup_muons(muons);
    if(!taus.empty()) helper->setup_taus(taus);
    if(!photons.empty()) helper->setup_photons(photons);
    if(!jets.empty()) helper->setup_jets(jets);
    if(!topjets.empty()) helper->setup_topjets(topjets);
    if(!met.empty()) helper->setup_met(met);
    
    is_mc = context->get("dataset_type") == "MC";
    
    if(is_mc){
        if(!genInfo.empty()) helper->setup_genInfo(genInfo);
        if(!gentopjets.empty()) helper->setup_gentopjets(gentopjets);
        if(!genjets.empty()) helper->setup_genjets(genparticles);
        if(!genparticles.empty()) helper->setup_genparticles(genparticles);
    }
    
    if(read_trigger){
        helper->setup_trigger();
    }
    module = uhh2::AnalysisModuleRegistry::build(module_name, *context);
    
    last_runid = last_runid_out = -1;
    uevent.reset(new uhh2::Event(*ges));
    helper->set_event(uevent.get());
}

void AnalysisModuleRunner::begin_in_file(const string & infile){
    std::unique_ptr<TFile> file(TFile::Open(infile.c_str(), "read"));
    if(read_trigger){
        std::map<int, std::vector<std::string>> runid_to_triggernames;
        // find triggernames for all runs in this file:
        TTree * intree = dynamic_cast<TTree*>(file->Get("AnalysisTree"));
        if(!intree) throw runtime_error("Did not find AnalysisTree in input file");
        std::vector<std::string> trigger_names;
        std::vector<std::string> *ptrigger_names = &trigger_names;
        int runid = 0;
        TBranch* runb = 0;
        TBranch* tnb = 0;
        intree->SetBranchAddress("run", &runid, &runb);
        intree->SetBranchAddress("triggerNames", &ptrigger_names, &tnb);
        if(runb==0 || tnb==0){
            throw runtime_error("did not find branches for setting up trigger names");
        }
        int nentries = intree->GetEntries();
        int last_runid = -1;
        for(int i=0; i<nentries; ++i){
            runb->GetEntry(i);
            if(last_runid == runid) continue;
            last_runid = runid;
            assert(runid >= 1);
            tnb->GetEntry(i);
            if(!trigger_names.empty()){
                runid_to_triggernames[runid] = trigger_names;
            }
        }
        LOG_INFO("Found " << runid_to_triggernames.size() << " trigger infos in file '" << infile << "'");
        helper->set_infile_triggernames(move(runid_to_triggernames));
    }
    
}

void AnalysisModuleRunner::process(ra::Event & revent){
    if(first_event_in_dataset){
        context->synchronize_event_containers(revent, *uevent);
    }
    helper->event_read(); // reads from uevent handles into uevent members
    bool keep = module->process(*uevent);
    if(!keep){
        revent.set(h_stop, true);
    }
    else{
        helper->event_write();
    }
}

REGISTER_ANALYSIS_MODULE(AnalysisModuleRunner)
