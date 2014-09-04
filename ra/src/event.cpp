#include "event.hpp"
#include "base/include/utils.hpp"

#include <stdexcept>
#include <iostream>
#include <cassert>
#include <sstream>

using namespace ra;
using namespace std;

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

void Event::fail(const std::type_info & ti, const Event::RawHandle & handle, const char * msg) const{
    std::stringstream errmsg;
    errmsg << "Event: error for member of type '" << demangle(ti.name()) << "' at index " << handle.index;
    if(handle.index < elements.size()){
        errmsg << " with name '" << elements[handle.index].name << "'";
    }
    errmsg << ": " << msg;
    throw std::runtime_error(errmsg.str());
}

Event::RawHandle Event::get_raw_handle(const std::type_info & ti, const std::string & name){
    // check if it exists already. Note that this is a slow (O(N)) operation, but
    // probably ok as we do not expect this method to be called in the main event loop but just once
    // at initialization. TODO: enforce this by mocing the get_handle routines outside of the Event class
    // to an EventManager class which is only available during setup, not during event processing.
    for(size_t i=0; i<elements.size(); ++i){
        if(elements[i].name == name && elements[i].type == ti){
            return RawHandle(i);
        }
    }
    elements.emplace_back(false, name, (void*)0, (eraser_base*)(0), ti);
    return RawHandle(elements.size() - 1);
}

std::string Event::name(const RawHandle & handle){
    assert(handle.index < elements.size());
    return elements[handle.index].name;
}

void * Event::get(const std::type_info & ti, const Event::RawHandle & handle, bool check_valid, bool allow_null){
    return const_cast<void*>(static_cast<const Event*>(this)->get(ti, handle, check_valid, allow_null));
}

const void * Event::get(const std::type_info & ti, const Event::RawHandle & handle, bool check_valid, bool allow_null) const{
    if(handle.index >= elements.size()){
        fail(ti, handle, "index out of bounds, i.e. the handle used is invalid");
    }
    element & e = elements[handle.index];
    assert(e.type == ti);
    if(e.data == 0){
        if(allow_null) return 0;
        else{
            fail(ti, handle, "member data pointer is null, i.e. this member is known but was never initialized with 'set'");
        }
    }
    // easiest case: data is there and valid:
    if(e.valid){
        return e.data;
    }
    // it's not valid, try to generate it:
    if(e.generator){
        try{
            (*e.generator)();
        }
        catch(...){
            std::cerr << "Exception while trying to generate element '" << e.name << "' of type '" << demangle(ti.name()) << "'" << std::endl;
            throw;
        }
        e.valid = true;
        return e.data;
    }
    // if it's not valid and cannot be generated, return the pointer
    // to the 'invalid' member:
    if(check_valid){
        fail(ti, handle, "member data is marked as invalid, i.e. this member is known and has associated memory, but was not marked valid via 'set' or 'set_validity'"); // does not return
        return 0; // only to make compiler happy
    }
    else{
        return e.data;
    }
}

void Event::set_validity(const std::type_info & ti, const Event::RawHandle & handle, bool valid){
    if(handle.index >= elements.size()) throw std::invalid_argument("Event::set_validity: handle index out of range");
    element & e = elements[handle.index];
    if(e.type!=ti) throw std::invalid_argument("Event::set_validity: inconsistent type information");
    e.valid = valid;
}

Event::element::~element(){
    if(eraser){
        eraser->operator()(data);
    }
}

void Event::invalidate_all(){
    for(auto & e : elements){
        e.valid = false;
    }
}

void Event::set_get_callback(const std::type_info & ti, const RawHandle & handle, boost::optional<std::function<void ()>> callback){
    assert(handle.index < elements.size());
    element & e = elements[handle.index];
    assert(e.type == ti);
    e.valid = false;
    e.generator = move(callback);
}

Event::state Event::get_state(const std::type_info & ti, const RawHandle & handle) const{
    assert(handle.index < elements.size());
    const element & e = elements[handle.index];
    if(e.data==0) return state::nonexistent;
    return e.valid ? state::valid : state::invalid;
}

Event::~Event(){}
