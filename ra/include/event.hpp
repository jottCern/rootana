#ifndef RA_EVENT_HPP
#define RA_EVENT_HPP

#include "fwd.hpp"
#include "identifier.hpp"

#include <map>
#include <typeinfo>
#include <typeindex>
#include <algorithm>

namespace ra {

 
/** \brief A generic event container saving arbitrary kind of data
 *
 * The data saved in this container have to be movable types. Elements are identified by their type and name; it is possible
 * to use the same name more than once for different types.
 * Note that the exact same type has to be used for identifying an element; it is NOT possible to 'get' via
 * a base class of what has been used to 'set' the data.
 * The name is basically a string, but as frequent access to the same name happens, this is optimized by using the "id construct" instead
 * (see identifier.hpp).
 * 
 * The data elements managed by the Event container are typically re-used a lot in the event loop with the same elements. To
 * optimize this use case, the Event object avoids frequent de-allocation and allocation by giving each element
 * three distinct states indicating the element's presence, "nonexistent", "allocated", and "present". "nonexistent"
 * means that the Event container has not information about the specified element whatsoever, "allocated" means that memory
 * has been allocated for that member, but no value has been set (yet), "present" means that memory has been allocated and
 * a value has been set.
 */
class Event {
public:
    
    enum class presence { nonexistent, allocated, present };
    
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
     * The member has to be allocated or present. If check_present is true and the member is not 
     */
    template<typename T>
    T & get(const identifier & name, bool check_present = true){
        return *(reinterpret_cast<T*>(get_raw(typeid(T), name, check_present)));
    }
    
    template<typename T>
    const T & get(const identifier & name, bool check_present = true) const{
        return *(reinterpret_cast<const T*>(get_raw(typeid(T), name, check_present)));
    }
    
    /// Get using raw pointers
    void * get_raw(const std::type_info & ti, const identifier & name, bool check_present = true);
    const void * get_raw(const std::type_info & ti, const identifier & name, bool check_present = true) const;
    
    /** \brief Set an element, allocating memory if necessary
     *
     * The element's new presence state will be present.
     */
    template<typename T>
    void set(const identifier & name, T value){
        ti_id key{typeid(T), name};
        auto it = data.find(key);
        if(it!=data.end()){
            *(reinterpret_cast<T*>(std::get<1>(it->second))) = std::move(value);
            std::get<0>(it->second) = true;
        }
        else{
            data[key] = std::make_tuple<bool, void*, eraser_base*>(true, new T(std::move(value)), new eraser<T>());
        }
    }
    
    
    void set_raw(const std::type_info & ti, const identifier & name, void * obj, const std::function<void (void*)> & eraser);
    
    
    /** \brief Set the presence flag of an element
     * 
     * The parameter \c present determines the new presence status of the element identified by type T name \c name.
     * 
     * Setting the presence to nonexistent will de-allocate the associated memory.
     * 
     * An attempt to mark a non-existent element as allocated or present results in a runtime_error; all other cases will work.
     */
    template<typename T>
    void set_presence(const identifier & name, presence p){
        set_presence(typeid(T), name, p);
    }
    
    void set_presence(const std::type_info & ti, const identifier & name, presence p);
    
    // set all to the allocated state and the weight to the default of 1.0. This
    // is typically done right before populating the container with new data.
    void reset_all(){
        for(auto & it : data){
            std::get<0>(it.second) = false;
        }
        weight_ = 1.0;
    }

    template<typename T>
    presence get_presence(const identifier & name) const{
        return get_presence(typeid(T), name);
    }
    
    presence get_presence(const std::type_info & ti, const identifier & name) const;
    
    ~Event();
    
private:
    double weight_;
    
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
    
    typedef std::pair<std::type_index, identifier> ti_id;
    typedef std::tuple<bool, void*, eraser_base*> element; // tuple of (present, data, eraser)
    std::map<ti_id, element> data;
};

std::ostream & operator<<(std::ostream & out, Event::presence p);

}

#endif
