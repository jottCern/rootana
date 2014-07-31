#ifndef RA_EVENT_HPP
#define RA_EVENT_HPP

#include "fwd.hpp"
#include "identifier.hpp"

#include <boost/optional.hpp>

#include <map>
#include <typeinfo>
#include <typeindex>
#include <algorithm>
#include <memory>

namespace ra {

 
/** \brief A generic event container saving arbitrary kind of data
 * 
 * This event container is a replacement of event data structs. It holds a number of named
 * pointers/references which can be accessed via 'set' and 'get' methods. 'set'ting a member
 * allocates memory and marks the member as 'valid'. It can be accessed with the same name via 'get'.
 * 
 * To invalidate the value of a data member, call 'set_state(..., invalid)'. This marks the member data as outdated
 * and makes a subsequent 'get' fail with an error. This is mainly a means to increase robustness against
 * re-using outdated data while at the same time avoiding frequent re-allocations
 * (see also the get_callback mechnism below for another use case). Once
 * a data member is 'set', its address is guaranteed to remain the same throughout the lifetime
 * of the Event class; in this sense it is similar to the old 'structs' and allows use of this class
 * e.g. with root and TTree::SetBranchAddress.
 * It also allows optimizations by saving the pointer/reference to a member which avoids looking it up frequently.
 *
 * Elements are identified by their type and name; it is possible to use the same name more than once for different types.
 * The exact same type has to be used for identifying an element in 'get' and 'set'; so it is NOT possible to 'get' via
 * a base class of what has been used to 'set' the data.
 * 
 * The data elements managed by the Event container are typically re-used a lot in the event loop with the same elements. To
 * optimize this use case, the Event object avoids frequent de-allocation and allocation by giving each element
 * three distinct states indicating the element's state, "nonexistent", "invalid", and "valid". "nonexistent"
 * means that the Event container has not information about the specified element whatsoever, "invalid" means that memory
 * has been allocated for that member, but no value has been set (yet), and "valid" means that memory has been allocated and
 * a value has been set.
 * 
 * In addition to the get/set interface, it is also possible to register a callback via 'set_get_callback' that is
 * called if 'get' is called on an invalid. This is used for lazy evaluation of member values. Note that the callback
 * is not called for member data in 'valid' state.
 */
class Event {
public:
    
    enum class state { nonexistent, invalid, valid };
    
    Event(): weight_(1.0){}
    
    // prevent copying, and moving, as we manage memory
    Event(const Event &) = delete;
    Event(Event &&) = delete;
    
    double weight() const{
        return weight_;
    }
    
    void set_weight(double w){
        weight_ = w;
    }

    /** \brief Get an element of this event
     *
     * If check_valid is true, the member has to be in state 'valid'; otherwise also 'invalid'
     * data members are returned.
     */
    template<typename T>
    T & get(const identifier & name, bool check_valid = true){
        return *(reinterpret_cast<T*>(get_raw(typeid(T), name, check_valid)));
    }
    
    template<typename T>
    const T & get(const identifier & name, bool check_valid = true) const{
        return *(reinterpret_cast<const T*>(get_raw(typeid(T), name, check_valid)));
    }

    // type-erased versions:
    void * get_raw(const std::type_info & ti, const identifier & name, bool check_present = true);
    const void * get_raw(const std::type_info & ti, const identifier & name, bool check_present = true) const;
    
    /** \brief Set an element, allocating memory if necessary
     *
     * The element's new presence state will be 'valid'.
     */
    template<typename T>
    void set(const identifier & name, T value){
        ti_id key{typeid(T), name};
        auto it = data.find(key);
        if(it!=data.end()){
            *(reinterpret_cast<T*>(it->second.data)) = std::move(value);
            it->second.valid = true;
        }
        else{
            data.insert(std::make_pair(key, element(true, new T(std::move(value)), new eraser<T>(), &typeid(T))));
        }
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
    void set_get_callback(const identifier & name, const std::function<void ()> & callback){
        set_get_callback(typeid(T), name, callback);
    }
    
    void set_get_callback(const std::type_info & ti, const identifier & name, const std::function<void ()> & callback);
    
    
    /** \brief Clear the get callback for the given member
     */
    template<typename T>
    void reset_get_callback(const identifier & name){
        reset_get_callback(typeid(T), name);
    }
    
    /** \brief Set the state flag of an element
     * 
     * This can be used to manully change the flag from 'valid' to 'invalid' and vice versa.
     * Note that setting from/to nonexistent is not allowed and results in a invalid_argument exception.
     */
    template<typename T>
    void set_state(const identifier & name, state s){
        set_state(typeid(T), name, s);
    }
    
    /** \brief Get the current state of an element
     */
    template<typename T>
    state get_state(const identifier & name) const{
        return get_state(typeid(T), name);
    }
    
    void set_state(const std::type_info & ti, const identifier & name, state p);
    state get_state(const std::type_info & ti, const identifier & name) const;
    
    /** \brief Set all member data states to invalid and reset the weight to 1.0
     * 
     * This is usually called just before reading a new event, to mark all previous
     * data (from the previous event) as invalid to catch unwanted re-use of old data.
     */
    void invalidate_all();
    
    ~Event();
    
private:
    
    void set_raw(const std::type_info & ti, const identifier & name, void * obj, const std::function<void (void*)> & eraser);
    void reset_get_callback(const std::type_info & ti, const identifier & name);
    void fail(const std::type_info & ti, const identifier & id) const;
    
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
    
    struct eraser_f_wrapper: public eraser_base{
        std::function<void (void*)> f;
        
        explicit eraser_f_wrapper(const std::function<void (void*)> & f_): f(f_){}
        
        virtual void operator()(void * p){
            f(p);
        }
    };
    
    // type-erased data member:
    struct element {
        bool valid;
        void * data; // managed by eraser
        std::unique_ptr<eraser_base> eraser;
        const std::type_info * type; // points to static global data
        boost::optional<std::function<void ()>> generator;
        
        ~element();
        element(const element &) = delete;
        element(element &&);
        void operator=(const element & other) = delete;
        void operator=(element && other) = delete;
        
        // element takes memory ownership of d and e.
        element(bool v, void * d, eraser_base *e, const std::type_info *t, boost::optional<std::function<void ()>> g = boost::none):
           valid(v), data(d), eraser(e), type(t), generator(std::move(g)){}
    };
    
    typedef std::pair<std::type_index, identifier> ti_id;
    
    double weight_;
    mutable std::map<ti_id, element> data; // note: mutable required for lazy generation of data with const access
};

std::ostream & operator<<(std::ostream & out, Event::state s);

}

#endif
