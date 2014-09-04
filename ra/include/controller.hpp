#ifndef RA_PROCESSING_HPP
#define RA_PROCESSING_HPP

#include "base/include/log.hpp"
#include "fwd.hpp"
#include "event.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ra {

/** \brief Central dispatcher calling the AnalysisModule routines
 * 
 * This is where (part of) the event loop lives; to use this class, construct it via a s_config object (which in
 * turn is constructed via a configuration file). Then, execute the start_dataset, start_file, and process methods
 * in turn to process all events in all datasets mentioned in the configuration.
 * 
 * This class avoids duplication between the serial local version (ra/ra) and the distributed versions
 * (e.g. dra/dra_local), which would otherwise contain the same code to construct the modules, initialize output, cleanup, etc.
 */
class AnalysisController {
public:
    
    // Note that the only effect of the \c check_for_parallel_safety flag is that all
    // AnalysisModules' is_parallel_safe methods are called and an exception is thrown if one of them
    // returns false.
    explicit AnalysisController(const s_config & config, bool check_for_parallel_safety);
 
    // initialize data structures for the given dataset (e.g. calling
    // the begin_dataset methods from the modules). Initializing
    // the same dataset with the same output file path is a (legal) no-op;
    // using the same idataset and another output file path will behave
    // almost as if one starts again with the dataset (including calling the module's
    // begin_dataset methods), but will keep the current input file information.
    // This allows switching output files and thereby e.g. distributing
    // large output of a single dataset over many files.
    //
    // the special value idataset = size_t(-1) is used to close the current
    // dataset without initializing a new one.
    void start_dataset(size_t idataset, const std::string & outfile_path);
    
    const s_dataset & current_dataset() const;
    
    // initialize file ifile in the current dataset. Note that
    // initializing the same file again is a legal no-op.
    void start_file(size_t ifile);
    
    // get the number of events in the file last initialized with start_file
    size_t get_file_size() const;
    
    
    struct ProcessStatistics {
        size_t nbytes_read, nevents_survived;
    };
    
    // run over the given range [ifirst, ilast) of events in the current file.
    // ifirst must be smaller than the current file size, ilast can be larger (in which
    // case it is truncated to that length).
    // s can be NULL.
    void process(size_t ifirst, size_t ilast, ProcessStatistics * s = 0);
    
    ~AnalysisController();
    
private:
    // check that a dataset / an input file have been initialized:. If not, they throw an invalid_argument
    // exception.
    void check_dataset() const;
    void check_file() const;
    
    std::shared_ptr<Logger> logger;
    
    const s_config & config;
    std::vector<std::unique_ptr<AnalysisModule>> modules;
    std::vector<std::string> module_names;
    
    // per dataset:
    size_t current_idataset;
    std::string outfile_path;
    std::unique_ptr<ra::TFileOutputManager> out;
    std::unique_ptr<ra::Event> event;
    std::unique_ptr<ra::TTreeInputManager> in;
    
    Event::Handle<bool> handle_stop;
    
    // per-file:
    size_t current_ifile;
    std::unique_ptr<TFile> infile;
    size_t infile_nevents;
};

}


#endif
