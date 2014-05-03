#ifndef RA_PROCESSING_HPP
#define RA_PROCESSING_HPP

#include "base/include/log.hpp"
#include "fwd.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ra {

// A utility classes to initialize per-dataset info as nedded for both
// the local ra program and the dra::Worker
class AnalysisController {
public:
    explicit AnalysisController(const s_config & config);
 
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
    // initializing the same file again is a no-op.
    void start_file(size_t ifile);
    
    // get the number of events in the file last initialized with start_file
    size_t get_file_size() const;
    
    // run over the given range [ifirst, ilast) of events in the current file.
    // ifirst must be smaller than the current file size, ilast can be larger (in which
    // case it is truncated to that length).
    void process(size_t ifirst, size_t ilast);
    
    // get the number of bytes read in process since the last call to nbytes_read
    size_t nbytes_read();
    
    ~AnalysisController();
    
private:
    // check that a dataset / an input file have been initialized:. If not, they throw an invalid_argument
    // exception.
    void check_dataset() const;
    void check_file() const;
    
    Logger & logger;
    
    const s_config & config;
    std::vector<std::unique_ptr<AnalysisModule>> modules;
    std::vector<std::string> module_names;
    
    // per dataset:
    size_t current_idataset;
    std::string outfile_path;
    std::unique_ptr<TFile> outfile;
    std::unique_ptr<ra::TFileOutputManager> out;
    std::unique_ptr<ra::Event> event;
    std::unique_ptr<ra::TTreeInputManager> in;
    
    // per-file:
    size_t current_ifile;
    std::unique_ptr<TFile> infile;
    size_t infile_nevents;
    
    // statistics:
    size_t n_read;
};

}


#endif
