#pragma once

#include <typeinfo>
#include <typeindex>
#include <algorithm>
#include <memory>

namespace ra {
   

/** \brief A generic, extensible event container for saving arbitrary kind of data
 * 
 * This event container is a replacement of event data structs and allows to dynamically define
 * the actual 'content' of the struct.
 * 
 * Using this class is split in two steps: First, the structure of the event container is defined via EventStructure.
 * Using this structure, an actual Event is created. This separation corresponds to the declaration of a struct to
 * actually defining one and allocating memory for it. It also has the (wanted) side-effect that all members have to be declared
 * at an early point of the program, in particular outside of any tight loops.
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
 * In addition to the get/set interface, it is also possible to register a callback via 'set_get_callback' which is
 * called if 'get' is called on an invalid data member. This allows implementing lazy evaluation (or lazy reads) for
 * data. Note that the callback is *only* called when accessing members in 'invalid' state. This effectively caches
 * the result of the callback.
 * 
 * Implementation note: most methods are implemented twice, once as template methods taking the type of the data member
 * as template argument and once as a 'raw' version using a runtime type_info as additional argument instead of a compile-time
 * template argument. Users usually only need the templated version; the 'raw' versions are needed by the framework to support
 * setting members whose type is not known at compile time but only at run time. Also, this avoids template code bloat as templated
 * versions of the methods just forward the call to the 'raw' ones.
 */
class EventStructure {
friend class Event;
friend class MutableEvent;
public:
    class RawHandle;
    class HandleAccess_; // only for framework use!
    
    template<typename T>
    class Handle {
        friend class HandleAccess_;
        int64_t index = -1;
        explicit constexpr Handle(int64_t index_): index(index_){}
    public:
        constexpr Handle() = default;
        bool operator==(const Handle & rhs) const {
            return index == rhs.index;
        }
    };
    
    class RawHandle {
        friend class HandleAccess_;
        int64_t index = -1;
        explicit constexpr RawHandle(int64_t index_): index(index_){}
    public:
        constexpr RawHandle() = default;
        bool operator==(const RawHandle & rhs) const {
            return index == rhs.index;
        }
    };
        
    template<typename T>
    Handle<T> get_handle(const std::string & name){
        return HandleAccess_::create_handle<T>(get_raw_handle(typeid(T), name));
    }
    
    RawHandle get_raw_handle(const std::type_info & ti, const std::string & name);
    
    
    // get the name for the data member of a handle.
    template<typename T>
    std::string name(const Handle<T> & handle){
        return name(HandleAccess_::create_raw_handle(handle));
    }
    
    std::string name(const RawHandle & handle);
    
    const std::type_info & type(const RawHandle & handle) const;
    
    // number of members defined so far
    size_t size() const{
        return member_infos.size();
    }
    
    
    // below here: framework internals!
    
    // HandleAccesss_ accesses the Handle internals and is only supposed to be used by the framework
    // not by the analysis user.
    class HandleAccess_ {
    public:
        
        template<typename HT>
        inline static constexpr int64_t index(const HT & handle);
        
        // create handle from another handle or index HT:
        template<typename T, typename HT>
        inline static constexpr Handle<T> create_handle(const HT & handle_or_index);
        
        template<typename HT>
        inline static constexpr RawHandle create_raw_handle(const HT & handle_or_index);
    };
    
private:
    struct member_info {
        std::string name;
        const std::type_info & type; // points to static global data
        
        member_info(const std::string & name_, const std::type_info & ti): name(name_), type(ti){}
    };
    std::vector<member_info> member_infos;
};



class Event {
public:
    enum class state { nonexistent, invalid, valid };
    
    typedef EventStructure::RawHandle RawHandle;
    template<class T> using Handle = EventStructure::Handle<T>;
    using HandleAccess_ = EventStructure::HandleAccess_;
    
    explicit Event(const EventStructure &);
    
