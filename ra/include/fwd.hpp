#ifndef RA_FWD_HPP
#define RA_FWD_HPP

#include <boost/property_tree/ptree_fwd.hpp>

namespace ra{
    // event.hpp:
    class Event;
    
    // context.hpp:
    class Configuration;
    class OutputManager;
    class OutputManagerBackend;
    class OutputManagerOperations;
    class InputManager;
    class InputManagerBackend;
    
    class AnalysisModule;
    
    class identifier;
    
    struct s_options;
    struct s_dataset;
    struct s_config;
}

using boost::property_tree::ptree;

// root stuff::
class TH1;
class TFile;
class TTree;
class TBranch;

#endif
