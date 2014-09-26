#ifndef DRA_MASTER_HPP
#define DRA_MASTER_HPP

#include "dc/include/swarm_manager.hpp"
#include "ra/include/fwd.hpp"
#include "ra/include/context-backend.hpp"
#include "messages.hpp"

namespace dra {
    
namespace detail {
    
// a union of integer intervals, to denote a range of events to process within a given file.
struct IndexRanges {
public:
    // make an interval [from, to)
    IndexRanges(size_t from, size_t to);
    IndexRanges(){} // empty interval
    
    // get a single interval [from, to) from this set (removing it at the same time), limiting it to maxsize
    std::pair<size_t, size_t> consume(size_t maxsize);
    
    // peek: get the first full interval. throws invalid_argument in case this is empty()
    std::pair<size_t, size_t> peek() const;
    
    // test whether it's empty
    bool empty() const;
    
    // make the union. If not disjoint, throw a invalid_argument
    // exception.
    void disjoint_union(const IndexRanges & rhs);
    
    size_t size() const;
    
private:
    std::list<std::pair<size_t, size_t> > intervals; // ordered
};


// a single event interval, with file information.
struct EventRange {
    size_t ifile, first, last;
};

// Split the events to process into small parts, in a way suitable for this problem:
// The number of files to be processed is assumed to be known, but not the number
// of events in each file, which will only be known after processing the first part of it.
//
// The events are processed in blocks, where there is a "zero" blocksize (defined at construction time),
// which determines the size of the block at the beginning of the file; at this point, the file
// size is (usually) not yet known. The size of all other blocks can be specified on a case-by-case basis.
class EventRangeManager {
public:
    explicit EventRangeManager(size_t nfiles, size_t blocksize0);
    
    // check whether EventRange are available for consumption:
    bool available() const;
    
    // get an EventRange, marking it as consumed. Trying to consume although nothing
    // is available throws an invalid_argument exception
    // The default preferred_ifile == size_t(-1) means that the first file should be used 
    // that has not been processed at all so far.
    // blocksize is the requested number of events. It is only a hint and the returned interval
    // can deviate from that:
    //  * if the first block within a file is returned, the returned event range has always size blocksize0
    //    which can be smaller or larger than the specified blocksize
    //  * if too few events are available in the chosen files, fewer events are returned.
    EventRange consume(size_t preferred_ifile = static_cast<size_t>(-1), size_t blocksize = -1);
    
    // set the file size for the given file. Can be called several times for the same
    // file, given that nevents is always the same; otherwise, a invalid_argument exception
    // is thrown
    void set_file_size(size_t ifile, size_t nevents);
    
    // add the given EventRange to the list of available EventRanges. It must correspond to a previously
    // consumed one.
    // This is typically used in case of worker failure to re-run other workers on those
    // Events.
    void add(const EventRange & er);
    
    // Current estimate of number or events left to do.
    // This is not accurate if not all file sizes are known yet; in this case, all unknown files 
    // count as blocksize0.
    size_t nevents_left() const;
    
    // number is negative if the total number is not known yet completely; in this case, each unknown file
    // is counted with blocksize0.
    ssize_t nevents_total() const;
    
    // total number of files
    size_t nfiles_total() const{
        return nevents.size();
    }
    
    size_t nfiles_done() const;
    
private:
    size_t blocksize0;
    std::vector<ssize_t> nevents; // -1 = unknown
    std::vector<IndexRanges> events_left;
};

    
}



class Master;

// abstract base class for getting notifications about the Master state
class MasterObserver: public dc::SwarmObserver {
public:
    virtual void set_master(Master * master){} // called when added to master
    virtual void on_dataset_start(const ra::s_dataset & dataset){}
    virtual void on_stop_complete(){}
    virtual ~MasterObserver();
};


// The master, giving work to workers.
// There are three conditions the IOManager::process for the master stops (beyond the external methods of kill -9 or stopping the IOManager directly):
// * by calling Master::stop. This will send a stop command to all workers as soon as possible to let them close
//   their (unmerged) output file cleanly.
// * by calling Master::abort. This will stop processing immediately, not sending any commands to the workers.
// * if all data is processed completely
//
// To distinguish between these conditions, use the 'stopped', 'aborted', and 'completed' methods, respectively. Note
// that completed implies stopped.
class Master {
public:
    typedef ::dc::WorkerId WorkerId;
    
