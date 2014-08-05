#ifndef RA_ANALYSIS_HPP
#define RA_ANALYSIS_HPP

#include "fwd.hpp"
#include "base/include/registry.hpp"
#include <boost/property_tree/ptree.hpp>

using boost::property_tree::ptree;

namespace ra {
    
/** \brief Abstract base class for all analysis modules
 *
 * To implement your own AnalysisModule class that can be used in the 'modules' section of the configuration file,
 * you have to 
 *  1. Create a new clas which derives from this class and provide a constructor taking the configuration (a 'const ptree &') as only argument
 *  2. implement the pure virtual methods, i.e. \c begin_dataset and \c process
 *  3. Use the REGISTER_ANALYSIS_MODULE macro at the global scope of your .cpp file to make your class known to the plugin system
 * 
 * These are the minimal steps required to use the module from the configuration.
 * 
 * For modules needing more control, there is also a \c begin_in_file method which is called whenever a new
 * root file is opened.
 * 
 * Note for modules which are not safe to be executed in parallel (such as modules checking for duplicate
 * events which can only be reliable if they see the whole dataset) should also override
 * the 'is_parallel_safe' method to return \c false such that an error is raised eraly in the processing.
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
     * Note that typically, most modules only need 1. and 2.; and let special AnalysisModules takes care of 3. and 4.,
     * where usually only one is needed per TTree format.
     */
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out) = 0;
    
    /** \brief Method called for each event in the dataset
     *
     * Do the filling of histograms, calculate new variables for the event, etc.
     */
    virtual void process(Event & event) = 0;
    
    /** \brief Method called whenever a new input file is opened.
     * 
     * Whenever a new input file is opened, the framework sets up the TTree according to the information
     * provided by the AnalysisModules via the InputManager. Before setting up the TTree, this
     * method is called to allow some special handling of information in the input file.
     * 
     * NOTE: For simple analyses, it should not be necessary to override this method, as all required information
     * is usually on a per-event basis and reading the event stream is handled by the framework (and by the \c InputManager, see
     * \c begin_dataset). Therefore, this method should only by used if it is necessary to access information
     * in the input file that is *not* part of this per-event information.
     * While it is possible to access the input event tree, some care should be taken: after
     * \c begin_in_file returns and before the first \c process call is made, the framework sets up the input tree
     * according to the declarations provided to the \c InputManager which might replace connections
     * (TTree::SetAddress) made here. Therefore, try to avoid touching the input event tree; if you do, do not
     * expect that anything done here has any effect after this method returns.
     */
    virtual void begin_in_file(TFile & input_file){}
    
    
    /** \brief Return whether this AnalysisModule can be considered safe for parallel/distributed execution
     * 
     * This method is used by the framework for basic consistency checks: When executing in a parellel environment,
     * an exception is raised before event processing unless all AnalysisModules return \c true for this method.
     * 
     * Note that the value returned here can depend on the configuration provided in the constructor, but it is called
     * before \c begin_dataset and hence can not depend on any initialization done there.
     */
    virtual bool is_parallel_safe() const { return true; }
};


typedef ::Registry<AnalysisModule, std::string, const ptree&> AnalysisModuleRegistry;

}

#define REGISTER_ANALYSIS_MODULE(T) namespace { int dummy##T = ::ra::AnalysisModuleRegistry::register_<T>(#T); }

#endif
