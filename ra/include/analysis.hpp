#ifndef DRA_ANALYSIS_HPP
#define DRA_ANALYSIS_HPP

#include "fwd.hpp"
#include "base/include/registry.hpp"
#include <boost/property_tree/ptree.hpp>

using boost::property_tree::ptree;


namespace ra {
    
/** \brief Abstract base class for all analysis modules
 *
 * Using it comprises two parts: implementing the analysis logic and calling it within the event loop to do the actual work.
 *
 * For the first part, derive from this class and implement the 'begin_dataset' and 'process' methods.
 *
 * Calling 'begin_dataset' or 'process' can be done either
 * - "manually" from another AnalysisModule or from a SFrame cycle
 * - "automatically" by using the AnalysisModuleRunner SFrame cycle which does nothing but call these
 * methods in the correct order, after setting up the input.
 */
class AnalysisModule {
public:
    
    virtual ~AnalysisModule();
    
    /** \brief Method called at the beginning of a dataset
     *
     * Use the Context to
     * - access the configuration
     * - create output
     */
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out) = 0;
    
    /** \brief Method called for each event in the dataset
     *
     * Do the filling of histograms, calculating new variables, etc. here.
     */
    virtual void process(Event & event) = 0;
    
    /** \brief Method called whenever a new input file is opened.
     * 
     * The method is called before wiring the input tree with the event container.
     * 
     * NOTE: For simple analyses, it should not be necessary to override this, as all input
     * is read on an event-based mechanism. This method should only by used
     * if it is necessary to access information in the input file that is *not* part of the event
     * tree.
     * Note that while it is possible to access the input event tree, some care should be taken: after
     * begin_file returns, InputTree will be called which will replace any connections (TTree::SetAddress) made
     * here with the ones defined in the InputTree. Therefore, try to avoid touching the input event tree; if you do, do not
     * expect that anything done here has any effect after this method returns.
     */
    virtual void begin_in_file(TFile & input_file){}
};


typedef ::Registry<AnalysisModule, std::string, const ptree&> AnalysisModuleRegistry;

}


#define REGISTER_ANALYSIS_MODULE(T) namespace { int dummy##T = ::ra::AnalysisModuleRegistry::register_<T>(#T); }

#endif
