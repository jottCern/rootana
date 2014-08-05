#include "master.hpp"
#include "stategraph.hpp"

#include "base/include/utils.hpp"
#include "ra/include/config.hpp"
#include "ra/include/root-utils.hpp"

using namespace dra;
using namespace dra::detail;
using namespace dc;
using namespace ra;
using namespace std;

namespace ph = std::placeholders;

IndexRanges::IndexRanges(size_t from, size_t to){
    if(to < from) throw invalid_argument("not an interval");
    if(from < to){
        intervals.emplace_back(from, to);
    }
}

bool IndexRanges::empty() const {
    return intervals.empty();
}

std::pair<size_t, size_t> IndexRanges::peek() const{
    if(intervals.empty()) throw invalid_argument("peek for empty IndexRanges called");
    return intervals.front();
}

std::pair<size_t, size_t> IndexRanges::consume(size_t maxsize){
    if(intervals.empty()) throw invalid_argument("consume for empty IndexRanges called");
    auto & f = intervals.front();
    if(f.second - f.first <= maxsize){
        auto result(f);
        intervals.pop_front();
        return result;
    }
    else{
        std::pair<size_t, size_t> result(f.first, f.first + maxsize);
        f.first += maxsize;
        return result;
    }
}

void IndexRanges::disjoint_union(const IndexRanges & rhs){
    if(rhs.empty()) return;
    auto iright = rhs.intervals.begin();
    auto ileft = intervals.begin();
    while(iright != rhs.intervals.end()){
        // given iright, there are potentially many overlapping intervals here. Find
        // the first candidate which could overlap, i.e. the first interval ileft for which
        // the high end is *not* smaller than iright's low end:
        while(ileft != intervals.end() && ileft->second <= iright->first) ++ileft;
        // check that ileft does not overlap
        if(ileft != intervals.end()){
            if(ileft->first < iright->second) throw invalid_argument("union not disjoint");
        }
        intervals.insert(ileft, *iright);
        ++iright;
    }
    
    // merging:
    for(auto it = intervals.begin(); it!=intervals.end(); ++it){
        auto it_after = it;
        ++it_after;
        while(it_after != intervals.end() && it->second == it_after->first){
            it->second = it_after->second;
            intervals.erase(it_after);
            it_after = it;
            ++it_after;
        }
    }
}

size_t IndexRanges::size() const{
    size_t result = 0;
    for(const auto & i : intervals){
        result += i.second - i.first;
    }
    return result;
}

EventRangeManager::EventRangeManager(size_t nfiles, size_t blocksize_): blocksize0(blocksize_),
   nevents(nfiles, -1), events_left(nfiles){
    // insert first block in each file:
    for(auto & er : events_left){
        er.disjoint_union(IndexRanges(0, blocksize0));
    }
}


bool EventRangeManager::available() const{
    // there is something available iff there is a non-empty index range for some file:
    return any_of(events_left.begin(), events_left.end(), [](const IndexRanges & r){return not r.empty();});
}

EventRange EventRangeManager::consume(size_t preferred_ifile, size_t blocksize){
    const size_t prefer_unprocessed(-1);
    if(blocksize == size_t(-1)) blocksize = blocksize0;
    assert(preferred_ifile == prefer_unprocessed || preferred_ifile < events_left.size());
    size_t ifile = preferred_ifile;
    if(preferred_ifile == prefer_unprocessed || events_left[preferred_ifile].empty()){
        // find unprocessed ifile, i.e. a ifile for which the size is unknown or for
        // which the file size is known and matches the number of available events:
        for(size_t i=0; i<events_left.size(); ++i){
            if(events_left[i].empty()) continue;
            if(nevents[i] < 0 || static_cast<size_t>(nevents[i]) == events_left[i].size()){
                ifile = i;
                break;
            }
        }
        // if this was not successful, just find any file not completely processed:
        if(ifile == preferred_ifile){
            for(size_t i=0; i<events_left.size(); ++i){
                if(!events_left[i].empty()){
                    ifile = i;
                    break;
                }
            }
        }
        // if this was not successfull, it means that we do not have any events left:
        if(ifile == preferred_ifile) throw invalid_argument("consume called although nothing available");
    }
    assert(ifile != prefer_unprocessed);
    assert(!events_left[ifile].empty());
    bool use_blocksize0 = events_left[ifile].peek().first == 0;
    auto interval = events_left[ifile].consume(use_blocksize0 ? blocksize0 : blocksize);
    EventRange result{ifile, interval.first, interval.second};
    // the result should either be the first block or be beyond, within the file:
    assert((result.first == 0 && result.last == blocksize0) ||
       (result.first >= blocksize0 && nevents[ifile] >= 0 && result.last <= static_cast<size_t>(nevents[ifile])));
    return result;
}

