#ifndef DRA_EVENT_HPP
#define DRA_EVENT_HPP

#include "fwd.hpp"
#include "analysis.hpp"
#include "identifier.hpp"

#include <map>
#include <typeinfo>
#include <typeindex>
#include <algorithm>

namespace ra {
    
/** \brief A generic event container saving arbitrary kind of data
 *
 * Saves members of a movable type. Members are identified by their type and name, i.e. it is possible
 * to use the same name twice if the type is different.
 * 
 * Note that you have to use the exact same type for setting and getting; in other words: it is NOT possible to 'get' via
 * a base class of what has been used to 'set' the data.
 * 
 * The data managed by the Event container are typically re-used in the event loop. In order to reliably detect cases
 * in which outdated data (from the previous event) is accessed, without the need of frequent memory-reallocation, some care
 * is required to manage the lifecycle of the data in the Event.
 * 
 * The rules are:
 *  - a data member can be non-existent, (only) "allocated" and "present"
 *  - Usually, only a distinction between "present" and the other states is necessary.
 *    The "set", "get", and "present" routines provide this interface: they will make stuff present, get present stuff and test
 *    on presence, respectively.
 *  - Low-level routines "set_raw", "get_raw" and "allocated" only distinguish between the non-existent and the rest of the states;
 *     "raw" in general refers to dealing with "raw pointers" and according low-level memory allocation primitives (instead of objects like in set/get).
 *  - To remove a data member completely, use "erase"; to mark a "present" element as (only) "allocated", use "unset".
 */
class Event {
public:
    
    Event(): weight_(1.0){}
    
    double weight() const{
        return weight_;
    }
    
    void set_weight(double w){
        weight_ = w;
    }
    
    /** \brief Get a member of this event
     *
     * If no member exists with that name, throws a invalid_argument exception.
     */
    template<typename T>
    T & get(const identifier & name){
        return *(reinterpret_cast<T*>(get_raw2(typeid(T), name, true)));
    }
    
    template<typename T>
    const T & get(const identifier & name) const{
        return *(reinterpret_cast<const T*>(get_raw2(typeid(T), name, true)));
    }
    
    // get a reference to the allocated (but not necessarily present) data member. Fails if it's not allocated.
    template<typename T>
    T & get_allocated(const identifier & name){
        return *(reinterpret_cast<T*>(get_raw2(typeid(T), name, false)));
    }
    
    void * get_raw(const std::type_info & ti, const identifier & name){
        return get_raw2(ti, name, false);
    }
    
    const void * get_raw(const std::type_info & ti, const identifier & name) const{
        return get_raw2(ti, name, false);
    }
    

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
    
    // eraser(obj) is called in the Event destructor
    void set_raw(const std::type_info & ti, const identifier & name, void * obj, const std::function<void (void*)> & eraser);
    
    
    // unset marks a data member as "not present". It is a noop if
    // the data member does not exist or has already been marked as not present.
    template<typename T>
    void unset(const identifier & name){
        unset(typeid(T), name);
    }
    
    void unset(const std::type_info & ti, const identifier & name){
        auto it = data.find(ti_id{ti, name});
        if(it!=data.end()){
            std::get<0>(it->second) = false;
        }
    }
    
    void unset_all(){
        for(auto & it : data){
            std::get<0>(it.second) = false;
        }
        weight_ = 1.0;
    }
    
    template<typename T>
    void set_present(const identifier & name){
        set_present(typeid(T), name);
    }
    
    void set_present(const std::type_info & ti, const identifier & name){
        auto it = data.find(ti_id{ti, name});
        if(it!=data.end()){
            std::get<0>(it->second) = true;
        }
        else{
            fail(ti, name);
        }
    }
    
    template<typename T>
    void erase(const identifier & name){
        erase(typeid(T), name);
    }
    
    void erase(const std::type_info & ti, const identifier & name);
    
    template<typename T>
    bool allocated(const identifier & name) const{
        return allocated(typeid(T), name);
    }
    
    bool allocated(const std::type_info & ti, const identifier & name) const{
        return data.find(ti_id{ti, name}) != data.end();
    }
    
    template<typename T>
    bool present(const identifier & name) const {
        return present(typeid(T), name);
    }
    
    bool present(const std::type_info & ti, const identifier & name) const{
        auto it = data.find(ti_id{ti, name});
        if(it==data.end()) return false;
        return std::get<0>(it->second);
    }
    
    virtual ~Event();
    
private:
    
    double weight_;
    
    void fail(const std::type_info & ti, const identifier & id) const;
    
    void * get_raw2(const std::type_info & ti, const identifier & name, bool fail_if_not_present);
    const void * get_raw2(const std::type_info & ti, const identifier & name, bool fail_if_not_present) const;
    
    
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
    typedef std::tuple<bool, void*, eraser_base*> element; // bool: is present
    std::map<ti_id, element> data;
};

}

#endif
