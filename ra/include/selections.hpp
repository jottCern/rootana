#ifndef SELECTIONS_HPP
#define SELECTIONS_HPP

#include "event.hpp"
#include "analysis.hpp"
#include "config.hpp"
#include "base/include/registry.hpp"

#include "TH1D.h"
#include <string>

namespace ra{

class Selection{
public:
    // derived classes must implement a constructor with the arguments
    // const ptree & cfg, OutputContext & out
    
    // true = event passes the selection
    virtual bool operator()(const Event & e) = 0;
    virtual ~Selection(){}
};


typedef ::Registry<Selection, std::string, const ptree &, OutputManager &> SelectionRegistry;
#define REGISTER_SELECTION(T) namespace { int dummy##T = ::ra::SelectionRegistry::register_<T>(#T); }



/** \brief AnalysisModule running a given set of Selection modules
 *
 * Example configuration:
 * \code
 * sels {
 *   type Selections
 *   all { ; user-defined name
 *       type PassallSelection ; Selection class name
 *   }
 *   bcand2 { ; user-defined name
 *       type NBCandSelection
 *       nmin 2
 *       nmax 2
 *   }
 *   final_selection {
 *      type AndSelection
 *      selections "all bcand2"
 *   }
 * }
 * \endcode
 * 
 * The configuration contains settings for \c Selection modules; see the respective \c Selection classes
 * for a description.
 * 
 * The module runs all configured \c Selections in the order given in the configuration (from top to bottom)
 * on the event and saves the result as a boolean value of the user-defined name ("all", "bcand2", and "final_selection"
 * in the above example). Note that order is important if selections refer to each other as for the \c AndSelection.
 */
class Selections: public AnalysisModule{
public:
    Selections(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    ptree cfg;
    typedef std::tuple<identifier, std::unique_ptr<Selection>> id_sel;
    std::vector<id_sel> selections;
};


class AndSelection: public Selection {
public:
    AndSelection(const ptree & cfg, OutputManager & out);
    virtual bool operator()(const Event & e);
    
private:
    std::vector<identifier> selids;
    
    TH1D * cutflow, *cutflow_raw; // histos are owned by the output file
};

// selects those which passed
//  not any of the given selections, i.e. if specifying
//  selection = "id1 id2 id3"
// the event passed the AndNotSelection iff
//  "!id1 and !id2 and !id3"
// This is useful as "catch-the-rest" selectio of an exclusive categorization
// of events based on selections id1, id2, id3.
class AndNotSelection: public Selection {
public:
    AndNotSelection(const ptree & cfg, OutputManager & out);
    virtual bool operator()(const Event & e);
    
private:
    std::vector<identifier> selids;
};

class PassallSelection: public Selection {
public:
    PassallSelection(const ptree & cfg, OutputManager & out){}
    virtual bool operator()(const Event & e){return true;}
};

}

#endif
