#ifndef RA_EVENT2_HPP
#define RA_EVENT2_HPP

#include "fwd.hpp"
#include "identifier.hpp"

#include <boost/optional.hpp>
#include <boost/functional/hash.hpp> // for hash of std::pair

#include <unordered_map>
#include <utility>
#include <typeinfo>
#include <typeindex>
#include <algorithm>
#include <memory>

namespace ra {

/** \brief A generic event container saving arbitrary kind of data
 * 
 * This event container is a replacement of event data structs. It holds a number of named
 * pointers/references which can be accessed via 'set' and 'get' methods. 'set'ting a member
 * allocates memory and marks the member as 'valid'. It can then be accessed via the same name and type via 'get'.
 * 
 * While members are referred to by name, actual access is done in two steps: first, a handle is created using the type and
 * the name pf the member, Then, this handle is used to actually access the data in a second step. This separation
 * is mainly an optimization: The typical use is that handles are created outside the event loop once and then used
 * a lot in the event loop. This means that while creating a new handle (which requires e.g. string manipulation) in general
 * is 'costly'in terms of CPU time, using the handle to set/get data is very 'cheap'.
 * 
 * Elements are identified by their type and name; it is possible to use the same name more than once for different types.
 * The exact same type has to be used for identifying an element in 'get' and 'set'; so it is NOT possible to 'get' via
 * a base class of what has been used to 'set' the data.
 * 
 * To invalidate the value of a data member, call 'set_validity(..., false)'. This marks the member data as invalid
 * and makes a subsequent 'get' fail with an error. This invalidation mechanism is mainly used as consistency check to prevent
 * using outdated data from the previous event: Just before a new event is read, all data is marked as invalid. (While such a mechanism
 * could also be implemented via a "erase" method, that would entail frequent re-allocation in tight loops which is avoided
 * here. Also, erase has other problems, see below.)
 * 
 * Once a data member is 'set', its address is guaranteed to remain the same throughout the lifetime
 * of the Event object. There is also no mechanism to "erase" a data member. In this sense, it is similar to the
 * 'structs' and allows to use this class e.g. with root and TTree::SetBranchAddress (which would be hard or impossible if
 * the member addresses where allowed to change by erasing and re-setting a data member).
 * 
 * In addition to the get/set interface, it is also possible to register a callback via 'set_get_callback' which is
 * called if 'get' is called on an invalid data member. This allows implementing lazy evaluation (or lazy reads) for
 * data. Note that the callback is *not* called for member data in 'valid' state, so it will only called again if the
 * state is reset to 'invalid' again.
 */
class Event;
class Event {
public:
    class RawHandle;
    
    template<typename T>
    class Handle {
        friend class Event;
        uint64_t index;
        Handle<T>(const RawHandle & handle): index(handle.index){}
    public:
        Handle<T>(): index(-1){}
        bool operator==(const Handle<T> & other) const{
            return index == other.index;
        }
    };
    
    class RawHandle {
        friend class Event;
        uint64_t index;
        template<typename T>
        RawHandle(const Handle<T> & h): index(h.index){}
        explicit RawHandle(uint64_t index_): index(index_){}
        
    public:
        RawHandle(): index(-1){}
        bool operator==(const RawHandle & other) const {
            return index == other.index;
        }
    };
    
    enum class state { nonexistent, invalid, valid };
    
    Event(){}
    
    // prevent copying and moving, as we manage memory
    Event(const Event &) = delete;
    Event(Event &&) = delete;
    
    template<typename T>
    Handle<T> get_handle(const std::string & name){
        return Handle<T>(get_raw_handle(typeid(T), name));
    }
    
    RawHandle get_raw_handle(const std::type_info & ti, const std::string & name);
    
    template<typename T>
    std::string name(const Handle<T> & handle){
        return name(RawHandle(handle));
    }
    
    std::string name(const RawHandle & handle);

    /** \brief Get an element of this event
     *
     * If the data member does not exist (state 'nonexistent'), an exception is thrown.
     * 
     * In other states, the behavior depends on 'check_valid':
     * check_valid is true and the member is in state 'invalid', and exception is thrown, otherwise
     * a reference to the member data is returned.
     */
    template<typename T>
    T & get(const Handle<T> & handle, bool check_valid = true){
        return *(reinterpret_cast<T*>(get(typeid(T), handle, check_valid, false)));
    }
    
    template<typename T>
    const T & get(const Handle<T> & handle, bool check_valid = true) const{
        return *(reinterpret_cast<const T*>(get(typeid(T), handle, check_valid, false)));
    }
    
