#ifndef RA_ROOT_UTILS_HPP
#define RA_ROOT_UTILS_HPP

#include "ra/include/event.hpp"

#include "TFile.h"
#include "TTree.h"

#include <string>
#include <memory>
#include <stdexcept>
#include <cassert>
#include <list>


namespace ra {

// merge all rootfiles into file1. file1 has to exist (it will be updated during the merge).
// all files must have the same keys.
void merge_rootfiles(const std::string & file1, const std::vector<std::string> & rhs_filenames);


// get a (copy of a) histogram from an open root file, with error checking and readable error messages
template<typename T>
inline std::unique_ptr<T> gethisto(TFile & infile, const std::string & name){
    assert(infile.IsOpen());
    auto * t = infile.Get(name.c_str());
    if(!t){
        throw std::runtime_error("did not find '" + name + "' in file '" + infile.GetName() + "'");
    }
    T * tmpt = dynamic_cast<T*>(t);
    if(!tmpt){
        throw std::runtime_error("found '" + name + "' in file '" + infile.GetName() + "' but it has wrong type");
    }
    std::unique_ptr<T> result(static_cast<T*>(tmpt->Clone()));
    result->SetDirectory(0);
    return result;
}



/** \brief Utility for reading a TTree into an Event container.
 * 
 * Wrapper for TTree with more consistent and type-erased interface for reading
 * the contents of the TTree into an Event (or MutableEvent) container.
 * 
 * The central method is open_branch, which 'connects' a branch in the input file to
 * a event member, where 'connect' means that a call to 'get_entry' populates the data
 * of that event member.
 * 
 * There are 4 open_branch methods to implement several scenarios:
 *  - in case a MutableEvent is available, a new member is created based on the event member name
 *  - in case a no MutableEvent (and only Event) is available, a Event::RawHandle is used to identify the event member
 * 
 * Those two exist either as templates or with a std::type_info as argument. 
 *
 */ 
class InTree {
public:
    explicit InTree(TTree * tree_, bool lazy = true);
    
    InTree(const InTree &) = delete;
    InTree(InTree &&) = default;
    
    template<typename T>
    void open_branch(const std::string & branchname, MutableEvent & event, const std::string & event_member_name = ""){
        open_branch(typeid(T), branchname, event, event_member_name);
    }
    
    void open_branch(const std::type_info & ti, const std::string & branchname, MutableEvent & event, const std::string & event_member_name = ""){
        auto handle = event.get_raw_handle(ti, event_member_name.empty() ? branchname : event_member_name);
        open_branch(ti, branchname, event, handle);
    }
    
    template<typename T>
    void open_branch(const std::string & branchname, Event & event, const Event::Handle<T> & handle){
        open_branch(typeid(T), branchname, event, handle);
    }
    
    void open_branch(const std::type_info & ti, const std::string & branchname, Event & event, const Event::RawHandle & handle);
    
    // prepare reading the given entry; either reads immediately all data from opened branches or
    // (if lazy=true) prepares the callbacks which trigger this read only on event.get.
    void get_entry(int64_t index);
    
    int64_t get_entries() const {
        return n_entries;
    }
    
    // get number of bytes read since last call to this method (or construction if never called),
    // and reset that counter.
    int64_t get_reset_bytes_read() {
        auto res = bytes_read;
        bytes_read = 0;
        return res;
    }
    
private:
    TTree * tree;
    bool lazy;
    std::list<void*> ptrs;
    
    int64_t current_index = -1;
    int64_t n_entries;
    int64_t bytes_read = 0;
    
    // save info about all opened branches:
    struct binfo {
        std::string branchname;
        TBranch * branch = nullptr;
        Event & event;
        Event::RawHandle handle;
        
        explicit binfo(const std::string & branchname_, TBranch * branch_, Event & event_, const Event::RawHandle & handle_):
            branchname(branchname_), branch(branch_), event(event_), handle(handle_){}
    };
    
    std::list<binfo> branch_infos; // make as list to keep references to elements valid all the time (needed for Event callback of set_get_callback)
    
    void read_branch(const binfo & bi);
};


}

#endif