void EventRangeManager::set_file_size(size_t ifile, size_t n) {
    assert(ifile < nevents.size());
    if(nevents[ifile] >= 0){
        if(static_cast<size_t>(nevents[ifile]) != n){
            throw invalid_argument("inconsistent file sizes");
        }
    }
    else{
        // now that we know the actual file size, we can extend the events_left accordingly:
        nevents[ifile] = n;
        if(n > blocksize0){
            events_left[ifile].disjoint_union(IndexRanges(blocksize0, n));
        }
    }
}

void EventRangeManager::add(const EventRange & er){
    assert(er.ifile < nevents.size());
    // note that we always allow adding the first block  of size blocksize0, even if we know that the file size is different.
    if((nevents[er.ifile] < 0 || er.last > static_cast<size_t>(nevents[er.ifile])) && !(er.first == 0 && er.last == blocksize0)){
        throw invalid_argument("adding range beyond file");
    }
    events_left[er.ifile].disjoint_union(IndexRanges(er.first, er.last)); // can throw
}

size_t EventRangeManager::nevents_left() const{
    size_t result = 0;
    for(auto & el : events_left){
        result += el.size();
    }
    return result;
}

size_t EventRangeManager::nfiles_done() const{
    size_t result = 0;
    for(size_t i=0; i<events_left.size(); ++i){
        // a file is done if we know its size and no range is left:
        if(events_left[i].size() == 0 && nevents[i] >= 0){
            ++result;
        }
    }
    return result;
}
    
// number is negative if the total number is not known yet completely
ssize_t EventRangeManager::nevents_total() const{
    ssize_t result = 0; // keep it positive for now
    ssize_t sign = 1;
    for(auto n : nevents){
        if(n >= 0) result += n;
        else{
            result += blocksize0;
            sign = -1;
        }
    }
    return sign * result;
}


MasterObserver::~MasterObserver(){}

Master::~Master(){}

Master::Master(const string & cfgfile_): logger(Logger::get("dra.Master")), sm(dra::get_stategraph(), bind(&Master::worker_failed, this, ph::_1, ph::_2)), idataset(-1),
  stopping(false), failed_(false){
    cfgfile = realpath(cfgfile_);
    config.reset(new s_config(cfgfile));
    if(!is_directory(config->options.output_dir)){
        LOG_THROW("options.output dir '" << config->options.output_dir << "' does not exist, is not a directory, or insufficient access rights");
    }
    
    config->logger.apply(config->options.output_dir);
    if(config->datasets.empty()){
        LOG_THROW("no dataset to process");
    }
    
    // setup connections:
    const auto & g = sm.get_graph();
    
    auto s_start = g.get_state("start");
    auto s_configure = g.get_state("configure");
    auto s_process = g.get_state("process");
    auto s_close = g.get_state("close");
    auto s_merge = g.get_state("merge");
    auto s_stop = g.get_state("stop");
    
    sm.connect<Configure>(s_start, bind(&Master::generate_configure, this, ph::_1));
    sm.connect<Process>(s_configure, bind(&Master::generate_process, this, ph::_1));
    sm.connect<Process>(s_process, bind(&Master::generate_process, this, ph::_1));
    sm.connect<Process>(s_merge, bind(&Master::generate_process, this, ph::_1));
    sm.connect<Process>(s_close, bind(&Master::generate_process, this, ph::_1));
    sm.connect<Close>(s_process, bind(&Master::generate_close, this, ph::_1));
    sm.connect<Merge>(s_close, bind(&Master::generate_merge, this, ph::_1));
    sm.connect<Merge>(s_merge, bind(&Master::generate_merge, this, ph::_1));
    
    sm.connect<Stop>(s_close, bind(&Master::generate_stop, this, ph::_1));
    sm.connect<Stop>(s_merge, bind(&Master::generate_stop, this, ph::_1));
    sm.connect<Stop>(s_start, bind(&Master::generate_stop, this,ph:: _1));
    sm.connect<Stop>(s_configure, bind(&Master::generate_stop, this, ph::_1));
    
    sm.set_result_callback(s_process, bind(&Master::process_complete, this, ph::_1, ph::_2));
    sm.set_result_callback(s_close, bind(&Master::close_complete, this, ph::_1, ph::_2));
    sm.set_result_callback(s_merge, bind(&Master::merge_complete, this, ph::_1, ph::_2));
    sm.set_result_callback(s_stop, bind(&Master::stop_complete, this, ph::_1, ph::_2));
    
    sm.set_target_state(s_process);
    sm.check_connections();
}