    // prevent copying and moving, as we manage memory
    Event(const Event &) = delete;
    Event(Event &&) = delete;
    
    /** \brief Get an element of this event
     *
     * If the data member does not exist (state 'nonexistent'), an exception is thrown.
     * 
     * In other states, the behavior depends on 'check_valid':
     * If check_valid is true and the member is in state 'invalid', and exception is thrown, otherwise
     * a reference to the (invalid) member data is returned.
     * 
     * In case a callback is installed and the member is in state 'invalid' before the call to 'get',
     * the callback is called and the state is set to valid. Note that the callback is called even if check_valid
     * is false.
     */
    template<typename T>
    T & get(const Handle<T> & handle, bool check_valid = true){
        return *(reinterpret_cast<T*>(get(typeid(T), EventStructure::HandleAccess_::create_raw_handle(handle), check_valid ? state::valid : state::invalid)));
    }
    
    template<typename T>
    const T & get(const Handle<T> & handle, bool check_valid = true) const{
        return *(reinterpret_cast<const T*>(get(typeid(T), EventStructure::HandleAccess_::create_raw_handle(handle), check_valid ? state::valid : state::invalid)));
    }
    
    // get with default, in case it is not valid.
    template<typename T>
    T & get_default(const Handle<T> & handle, T & default_value){
        if(get_state(handle) != state::valid){
            return default_value;
        }
        else{
            return get<T>(handle);
        }
    }
    
    template<typename T>
    const T & get_default(const Handle<T> & handle, const T & default_value) const{
        return const_cast<Event*>(this)->get_default(handle, const_cast<T&>(default_value));
    }
    
    void * get(const std::type_info & ti, const RawHandle & handle, state minimum_state = state::valid);
    const void * get(const std::type_info & ti, const RawHandle & handle, state minimum_state = state::valid) const;
    
    /** \brief Set an element, allocating memory if necessary
     *
     * The element's new presence state will be 'valid'.
     */
    template<typename T, typename U>
    void set(const Handle<T> & handle, U && value){
        check(typeid(T), HandleAccess_::create_raw_handle(handle), "set");
        auto index = HandleAccess_::index(handle);
        member_data & md = member_datas[index];
        if(md.data != 0){
            *(reinterpret_cast<T*>(md.data)) = std::forward<U>(value);
        }
        else{
            md.data = new T(std::forward<U>(value));
            md.eraser = [](void * ptr){ delete reinterpret_cast<T*>(ptr);};
        }
        md.valid = true;
    }
    
    // set a data member via the handle to the given pointer. Only valid if the member was nonexistent so far (nullptr).
    void set(const std::type_info & ti, const RawHandle & handle, void * data, const std::function<void (void*)> & eraser);
    
    /** \brief Install a get callback
     * 
     * The callback is called whenever 'get' is called on an invalid member; after that call
     * to the callback, the member's state is set to 'valid', i.e. the callback is not called again
     * until the member is marked as 'invalid'.
     * 
     * The data member identified by T and name must exist already. Its state will be set to
     * invalid to trigger the callback on the next call to 'get'.
     */
    template<typename T>
    void set_get_callback(const Handle<T> & handle, const std::function<void ()> & callback){
        set_get_callback(typeid(T), EventStructure::HandleAccess_::create_raw_handle(handle), callback);
    }
    
    void set_get_callback(const std::type_info & ti, const RawHandle & handle, const std::function<void ()> & callback);
    
    /** \brief Set the validity flag of an element
     * 
     * This can be used to manually change the flag from 'valid' to 'invalid' and vice versa.
     */
    template<typename T>
    void set_validity(const Handle<T> & handle, bool valid){
        set_validity(typeid(T), EventStructure::HandleAccess_::create_raw_handle(handle), valid);
    }
    
    void set_validity(const std::type_info & ti, const RawHandle & handle, bool valid);
    
    // without type checking:
    void set_validity(const RawHandle & handle, bool valid);
    
