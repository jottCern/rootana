#ifndef RA_ANALYSIS_HPP
#define RA_ANALYSIS_HPP

#include "fwd.hpp"
#include "base/include/registry.hpp"
#include <boost/property_tree/ptree.hpp>

using boost::property_tree::ptree;


namespace ra {
    
/** \brief Abstract base class for all analysis modules
 *
 * Derive from this class and implement the 'begin_dataset' and 'process' methods. Make sure
 * to use the REGISTER_ANALYSIS_MODULE macro to make your module known to the plugin system.
 * Then name it in the configuration to execute it.
 */
class AnalysisModule {
public:
    
    virtual ~AnalysisModule();
    
    /** \brief Method called at the beginning of a dataset
     *
     * Here the initialization should take place, including:
     *  1. accessing the current dataset information (esp. dataset.tags)
     *  2. putting histograms in the output via 'out.put'
     *  3. declaring which Event members to write to the output TTree(s) via 'out.declare_(event)_output'
     *  4. use 'in' to declare input variables and where to find them in the input TTree
     * 
     * Note that typically, most modules only need 1. and 2.; for each data format, one can then write two AnalysisModules
     * performing 3. and 4.
     */
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out) = 0;
    
    /** \brief Method called for each event in the dataset
     *
     * Do the filling of histograms, calculating new variables, etc. here.
     */
    virtual void process(Event & event) = 0;
    
    /** \brief Method called whenever a new input file is opened.
     * 
     * Whenever a new input file is opened, the framework sets up the TTree according to the information
     * provided by the AnalysisModules via the InputManager. Before setting up the TTree, this
     * method is called to allow some special handling of information in the input file.
     * 
     * NOTE: For simple analyses, it should not be necessary to override this, as all required information
     * is read on an per-event basis. This method should only by used if it is necessary to access information
     * in the input file that is *not* part of this per-event information.
     * While it is possible to access the input event tree, some care should be taken: after
     * begin_file returns, InputTree will be called which will replace any connections (TTree::SetAddress) made
     * here with the ones defined in the InputTree. Therefore, try to avoid touching the input event tree; if you do, do not
     * expect that anything done here has any effect after this method returns.
     */
    virtual void begin_in_file(TFile & input_file){}
    
    virtual bool is_parallel_safe() const { return true; }
};


typedef ::Registry<AnalysisModule, std::string, const ptree&> AnalysisModuleRegistry;

}

#define REGISTER_ANALYSIS_MODULE(T) namespace { int dummy##T = ::ra::AnalysisModuleRegistry::register_<T>(#T); }

#endif