    template<typename T>
    T & get_default(const Handle<T> & handle, T & default_value){
        T * result = reinterpret_cast<T*>(get(typeid(T), handle, true, true));
        if(result == 0){
            return default_value;
        }
        else{
            return *result;
        }
    }
    
    template<typename T>
    const T & get_default(const Handle<T> & handle, const T & default_value) const{
        return const_cast<Event*>(this)->get_default(handle, const_cast<T&>(default_value));
    }
    
    // type-erased versions:
    void * get(const std::type_info & ti, const RawHandle & handle, bool check_valid = true, bool allow_null = false);
    const void * get(const std::type_info & ti, const RawHandle & handle, bool check_valid = true, bool allow_null = false) const;
    
    /** \brief Set an element, allocating memory if necessary
     *
     * The element's new presence state will be 'valid'.
     */
    template<typename T>
    void set(const Handle<T> & handle, T value){
        assert(handle.index < elements.size());
        element & e = elements[handle.index];
        assert(e.type == typeid(T));
        if(e.data != 0){
            *(reinterpret_cast<T*>(e.data)) = std::move(value);
        }
        else{
            e.data = new T(std::move(value));
            e.eraser.reset(new eraser<T>());
        }
        e.valid = true;
    }
    
    /** \brief Install a get callback
     * 
     * The callback is called whenever 'get' is called on an invalid member; after the call,
     * the member state is set to 'valid', i.e. the callback is not called again
     * until the member is marked as 'invalid'.
     * 
     * The data member identified by T and name must exist already. Its state will be set to
     * invalid to trigger the callback on the next call to 'get'.
     */
    template<typename T>
    void set_get_callback(const Handle<T> & handle, boost::optional<std::function<void ()>> callback){
        set_get_callback(typeid(T), handle, std::move(callback));
    }
    
    void set_get_callback(const std::type_info & ti, const RawHandle & handle, boost::optional<std::function<void ()>> callback);
    
    /** \brief Set the validity flag of an element
     * 
     * This can be used to manually change the flag from 'valid' to 'invalid' and vice versa.
     */
    template<typename T>
    void set_validity(const Handle<T> & handle, bool valid){
        set_validity(typeid(T), handle, valid);
    }
    
    void set_validity(const std::type_info & ti, const RawHandle & handle, bool valid);
    
    /** \brief Get the current state of an element
     */
    template<typename T>
    state get_state(const Handle<T> & handle) const{
        return get_state(typeid(T), handle);
    }
    
    state get_state(const std::type_info & ti, const RawHandle & handle) const;
    
    /** \brief Set all member data states to invalid
     * 
     * This is usually called just before reading a new event, to mark all previous
     * data (from the previous event) as invalid to catch unwanted re-use of old data.
     */
    void invalidate_all();
    
    ~Event();
    
private:
    void fail(const std::type_info & ti, const RawHandle & handle, const char * msg) const;
    
    struct eraser_base{
        virtual ~eraser_base(){}
        virtual void operator()(void *) = 0;
    };
    
    template<typename T>
    struct eraser: public eraser_base {
        virtual void operator()(void * p){
            T * t = reinterpret_cast<T*>(p);
            delete t;
        }
    };
    
    // type-erased data member.
    // Life-cycle: it is created upon handle creation with data=0, eraser=0, generator=none, valid=false. At this point, it is considered nonexistent
    // 'set' sets data, eraser, valid=true. Now, it is 'valid'.
    // 'set_validity' can be used to set it to 'invalid' or 'valid' now.
    // 'set_generator' sets generator
    // destructor calls the eraser on data, if eraser is set.
    struct element {
        bool valid;
        std::string name;
        void * data; // managed by eraser. Can be 0 if not 'set' yet, but handle exists
        std::unique_ptr<eraser_base> eraser;
        const std::type_info & type; // points to static global data
        boost::optional<std::function<void ()>> generator;
        
        ~element();
        element(const element &) = delete;
        element(element &&) = default; // note: default is save although this class manages the memory of 'data', as a move will make 'eraser' null, preventing double delete of data.
        void operator=(const element & other) = delete;
        void operator=(element && other) = delete;
        
        // element takes memory ownership of d and e.
        element(bool v, std::string name_, void * d, eraser_base *e, const std::type_info & t, boost::optional<std::function<void ()>> g = boost::none):
           valid(v), name(std::move(name_)), data(d), eraser(e), type(t), generator(std::move(g)){}
    };
    mutable std::vector<element> elements; // note: mutable required for lazy generation of data with const access
};


std::ostream & operator<<(std::ostream & out, Event::state s);

}

#endif