    /** \brief Get the current state of an element
     */
    template<typename T>
    state get_state(const Handle<T> & handle) const{
        return get_state(typeid(T), EventStructure::HandleAccess_::create_raw_handle(handle));
    }
    
    state get_state(const std::type_info & ti, const RawHandle & handle) const;
    
    /** \brief Set all member data states to invalid
     * 
     * This is usually called just before reading a new event, to mark all previous
     * data (from the previous event) as invalid to catch unwanted re-use of old data.
     */
    void invalidate_all();
    
    ~Event();
    
    
    // forward calls to EventStructure:
    template<typename T>
    std::string name(const Handle<T> & handle){
        return structure.name(handle);
    }
    
    std::string name(const RawHandle & handle){
        return structure.name(handle);
    }
    
    const std::type_info & type(const RawHandle & handle) const{
        return structure.type(handle);
    }
    
    size_t size() const{
        return structure.size();
    }
    
protected:
    Event(){}
    
    // check that the supplied handle is valid and fail if it's not. Optionally with
    // type checking.
    void check(const std::type_info & ti, const RawHandle & handle, const std::string & where) const;
    void check(const RawHandle & handle, const std::string & where) const;
    
    // raise a runtime error
    void fail(const std::type_info & ti, const RawHandle & handle, const std::string & msg) const;
    void fail(const RawHandle & handle, const std::string & msg) const;
    
    
    // type-erased data member.
    // Life-cycle: it is created upon Event creation with data=0, eraser=0, generator=none, valid=false. At this point, it is considered 'nonexistent'
    // 'set' sets data and eraser, valid=true. Now, it is 'valid'.
    // 'set_validity' can be used to set it to 'invalid' or 'valid' now.
    // 'set_get_callback' sets generator
    // destructor calls the eraser on data, if eraser is set.
    struct member_data {
        mutable bool valid = false;
        void * data = nullptr; // managed by eraser. Can be 0 if not 'set' yet, but handle exists, or if unmanaged.
        std::function<void (void*)> eraser;
        std::function<void ()> generator;
        
        ~member_data();
        member_data(const member_data &) = delete;
        member_data(member_data &&) = default; // note: default is save although this class manages the memory of 'data', as a move will make 'eraser' null, preventing double delete of data.
        void operator=(const member_data & other) = delete;
        void operator=(member_data && other) = delete;
        member_data(){}
    };
    EventStructure structure;
    std::vector<member_data> member_datas;
};


// an event which can change structure after creation.
// (Note that having both Event and MutableEvent is mainly to be able to use Event to enforce a policy
// of first declaring everything in a setup phase before the processing phase, while using
// MutableEvent if such a policy is not enforced).
class MutableEvent: public Event {
public:
    using Event::Event;
    
    MutableEvent(){}
    
    template<typename T>
    Handle<T> get_handle(const std::string & name){
        return EventStructure::HandleAccess_::create_handle<T>(get_raw_handle(typeid(T), name));
    }
    
    RawHandle get_raw_handle(const std::type_info & ti, const std::string & name);
};

std::ostream & operator<<(std::ostream & out, Event::state s);


template<typename HT>
constexpr int64_t EventStructure::HandleAccess_::index(const HT & handle){
    return handle.index;
}
        
template<>
constexpr int64_t EventStructure::HandleAccess_::index<int64_t>(const int64_t & index_){
    return index_;
}
        
// create handle from another handle or index HT:
template<typename T, typename HT>
constexpr EventStructure::Handle<T> EventStructure::HandleAccess_::create_handle(const HT & handle_or_index){
    return EventStructure::Handle<T>{index(handle_or_index)};
}

template<typename HT>
constexpr EventStructure::RawHandle EventStructure::HandleAccess_::create_raw_handle(const HT & handle_or_index){
    return EventStructure::RawHandle{index(handle_or_index)};
}

}
