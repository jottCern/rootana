#ifndef RA_CONTEXT_HPP
#define RA_CONTEXT_HPP

#include <map>
#include <list>
#include <string>
#include <type_traits>
#include <typeinfo>

#include "fwd.hpp"
#include "event.hpp"

namespace ra{
   

/** \brief Manage Event input from a TTree
 * 
 * Typical usage is in AnalysisModule::start_dataset:
 * \code
 * in.declare_input<int>("my_int_branch");
 * \endcode
 * 
 * The content of this can then be retrieved later in AnalysisModule::process:
 * \code
 *  event.get<int>("my_int_branch");
 * \endcode
 */
class InputManager{
public:
    virtual ~InputManager();
    
    /** \brief Declare an input variable in the event tree
     */
    template<typename T>
    void declare_event_input(const char * branchname, const std::string & event_member_name){
        auto h = event.get_handle<T>(event_member_name);
        event.set(h, T());
        void * addr = &(event.get(h));
        event.set_validity(h, false);
        declare_input(branchname, event_member_name, addr, typeid(T));
    }
    
    template<typename T>
    void declare_event_input(const char * name){
        declare_event_input<T>(name, name);
    }
    
    template<typename T>
    Event::Handle<T> get_handle(const std::string & name){
        return event.get_handle<T>(name);
    }
    
protected:
    // "low-level" access; addr has to point to a structure of type ti, which has not necessarily to be in the Event container.
    // If the data is not stored in the Event container, use event_member_name="".
    virtual void declare_input(const char * bname, const std::string & event_member_name, void * addr, const std::type_info & ti) = 0;
    
    explicit InputManager(Event & event_): event(event_){}
    Event & event;
};



// This is the actual InputManager class usuall used in the framework: TODO: remove from here?
class TTreeInputManager: public InputManager{
public:
    virtual void declare_input(const char * bname, const std::string & event_member_name, void * addr, const std::type_info & ti);
    
    explicit TTreeInputManager(Event & event_, bool lazy_ = false): InputManager(event_), nentries(0), current_ientry(-1), bytes_read(0), lazy(lazy_){}
    
    // call SetBranchAddress for all saved 'declare_input' calls.
    void setup_tree(TTree * tree);
    
    // returns the number of read bytes (uncompressed, as by TBranch::GetEntry).
    void read_entry(size_t ientry);
    
    // get the number of bytes read since the last time this function was called
    size_t nbytes_read(){
        size_t result = bytes_read;
        bytes_read = 0;
        return result;
    }
    
    // the number of entries in the setup tree
    size_t entries() const{
        return nentries;
    }
    
private:
    struct branchinfo {
        TBranch * branch;
        const std::type_info & ti; // this is always a non-pointer type.
        Event::RawHandle handle; // can be an invalid handle in case addr is outside the event container
        void * addr; // address of an object of type ti, either into the event container or to somewhere else
        
        branchinfo(TBranch * branch_, const std::type_info & ti_, const Event::RawHandle & handle_, void * addr_): branch(branch_), ti(ti_), handle(handle_), addr(addr_){}
    };
    
    void read_branch(branchinfo & bi);
    
    size_t nentries;
    size_t current_ientry;
    size_t bytes_read;
    std::map<std::string, branchinfo> bname2bi;
    bool lazy;
};

/** \brief Utility class for declaring histogram output
 */
class HistogramOutputManager {
public:
    /** \brief Put a histogram in the output root file at the specified path
    *
    * By calling this routine, you pass memory management of t to the framework.
    *
    * To create the object in a subdirectory of the output, use an id with the corresponding
    * full name, i.e. "dir/subdirB/name"; the required directories will be created automatically.
    */
    virtual void put(const char * name, TH1 * t) = 0;
    
    virtual ~HistogramOutputManager();
};


/** \brief Utility class for performing TTree and histogram output to a rootfile
 *
 * An instance of this class is passed to the AnalysisModule, where the user can declare tree and histogram output.
 *
 * There are two types of trees: event trees and user-defined trees. Event trees are managed by the framework
 * in the sense that each entry in the input event tree is processed once (by calling AnalysisModule::process)
 * and each selected event is written to the output event tree.
 * 
 * User-defined trees, on the other hand, only exist as output trees and can contain arbitrary, non-event data. An example
 * would be to fill data on a per-object basis. To use user-defined trees, call the 'declare_output' method, and
 * for each entry, call the 'write_output' method.
 */
class OutputManager: public HistogramOutputManager {
public:

    /** \brief Declare an output variable in the event tree
     *
     * The variable has to exist in the Event before calling this method.
     */
    template<typename T>
    void declare_event_output(const char * branchname, const std::string & event_member_name){
        auto h = event.get_handle<T>(event_member_name);
        if(event.get_state(h) == Event::state::nonexistent){
            event.set(h, T());
        }
        const T & t = event.get(h, false);
        declare_event_output(branchname, event_member_name, static_cast<const void*>(&t), typeid(T));
    }
    
    // shortcut if branchname and member name are the same
    template<typename T>
    void declare_event_output(const char * name){
        declare_event_output<T>(name, name);
    }
    
    /** \brief Declare a non-event variable which should be written to the tree identified by tree_id instead.
     *
     * T must not be of pointer type. Make sure that the lifetime of t exceeds the last call of write_output of the corresponding tree
     */
    template<typename T>
    void declare_output(const identifier & tree_id, const char * name, T & t){
        static_assert(!std::is_pointer<T>::value, "T must not be of pointer type");
        declare_output(tree_id, name, static_cast<const void*>(&t), typeid(T));
    }
    
    /// Write a single entry to the output tree identified by tree_id with the current contents of the declared output addresses.
    virtual void write_output(const identifier & tree_id) = 0;
    
    /// declare the destructor virtual, as this is a purely virtual base class.
    virtual ~OutputManager();
    
    // low-level access:
    virtual void declare_event_output(const char * branchname, const std::string & event_member_name, const void * addr, const std::type_info & ti) = 0;
    virtual void declare_output(const identifier & tree_id, const char * branchname, const void * t, const std::type_info & ti) = 0;
    
protected:
    explicit OutputManager(Event & event_): event(event_){}
    Event & event;
};


// this is the framework part ...
class TFileOutputManager: public OutputManager {
public:
    virtual void put(const char * name, TH1 * t);
    virtual void declare_event_output(const char * name, const std::string & event_member_name, const void * addr, const std::type_info & ti);
    virtual void declare_output(const identifier & tree_id, const char * name, const void * t, const std::type_info & ti);
    virtual void write_output(const identifier & tree_id);
    
    void write_event();
    
    // write and close the underlying TFile. Any put or write after that is illegal.
    // Called from the destructor automatically, so often no need to do it explicitly.
    void close();
    
    // outfile ownership is taken by the TFileOutputManager.
    TFileOutputManager(std::unique_ptr<TFile> outfile, const std::string & event_treename, Event & event);
    
    ~TFileOutputManager();
    
private:
    std::unique_ptr<TFile> outfile;
    std::string event_treename;
    TTree * event_tree; // owned by outfile
    std::vector<std::pair<Event::RawHandle, const std::type_info*>> event_members; // list of event members to read before the write; important for lazy read
    std::map<identifier, TTree*> trees; // additional trees beyond the event tree
    std::list<void*> ptrs; // keep a list of pointers, so we can give root the *address* of the pointer
};

}

#endif