void Master::init_dataset(size_t id){
    LOG_DEBUG("init_dataset called for id=" << id);
    bool last = static_cast<size_t>(id) >= config->datasets.size();
    idataset = id;
    worker_ranges.clear();
    closed.clear();
    needs_merging.clear();
    nbytes_read_ = 0;
    // NOTE: make sure to call the methods of sm. last, as they will call the generate_process methods and friends so
    // we need to make sure they see a consistent state ...
    if(last){
        erm.reset();
        sm.activate_restriction_set(sm.get_graph().get_restriction_set("noprocess")); // note: nomerge is active anyway
        stop();
    }
    else{
        LOG_INFO("Start processing dataset " << config->datasets[idataset].name);
        erm.reset(new EventRangeManager(config->datasets[id].files.size(), config->options.blocksize));
        for(auto & observer : observers){
            observer->on_dataset_start(config->datasets[idataset]);
        }
        sm.deactivate_restriction_set(sm.get_graph().get_restriction_set("noprocess"));
        sm.set_target_state(sm.get_graph().get_state("process"));
    }
}

void Master::start(){
    assert(idataset == -1);
    init_dataset(0);
}

void Master::add_worker(unique_ptr<Channel> c){
    sm.add_worker(move(c));
}

std::unique_ptr<Configure> Master::generate_configure(const WorkerId & wid){
    std::unique_ptr<Configure> config(new Configure());
    config->cfgfile = cfgfile;
    config->iworker = wid.id();
    return config;
}

std::unique_ptr<Process> Master::generate_process(const WorkerId & wid){
    // find an EventRange corresponding to what the worker did last:
    size_t preferred_ifile(-1);
    auto wr = worker_ranges.find(wid);
    if(wr != worker_ranges.end()){
        preferred_ifile = wr->second.back().ifile;
    }
    auto er = erm->consume(preferred_ifile);
    worker_ranges[wid].push_back(er);
    if(!erm->available()){
        sm.activate_restriction_set(sm.get_graph().get_restriction_set("noprocess"));
    }    
    closed[wid] = false;
    
    // send it to the worker:
    std::unique_ptr<Process> result(new Process());
    result->idataset = idataset;
    result->ifile = er.ifile;
    result->first = er.first;
    result->last = er.last;
    result->files_hash = config->datasets[idataset].filenames_hash;
    assert(worker_ranges.find(wid) != worker_ranges.end());
    LOG_DEBUG("generate_process for worker " << wid.id() << ": file = " << result->ifile << "; events = " << result->first << " -- " << result->last);
    return move(result);
}

std::unique_ptr<Close> Master::generate_close(const WorkerId & wid){
    return unique_ptr<Close>(new Close());
}

std::unique_ptr<Stop> Master::generate_stop(const WorkerId & wid){
    return unique_ptr<Stop>(new Stop());
}

void Master::worker_failed(const WorkerId & worker, const dc::StateGraph::StateId & last_state){
    LOG_WARNING("Worker " << worker.id() << " failed in state " << sm.get_graph().name(last_state));
    if(last_state == sm.get_graph().get_state("merge")){
        failed_ = true;
        LOG_THROW("Worker " << worker.id() << " failed while merging; this is not recoverable");
        // TODO: better error handling / reporting: could re-start whole dataset (but: careful with endless loops ...)
    }
    else if(last_state == sm.get_graph().get_state("close")){
        if(closed[worker]){
            LOG_WARNING("Worker " << worker.id() << " failed while idling in close state; ignoring that");
        }
        else{
            failed_ = true;
            LOG_THROW("Worker " << worker.id() << " failed while closing; this is not recoverable");
        }
    }
    else if(last_state == sm.get_graph().get_state("process")){
        auto wr = worker_ranges.find(worker);
        assert(wr!=worker_ranges.end());
        assert(!wr->second.empty());
        auto & last_er = wr->second.back();
        LOG_WARNING("Worker failed while processing events in file "
                    << last_er.ifile << " (" << config->datasets[idataset].files[last_er.ifile].path
                    << "): " << last_er.first << "--" << last_er.last);
        size_t s_before = erm->nevents_left();
        // add all back to erm:
        for(auto & er : wr->second){
            erm->add(er);
        }
        size_t s_after = erm->nevents_left();
        LOG_INFO("Added the " << (s_after - s_before) << " events that the failing worker processed back to the pool to process");
        // erase from the worker_ranges, which should only contain successfull runs in the end:
        worker_ranges.erase(wr);
        // no merging and closing needs to be done for that worker:
        assert(needs_merging.find(worker) == needs_merging.end());
        closed.erase(worker);
        // make sure processing is enabled, now that we have work:
        sm.deactivate_restriction_set(sm.get_graph().get_restriction_set("noprocess"));
    }
    // otherwise, the state was start or configure or stop; in both cases, no events are lost ...
    else{
        assert(last_state == sm.get_graph().get_state("start") || last_state == sm.get_graph().get_state("configure") || last_state == sm.get_graph().get_state("stop"));
    }
}

