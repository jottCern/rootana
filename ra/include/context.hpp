#ifndef RA_CONTEXT_HPP
#define RA_CONTEXT_HPP

#include <string>
#include <type_traits>
#include <typeinfo>

#include "fwd.hpp"
#include "event.hpp"

namespace ra{
   

/** \brief Manage Event input from some source.
 * 
 * Typical usage is in AnalysisModule::start_dataset:
 * \code
 * // in the class declaration of the AnalysisModule:
 * Event::Handle<int> handle;
 * 
 * // in begin_dataset:
 * handle = in.declare_input<int>("my_int_branch");
 * \endcode
 * 
 * This will read a branch named "my_int_branch" in the event input tree and make it
 * available in the event container via \c handle (which is usually a class member and
 * not localy defined).
 * 
 * The content of this can then be retrieved later in AnalysisModule::process:
 * \code
 *  int myint = event.get(handle);
 * \endcode
 */
class InputManager {
public:
    virtual ~InputManager();
    
    /** \brief Declare an input variable in the event tree
     */
    template<typename T>
    Event::Handle<T> declare_event_input(const std::string & bname, const std::string & ename_ = ""){
        static_assert(!std::is_pointer<T>::value, "T must not be of pointer type");
        const auto & ename = ename_.empty() ? bname : ename_;
        do_declare_event_input(typeid(T), bname, ename);
        return es.get_handle<T>(ename);
    }
    
    template<typename T>
    Event::Handle<T> get_handle(const std::string & ename){
        return es.get_handle<T>(ename);
    }
    
    Event::RawHandle get_raw_handle(const std::type_info & ti, const std::string & name){
        return es.get_raw_handle(ti, name);
    }
    
    // "low-level" access; addr has to point to a structure of type ti, which has not necessarily to be in the Event container.
    // If the data is not stored in the Event container, use event_member_name="".
    virtual void do_declare_event_input(const std::type_info & ti, const std::string & bname, const std::string & mname) = 0;
    
protected:
    
    explicit InputManager(EventStructure & es_): es(es_){}
    
    EventStructure & es;
};


/** \brief Utility class for declaring histogram output
 */
class HistogramOutputManager {
public:
    
    /** \brief Put a histogram in the output root file at the specified path
    *
    * By calling this routine, you pass memory management of t to the HistogramOutputManager.
    *
    * To create the object in a subdirectory of the output, use a name with the corresponding
    * full name, i.e. "dir/subdirB/name"; the required directories will be created automatically.
    */
    virtual void put(const char * name, TH1 * t) = 0;
    
    virtual ~HistogramOutputManager();
};


/** \brief Utility class for performing event data and histogram output to the output file
 *
 * An instance of this class is passed to the AnalysisModule, where the user can declare event and histogram output.
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
     * T must not be of pointer type.
     */
    template<typename T>
    Event::Handle<T> declare_event_output(const std::string & bname, const std::string & ename_ = ""){
        static_assert(!std::is_pointer<T>::value, "T must not be of pointer type");
        const auto & ename = ename_.empty() ? bname : ename_;
        declare_event_output(typeid(T), bname, ename);
        return es.get_handle<T>(ename);
    }
    
    /** \brief Declare a non-event variable which should be written to the tree identified by tree_id instead.
     *
     * T must not be of pointer type. Make sure that the lifetime of the reference t exceeds the last call of write_output of the corresponding tree
     */
    template<typename T>
    void declare_output(const identifier & tree_id, const char * name, T & t){
        static_assert(!std::is_pointer<T>::value, "T must not be of pointer type");
        declare_output(typeid(T), tree_id, name, static_cast<const void*>(&t));
    }
    
    template<typename T>
    Event::Handle<T> get_handle(const std::string & ename){
        return es.get_handle<T>(ename);
    }
    
    /// Write a single entry to the output tree identified by tree_id with the current contents of the declared output addresses.
    virtual void write_output(const identifier & tree_id) = 0;
    
    /// declare the destructor virtual, as this is a purely virtual base class.
    virtual ~OutputManager();
    
    // low-level access:
    virtual void declare_output(const std::type_info & ti, const identifier & tree_id, const std::string & branchname, const void * t) = 0;
    virtual void declare_event_output(const std::type_info & ti, const std::string & bname, const std::string & mname) = 0;
    
protected:
    explicit OutputManager(EventStructure & es_): es(es_){}
    EventStructure & es;
};

}

#endif

