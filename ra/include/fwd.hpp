#ifndef RA_FWD_HPP
#define RA_FWD_HPP

namespace ra{
    // event.hpp:
    class Event;
    
    // context.hpp:
    class Configuration;
    class OutputManager;
    class InputManager;
    
    class TFileOutputManager;
    class TTreeInputManager;
    
    class AnalysisModule;
    
    // identifier:
    class identifier;
    
    struct s_options;
    struct s_dataset;
    struct s_config;
}

// root stuff::
class TH1;
class TFile;
class TTree;
class TBranch;

#endif
