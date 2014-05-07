#include "event.hpp"
#include "base/include/utils.hpp"

#include <stdexcept>
#include <iostream>

using namespace ra;

std::ostream & ra::operator<<(std::ostream & out, Event::presence p){
    switch(p){
        case Event::presence::nonexistent:
            return out << "nonexistent";
        case Event::presence::allocated:
            return out << "allocated";
        default:
            return out << "present";
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
    if(fail_){
        bool present = std::get<0>(it->second);
        if(!present){
            fail(ti, name);
        }
    }
    return std::get<1>(it->second);
}

void Event::set_presence(const std::type_info & ti, const identifier & name, presence p){
    auto it = data.find(ti_id{ti, name});
    if(it!=data.end()){
        switch(p){
            case presence::nonexistent:
                // call eraser to remove data itself:
                if(std::get<2>(it->second)){
                    std::get<2>(it->second)->operator()(std::get<1>(it->second));
                    // erase the eraser:
                    delete std::get<2>(it->second);
                }
                data.erase(it);
                break;
                
            case presence::allocated:
                std::get<0>(it->second) = false;
                break;
                
            case presence::present:
                std::get<0>(it->second) = true;
                break;
        }
    }
    else if(p != presence::nonexistent){
        fail(ti, name);
    }
}

void Event::set_raw(const std::type_info & ti, const identifier & name, void * obj, const std::function<void (void*)> & eraser){
    ti_id key{ti, name};
    auto it = data.find(key);
    if(it!=data.end()){
        std::get<2>(it->second)->operator()(std::get<1>(it->second));
        delete std::get<2>(it->second);
    }
    data[key] = std::tuple<bool, void*, eraser_base*>(true, obj, new eraser_f_wrapper(eraser));
}

Event::presence Event::get_presence(const std::type_info & ti, const identifier & name) const{
    ti_id key{ti, name};
    auto it = data.find(key);
    if(it==data.end()) return presence::nonexistent;
    return std::get<0>(it->second) ? presence::present : presence::allocated;
}

Event::~Event(){
    for(auto & it : data){
        element & e = it.second;
        if(std::get<2>(e)){
            std::get<2>(e)->operator()(std::get<1>(e));
            delete std::get<2>(e);
        }
    }
}
