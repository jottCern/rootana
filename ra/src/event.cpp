#include "event.hpp"
#include "base/include/utils.hpp"

#include <stdexcept>
#include <iostream>
#include <cassert>

using namespace ra;

namespace {
    
template<class T, class... Args>
std::unique_ptr<T> make_unique( Args&&... args ){
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}

std::ostream & ra::operator<<(std::ostream & out, Event::state p){
    switch(p){
        case Event::state::nonexistent:
            return out << "nonexistent";
        case Event::state::invalid:
            return out << "invalid";
        default:
            return out << "valid";
    }
}

void Event::fail(const std::type_info & ti, const identifier & id) const{
    throw std::runtime_error("Event: did not find member with name '" + id.name() + "' of type '" + demangle(ti.name()) + "'");
}

void * Event::get_raw(const std::type_info & ti, const identifier & name, bool fail_){
    return const_cast<void*>(static_cast<const Event*>(this)->get_raw(ti, name, fail_));
}

const void * Event::get_raw(const std::type_info & ti, const identifier & name, bool fail_) const{
    ti_id key{ti, name};
    auto it = data.find(key);
    if(it==data.end()) fail(ti, name); // fail does not return
    element & e = it->second;
    if(e.valid){
        return e.data;
    }
    if(e.generator){
        try{
            (*e.generator)();
        }
        catch(...){
            std::cerr << "Exception while trying to generate element '" << name.name() << "' of type '" << demangle(ti.name()) << "'" << std::endl;
            throw;
        }
        e.valid = true;
        return e.data;
    }
    if(fail_){
        fail(ti, name); // does not return
        return 0; // only to make compiler happy
    }
    else{
        return e.data;
    }
}

void Event::set_state(const std::type_info & ti, const identifier & name, state s){
    auto it = data.find(ti_id{ti, name});
    if(it==data.end()){
        throw std::invalid_argument("Error in Event::set_state: member does not exist");
    }
    switch(s){
        case state::invalid:
            it->second.valid = false;
            break;
            
        case state::valid:
            it->second.valid = true;
            break;
            
        default:
            throw std::invalid_argument("Error in Event::set_state: invalid new state (only valid and invalid are allowed)");
    }
}

Event::element::~element(){
    if(eraser){
        eraser->operator()(data);
    }
}

Event::element::element(element && other){
    if(&other == this) return;
    if(eraser){
        eraser->operator()(data);
    }
    valid = other.valid;
    data = other.data;
    other.data = 0;
    eraser = move(other.eraser);
    type = other.type;
    generator = move(other.generator);
}

void Event::set_raw(const std::type_info & ti, const identifier & name, void * obj, const std::function<void (void*)> & eraser){
    ti_id key{ti, name};
    auto it = data.find(key);
    if(it!=data.end()){
        data.erase(it);
    }
    data.insert(make_pair(key, element(true, obj, new eraser_f_wrapper(eraser), &ti)));
}

void Event::invalidate_all(){
    for(auto & it : data){
        it.second.valid = false;
    }
    weight_ = 1.0;
}

void Event::set_get_callback(const std::type_info & ti, const identifier & name, const std::function<void ()> & callback){
    ti_id key{ti, name};
    auto it = data.find(key);
    if(it==data.end()){
        fail(ti, name);
    }
    else{
        it->second.valid = false;
        it->second.generator = callback;
    }
}

void Event::reset_get_callback(const std::type_info & ti, const identifier & name){
    ti_id key{ti, name};
    auto it = data.find(key);
    if(it==data.end()){
        fail(ti, name);
    }
    it->second.generator = boost::none;
}

Event::state Event::get_state(const std::type_info & ti, const identifier & name) const{
    ti_id key{ti, name};
    auto it = data.find(key);
    if(it==data.end()) return state::nonexistent;
    return it->second.valid ? state::valid : state::invalid;
}

Event::~Event(){}
