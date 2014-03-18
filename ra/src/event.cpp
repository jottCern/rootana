#include "event.hpp"
#include "base/include/utils.hpp"

#include <stdexcept>

using namespace ra;

void Event::fail(const std::type_info & ti, const identifier & id) const{
    throw std::runtime_error("Event: did not find member with name '" + id.name() + "' of type '" + demangle(ti.name()) + "'");
}


void * Event::get(const std::type_info & ti, const identifier & name){
    ti_id key{ti, name};
    auto it = data.find(key);
    if(it==data.end()) fail(ti, name);
    return it->second.first;
}

const void * Event::get(const std::type_info & ti, const identifier & name) const{
    ti_id key{ti, name};
    auto it = data.find(key);
    if(it==data.end()) fail(ti, name);
    return it->second.first;
}

void Event::set(const std::type_info & ti, const identifier & name, void * obj, const std::function<void (void*)> & eraser){
    ti_id key{ti, name};
    auto it = data.find(key);
    if(it!=data.end()){
        it->second.second->operator()(it->second.first);
        delete it->second.second;
    }
    data[key] = std::pair<void*, eraser_base*>(obj, new eraser_f_wrapper(eraser));
}

Event::~Event(){
    for(auto & it : data){
        if(it.second.second){
            // call eraser to remove data itself:
            it.second.second->operator()(it.second.first);
            // erase the eraser:
            delete it.second.second;
        }
    }
}
