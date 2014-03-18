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
 * The first call to 'set' will allocate memory for that object and set the initial value. Subsequent calls to 'set'
 * will overwrite this value without performing a new allocation. Therefore, it is guaranteed, that the reference
 * returned by 'get' remains valid if 'set' is called. Only calls to 'erase' will make the references invalid.
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
        return *(reinterpret_cast<T*>(get(typeid(T), name)));
    }
    
    template<typename T>
    const T & get(const identifier & name) const{
        return *(reinterpret_cast<const T*>(get(typeid(T), name)));
    }
    
    void * get(const std::type_info & ti, const identifier & name);
    const void * get(const std::type_info & ti, const identifier & name) const;
    
    
    //set<T>(name, value) is equivalent to:
    // if exists<T>(name){
    //    get<T>(name) = move(value);
    //}
    // else{
    //    set_new<T>(name, value); // creates T with new
    // }
    template<typename T>
    void set(const identifier & name, T value){
        ti_id key{typeid(T), name};
        auto it = data.find(key);
        if(it!=data.end()){
            *(reinterpret_cast<T*>(it->second.first)) = std::move(value);
            
            //it->second.second->operator()(it->second.first);
            //delete it->second.second;
        }
        else{
            data[key] = std::make_pair<void*, eraser_base*>(new T(std::move(value)), new eraser<T>());
        }
    }
    
    // eraser(obj) is called in the Event destructor
    void set(const std::type_info & ti, const identifier & name, void * obj, const std::function<void (void*)> & eraser);
    
    template<typename T>
    void erase(const identifier & name){
        ti_id key{typeid(T), name};
        auto it = data.find(key);
        if(it==data.end()) fail(typeid(T), name);
        // call eraser to remove data itself:
        if(it->second.second){
            it->second.second->operator()(it->second.first);
            // erase the eraser:
            delete it->second.second;
        }
        data.erase(it);
    }
    
    template<typename T>
    bool exists(const identifier & name) const{
        return data.find(ti_id{typeid(T), name}) != data.end();
    }
    
    bool exists(const std::type_info & ti, const identifier & name) const{
        return data.find(ti_id{ti, name}) != data.end();
    }
    
    virtual ~Event();
    
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
    typedef std::pair<void*, eraser_base*> element;
    std::map<ti_id, element> data;
};

}

#endif