std::string Master::get_unmerged_filename(int iworker) const {
    stringstream unmerged_outfilename;
    unmerged_outfilename << config->options.output_dir << "/unmerged-" << config->datasets[idataset].name << "-" << iworker << ".root";
    return unmerged_outfilename.str();
}

// last_worker is the worker that last merged files, i.e. the one whose output file contains everything
void Master::finalize_dataset(const WorkerId & last_worker){
    assert(needs_merging[last_worker]);
    string unmerged_filename = get_unmerged_filename(last_worker.id());
    stringstream merged_outfilename;
    merged_outfilename << config->options.output_dir << "/" << config->datasets[idataset].name << ".root";
    int res = rename(unmerged_filename.c_str(), merged_outfilename.str().c_str());
    if(res < 0){
        LOG_ERRNO("renaming output file from '" << unmerged_filename << "' to '" << merged_outfilename.str() << "'");
        throw runtime_error("error renaming merged output");
    }
    // go to next dataset:
    init_dataset(idataset + 1);
}

size_t Master::get_n_unmerged() const{
    return count_if(needs_merging.begin(), needs_merging.end(), [](const pair<const WorkerId, bool> & wm){ return wm.second;});
}


void Master::merge_complete(const WorkerId & worker, std::unique_ptr<Message> result){
    if(stopping) return;
    // we need to merge the file of the merged worker again.
    Merge & merge = dynamic_cast<Merge&>(*result);
    needs_merging[WorkerId(merge.iworker1)] = true;
    auto n_unmerged = get_n_unmerged();
    if(n_unmerged >= 2){
        sm.deactivate_restriction_set(sm.get_graph().get_restriction_set("nomerge"));
    }
    else{
        // n_unmerged == 1
        
        // look whether the merging is done, i.e. all workers that are in the
        // close or merge state are idle. Note that we ignore workers in any state
        // other than close or merge: if asking for all workers to be idle and
        // if a new worker is added and currently active in configure while we reach this point, we could
        // miss being done here ...
        const auto s_close = sm.get_graph().get_state("close");
        const auto s_merge = sm.get_graph().get_state("merge");
        bool merging_done = true;
        auto all_workers = sm.get_workers();
        for(auto & w : all_workers){
            auto state = sm.state(w);
            if(state.first != s_close && state.first != s_merge) continue;
            if(state.second){
                merging_done = false;
                break;
            }
        }
        if(merging_done){
            LOG_INFO("Merge complete for dataset " << config->datasets[idataset].name << "; moving on to the next dataset");
            finalize_dataset(WorkerId(merge.iworker1));
        }
    }
}


void Master::stop_complete(const WorkerId & worker, std::unique_ptr<dc::Message> result){
    LOG_DEBUG("stop complete for worker " << worker.id());
    if(!stopping) return;
    auto all_workers = sm.get_workers();
    bool all_idle = none_of(all_workers.begin(), all_workers.end(), [this](const WorkerId & w){return sm.state(w).second;});
    if(all_idle){
        for(auto & observer : observers){
            observer->on_stop_complete();
        }
    }
}


