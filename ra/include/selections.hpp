#ifndef RA_SELECTIONS_HPP
#define RA_SELECTIONS_HPP

#include "event.hpp"
#include "analysis.hpp"
#include "config.hpp"
#include "base/include/registry.hpp"

#include "TH1D.h"
#include <string>

namespace ra{

/** \brief Base class for event selections
 * 
 * To implement your own event selection, derive from this class and implement a constructor taking a const ptree & and OutputManager & as
 * arguments. Also implement the evaluation operator to perform the actual selection decision.
 * 
 * The \c OutputManager passed to the constructor is usually not necessary for the 'core function' of this class (to make
 * event seletion decisions), but it allows to create monitoring histograms, which can be very useful for debugging selections
 * (for example, the \c AndSelection makes use of that to create the cutflow histograms).
 * 
 * After implementing the class, use the REGISTER_SELECTION macro to register the selection. This allows you to use your new module
 * within the \c Selections AnalysisModule.
 */
class Selection{
public:
    
    /// returns true if the event passes the selection and should be kept, false otherwise
    virtual bool operator()(const Event & e) = 0;
    virtual ~Selection(){}
};


typedef ::Registry<Selection, std::string, const ptree &,  InputManager &, OutputManager &> SelectionRegistry;
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
 * 
 * Note that this class only calculates the result of the selection as boolean and stores them to the event. Event
 * processing is *not* stopped in any case by this module. If you want to stop further event processing (i.e. further
 * modules being called for this event), use the \c stop_unless AnalysisModule; you can let it run
 * directly after this Selections module.
 */
class Selections: public AnalysisModule{
public:
    Selections(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    ptree cfg;
    typedef std::tuple<Event::Handle<bool>, std::unique_ptr<Selection>> handle_sel;
    std::vector<handle_sel> selections;
};


/** \brief Prevent further event processing unless a selection is true
 * 
 * Typically, this is scheduled after a Selections module to short-cut further processing of events.
 * 
 * Configuration:
 * \code
 * {
 *   type stop_unless
 *   selection final
 * }
 * \endcode
 * 
 * \c selection is the event member name of type \c bool. Only if this event member is valid and set to true, the event is further
 * processed.
 */
class stop_unless: public AnalysisModule {
public:
    stop_unless(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    
private:
    std::string s_selection;
    Event::Handle<bool> selection;
    Event::Handle<bool> stop_handle;
};

/** \brief Compute the logical and of several existing selections; optionally, fill a cutflow histogram
 * 
 * Configuration (usually within a \c Selections module):
 * \code
 *  {
 *    type AndSelection
 *    selections "mll bcand"
 *    cutflow_hname cf0   ; optional
 *  }
 * \endcode
 * 
 * \c selections is a space-separated list of selection names which are tested
 * 
 * \c cutflow_hname is an optional name of 1D "cutflow" histogram to create at the top level of the output file. It contains 
 *    \c n_selections + 1 bins, where \c n_selections is the number of selection ids given in the \c selections setting. The first
 *    bin contains the sum of all events seen by this module, the second bin those events which pass the first selection (as given in \c selections),
 *    the third bin those which also survive the second selection, etc.
 *    Two histograms are created for weighted (using the name directly) and for unweighted event counts (using the name + "_raw" as histogram name).
 *    If no \c cutflow_hname is given, histogram is created.
 */
class AndSelection: public Selection {
public:
    AndSelection(const ptree & cfg, InputManager & in, OutputManager & out);
    virtual bool operator()(const Event & e);
    
private:
    std::vector<Event::Handle<bool> > sel_handles;
    Event::Handle<double> weight_handle;
    
    TH1D * cutflow, *cutflow_raw; // histos are owned by the output file
};


/** \brief Calculate a selection of and-not
 * 
 * Configuration:
 * \code
 * selections = "id1 id2 id3"
 * \endcode
 * 
 * The event passes this AndNotSelection if and only if "!id1 and !id2 and !id3" is true.
 * 
 * This is useful as a "catch-the-rest" category selection where -- to stay with the example above --
 * id1, id2, and id3 would be categories and this AndNotSlection would catch all not going in any of those three categories.
 *
 */
class AndNotSelection: public Selection {
public:
    AndNotSelection(const ptree & cfg, InputManager & in, OutputManager & out);
    virtual bool operator()(const Event & e);
    
private:
    std::vector<Event::Handle<bool> > sel_handles;
};

/// A Selection that lets all events pass by returning always true
class PassallSelection: public Selection {
public:
    PassallSelection(const ptree & cfg,  InputManager & in, OutputManager & out){}
    virtual bool operator()(const Event & e){return true;}
};

}

#endif