    explicit Master(const std::string & cfgfile);
    
    void add_worker(std::unique_ptr<dc::Channel> c);
    ~Master();
    
    size_t nevents_left() const{
        return erm ? erm->nevents_left() : 0;
    }
    
    ssize_t nevents_total() const{
        return erm ? erm->nevents_total() : 0;
    }
    
    size_t nfiles_done() const{
        return erm ? erm->nfiles_done() : 0;
    }
    
    size_t nfiles_total() const{
        return erm ? erm->nfiles_total() : 0;
    }
    
    // get the number of bytes read by all workers in the current dataset;
    // Note that this is the number of bytes reported by TBranch::GetEntry
    // which is the number of uncompressed bytes, so the actual number
    // of bytes read from the file system is lower than this.
    size_t nbytes_read() const {
        return nbytes_read_;
    }
    
    void stop();
    
    void abort();
    
    bool aborted() const {
        return aborted_;
    }
    
    bool stopped() const {
        return stopped_;
    }
    
    bool completed() const {
        return completed_;
    }
    
    // note: the observer will also be added to the SwarmManager as SwarmObserver.
    void add_observer(const std::shared_ptr<MasterObserver> & observer);
    
    // start sending work to the workers via the worker channels.
    void start();
    
private:
    std::unique_ptr<Configure> generate_configure(const WorkerId & wid);
    std::unique_ptr<Process> generate_process(const WorkerId & wid);
    std::unique_ptr<Close> generate_close(const WorkerId & wid);
    std::unique_ptr<Merge> generate_merge(const WorkerId & wid);
    std::unique_ptr<Stop> generate_stop(const WorkerId & wid);
    
    void worker_failed(const WorkerId & worker, const dc::StateGraph::StateId & last_state);
    void process_complete(const WorkerId & worker, std::unique_ptr<dc::Message> result);
    void close_complete(const WorkerId & worker, std::unique_ptr<dc::Message> result);
    void merge_complete(const WorkerId & worker, std::unique_ptr<dc::Message> result);
    void stop_complete(const WorkerId & worker, std::unique_ptr<dc::Message> result);
    
    // use idataset = config.datasets.size() to finalize completely
    void init_dataset(size_t idataset);
    void finalize_dataset(const WorkerId & last_worker);
    std::string get_unmerged_filename(int iworker) const;
    std::string get_filename(int iworker) const;
    
    size_t get_n_unmerged() const;
    
    
    std::shared_ptr<Logger> logger;
    dc::SwarmManager sm;
    
    // per-config
    std::string cfgfile;
    std::unique_ptr<ra::s_config> config;
    std::unique_ptr<ra::OutputManagerOperations> out_ops; // needed for merging and filename information
    
    // per-dataset information:
    int idataset;
    size_t nbytes_read_;
    
    // per-dataset processing information:
    std::unique_ptr<detail::EventRangeManager> erm;
    std::map<WorkerId, std::list<detail::EventRange> > worker_ranges; // save which ranges have been processed by which worker. This is to re-schedule ranges of failed workers; failed workers are removed from this list.
    
    // closing and merging status information:
    std::map<WorkerId, bool> closed; // only save workers needing closing = processing, non-failing workers
    
    std::map<WorkerId, bool> needs_merging; // only save closed workers
    
    std::vector<std::shared_ptr<MasterObserver>> observers;
    
    bool stopped_;
    bool aborted_;
    bool completed_;
};


}

#endif