// generate_merge will only be called if there are at least 2 *unmerged* workers.
std::unique_ptr<Merge> Master::generate_merge(const WorkerId & wid){
    // find another worker to merge with:
    auto mit1 = find_if(needs_merging.begin(), needs_merging.end(), [](const pair<const WorkerId, bool> & wm){ return wm.second;});
    assert(mit1 != needs_merging.end());
    auto mit2 = mit1;
    ++mit2;
    mit2 = find_if(mit2, needs_merging.end(), [](const pair<const WorkerId, bool> & wm){ return wm.second;});
    assert(mit2 != needs_merging.end());
    mit1->second = mit2->second = false;

    // update master status: look if there are another two workers left to merge, otherwise close that path:
    auto n_unmerged = get_n_unmerged();
    if(n_unmerged < 2){
        sm.activate_restriction_set(sm.get_graph().get_restriction_set("nomerge"));
    }
    
    LOG_DEBUG("generate_merge for worker " << wid.id() << ": worker should merge results from workers " << mit1->first.id() << " and " << mit2->first.id());
    return std::unique_ptr<Merge>(new Merge(idataset, mit1->first.id(), mit2->first.id()));
}

void Master::abort(){
    failed_ = true;
    sm.abort();
}

void Master::stop(){
    stopping = true;
    sm.set_target_state(sm.get_graph().get_state("stop"));
}

void Master::close_complete(const WorkerId & worker, std::unique_ptr<Message> result){
    if(stopping) return;
    closed[worker] = true;
    needs_merging[worker] = true;
    bool all_closed = all_of(closed.begin(), closed.end(), [](const pair<const WorkerId, bool> & wc){return wc.second;});
    if(all_closed){
        LOG_INFO("Closing complete for dataset " << config->datasets[idataset].name << "; merging all output files");
        // merge_status of all workers should be 'unmerged' now.
        auto n_unmerged = get_n_unmerged();
        assert(n_unmerged == needs_merging.size());
        // handle the trivial case first:
        if(n_unmerged == 1){
            finalize_dataset(worker);
        }
        else{
            if(config->options.mergemode == s_options::mm_master){
                std::vector<std::string> filenames;
                filenames.reserve(n_unmerged);
                for(auto & w_nm : needs_merging){
                    filenames.emplace_back(get_unmerged_filename(w_nm.first.id()));
                }
                LOG_DEBUG("Merging " << filenames.size() << " output files on master");
                std::string filename0 = filenames[0];
                filenames.erase(filenames.begin());
                assert(!filenames.empty()); // as this was handled in n_unmerged == 1
                ra::merge_rootfiles(filename0, filenames);
                LOG_DEBUG("Merging complete");
                // remove all merged files:
                if(!config->options.keep_unmerged){
                    for(auto & s: filenames){
                        int res = unlink(s.c_str());
                        if(res < 0){
                            LOG_ERRNO_LEVEL(loglevel::warning, "Could not remove merged file (after merging): " << s);
                        }
                    }
                }
                // after merging is complete, move on to next dataset:
                finalize_dataset(needs_merging.begin()->first);
            }
            else{
                // start distributing merge messages:
                sm.deactivate_restriction_set(sm.get_graph().get_restriction_set("nomerge"));
                sm.set_target_state(sm.get_graph().get_state("merge"));
            }
        }
    }
}

void Master::process_complete(const WorkerId & worker, std::unique_ptr<Message> result){
    if(stopping) return;
    LOG_DEBUG("process complete for worker " << worker.id());
    // update file info erm:
    assert(result);
    ProcessResponse & pr = dynamic_cast<ProcessResponse&>(*result);
    nbytes_read_ += pr.nbytes;
    auto wr = worker_ranges.find(worker);
    assert(wr != worker_ranges.end());
    assert(!wr->second.empty());
    auto & last_er = wr->second.back();
    erm->set_file_size(last_er.ifile, pr.file_nevents);
    if(erm->available()){
       sm.deactivate_restriction_set(sm.get_graph().get_restriction_set("noprocess"));
    }
    else{
        // no more work available. Check that all workers are idle (= no worker is active). If they are,
        // go on to the closing stage.
        // Note that waiting until all are idle is not strictly necessary,
        // but enhances robustness: if a worker fails at this stage, recovery entails to distribute the work
        // of the failed worker to other workers, and this required other workers to wait with closing their output ...
        auto all_workers = sm.get_workers();
        bool all_idle = none_of(all_workers.begin(), all_workers.end(), [this](const WorkerId & w){return sm.state(w).second;});
        if(all_idle){
            LOG_INFO("Processing complete for dataset " << config->datasets[idataset].name << "; closing all output files");
            sm.set_target_state(sm.get_graph().get_state("close"));
        }
    }
}

void Master::add_observer(const std::shared_ptr<MasterObserver> & observer){
    if(observer){
        observer->set_master(this);
        sm.add_observer(observer);
        observers.push_back(observer);
    }
}


