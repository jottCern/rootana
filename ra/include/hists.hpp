#ifndef RA_HISTS_HPP
#define RA_HISTS_HPP

#include "context.hpp"
#include "analysis.hpp"
#include "config.hpp"
#include "event.hpp"
#include "TH1D.h"

#include "base/include/registry.hpp"

#include <boost/optional.hpp>

#include <type_traits>
#include <functional>

namespace ra{

class HistFiller;

/** \brief Utility class for easy filling of Histograms
 *
 * To use it:
 * - derive from this class
 * - book the histograms in the constructor, using the book* routines
 * - fill the (non-auto) histograms in the process method; you can use the 'get' method
 *   to retrieve the histograms booked previously.
 *
 * To run it in the analysis workflow, use the HistFiller module.
 *
 * Alternative use without inheritance:
 *  - from your AnlaysisModule::begin_dataset, construct an instance of this type and create histograms
 *  - in your AnalysisModule::process, call Hists::process_all to process the autofill histograms
 *  - in addition, you can use Hists::get from AnalysisModule::process to retrieve historgams you can fill
 *
 * The class is usually called from a HistsFiller. Instances are constructed at HistsFiller::begin_dataset,
 * so they live only for one dataset.
 */
class Hists{
public:
    
    Hists(const std::string & dirname, const s_dataset & dataset, OutputManager & out);
    
    virtual void process(Event & event){}
    
    // book a histogram with the name as specified by the identifier:
    template<typename T, typename... cargs>
    T * book(const identifier & id, cargs... parameters);
    
    // the functor should return not-a-number to prevent filling
    typedef std::function<double (Event &)> event_functor;
    void book_1d_autofill(event_functor f, const char * name, int nbins, double xmin, double xmax, Event::Handle<double> weight_handle = Event::Handle<double>());

    // get a histogram booked with book:
    TH1 * get(const identifier & id);
    
    virtual ~Hists();
    
    // this is called by HistFiller; it calls the virtual 'process' method and then does the autofills.
    void process_all(Event & event);
    
private:
    
    struct autofill_histo {
        event_functor f;
        Event::Handle<double> weight_handle;
        TH1* histo;
    };
    
    std::string dirname; // including the final '/'
    OutputManager & out;
    std::vector<autofill_histo> autofill_histos;
    std::map<identifier, TH1*> i2h; // name to histos
};



template<typename T, typename... cargs>
T * Hists::book(const identifier & id, cargs... parameters){
    static_assert(std::is_base_of<TH1,T>::value, "book is only allowed for histograms; T must derive from TH1");
    std::string name = id.name();
    T * result = new T(name.c_str(), name.c_str(), parameters...);
    result->Sumw2();
    out.put((dirname + name).c_str(), result);
    i2h[id] = result;
    return result;
}

/** \brief Use selections and Hists to fill histograms
 *
 * configuration:
 * \code
 * dirname1 {  ; name of the directory in the output file in which histograms are created
 *    selection btag12  ; name of the selection. Default is to use the diretory name.
 *    weights "btag12"  ; one more more (space-separated) names of double event members to use as additional event weight.
 *    hists Hists1   ; name of a Hists class
 *    hists Hists2   ; name of another Hists class
 * }
 * 
 * dirname2 {
 *   hists {
 *      type Hists3  ; name of a Hists class
 *      myparam myvalue  ; configuration for Hists3
 *   }
 * }
 * \endcode
 * 
 * At the beginning of each dataset, the constructors for the Hists configured are called. The Hists classes
 * create their output in the respective directory.  For each event, all configured directories are processed
 * in the order as given in the configuration file. For each directory,
 *   - it is checked whether it passes the selection (of not, processing aborts here)
 *   - the event weight is adapted if the 'weights' setting is given
 *   - Hists::process_all is called for all configured \c Hists classes.
 *   - the weight is reverted if necessary
 */
class HistFiller: public AnalysisModule{
public:
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    explicit HistFiller(const ptree & cfg);
    
private:
    struct outdir{
        std::string dirname;
        std::vector<std::unique_ptr<Hists>> hists;
        Event::Handle<bool> sel_handle;
        std::vector<Event::Handle<double>> weight_handles;
    };
    ptree cfg;
    std::vector<outdir> outdirs;
    
    Event::Handle<double> h_weight;
    
    static void parse_dir_cfg(const ptree & dircfg, outdir & od, const s_dataset & dataset, InputManager & in, OutputManager & out);
};

typedef ::Registry< ::ra::Hists, std::string, const ptree &, const std::string&, const ::ra::s_dataset &, ::ra::InputManager &, ::ra::OutputManager &> HistsRegistry;

}

#define REGISTER_HISTS(T) namespace { int dummy##T = ::ra::HistsRegistry::register_<T>(#T); }

#endif

