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

EventStructure::RawHandle EventStructure::get_raw_handle(const std::type_info & ti, const std::string & name){
    // check if it exists already. Note that this is a slow (O(N)) operation, but
    // this should be ok as we do not expect this method to be called often
    for(size_t i=0; i<member_infos.size(); ++i){
        if(member_infos[i].name == name && member_infos[i].type == ti){
            return RawHandle(i);
        }
    }
    member_infos.emplace_back(name, ti);
    return RawHandle(member_infos.size() - 1);
}

std::string EventStructure::name(const RawHandle & handle){
    assert(handle.index < member_infos.size());
    return member_infos[handle.index].name;
}


Event::Event(const EventStructure & es): structure(es), member_datas(es.member_infos.size()){}

void Event::fail(const std::type_info & ti, const EventStructure::RawHandle & handle, const string & msg) const{
    std::stringstream errmsg;
    errmsg << "Event: error for member of type '" << demangle(ti.name()) << "' at index " << handle.index;
    if(handle.index < structure.member_infos.size()){
        errmsg << " with name '" << structure.member_infos[handle.index].name << "'";
    }
    errmsg << ": " << msg;
    throw std::runtime_error(errmsg.str());
}


void Event::set(const std::type_info & ti, const RawHandle & handle, void * data, const std::function<void (void*)> & eraser){
    check(ti, handle, "set_unmanaged");
    member_data & md = member_datas[handle.index];
    if(md.data != nullptr){
        throw invalid_argument("set: tries to reset member already set.");
    }
    md.eraser = eraser;
    md.data = data;
    md.valid = true;
}

void * Event::get(const std::type_info & ti, const EventStructure::RawHandle & handle, state minimum_state){
    return const_cast<void*>(static_cast<const Event*>(this)->get(ti, handle, minimum_state));
}

const void * Event::get(const std::type_info & ti, const EventStructure::RawHandle & handle, state minimum_state) const{
    check(ti, handle, "get");
    const member_data & md = member_datas[handle.index];
    // handle nonexistent first (no generator callback involved here):
    if(md.data == nullptr){
        if(minimum_state == state::nonexistent) return nullptr;
        else{
            fail(ti, handle, "member data pointer is null, i.e. this member is known but was never initialized with 'set'");
        }
    }
    // generate if invalid and can be generated:
    if(!md.valid && md.generator){
        try{
            md.generator();
        }
        catch(...){
            const auto & mi = structure.member_infos[handle.index];
            std::cerr << "Exception while trying to generate element '" << mi.name << "' of type '" << demangle(ti.name()) << "'" << std::endl;
            throw;
        }
        md.valid = true;
    }
    // easiest case: data is there and valid:
    if(md.valid || minimum_state == state::invalid){
        return md.data;
    }
    else{
        fail(ti, handle, "member data is marked as invalid, i.e. this member is known and has associated memory, but was not marked valid via 'set' or 'set_validity'"); // does not return
        return nullptr; // only to make compiler happy
    }
}


void Event::check(const std::type_info & ti, const RawHandle & handle, const std::string & where) const{
    if(handle.index >= member_datas.size()){
        fail(ti, handle, where + ": index out of bounds, i.e. the handle used is invalid");
    }
    const auto & mi = structure.member_infos[handle.index];
    if(mi.type!=ti){
        fail(ti, handle, where + ": type mismatch");
    }
}

void Event::set_validity(const std::type_info & ti, const EventStructure::RawHandle & handle, bool valid){
    check(ti, handle, "set_validity");
    member_data & md = member_datas[handle.index];
    md.valid = valid;
}

namespace {
    struct undefined_type{};
}

void Event::set_validity(const EventStructure::RawHandle & handle, bool valid){
    if(handle.index >= member_datas.size()){
        fail(typeid(undefined_type), handle, "set_validity: index out of bounds, i.e. the handle used is invalid");
    }
    member_data & md = member_datas[handle.index];
    md.valid = valid;
}

Event::member_data::~member_data(){
    if(eraser){
        eraser(data);
    }
}

void Event::invalidate_all(){
    for(auto & md : member_datas){
        md.valid = false;
    }
}

void Event::set_get_callback(const std::type_info & ti, const RawHandle & handle, const std::function<void ()> & callback){
    check(ti, handle, "set_get_callback");
    member_data & md = member_datas[handle.index];
    if(md.data == nullptr){
        fail(ti, handle, "set_get_callback called for member without data");
    }
    md.generator = callback;
    if(callback){
        md.valid = false;
    }
}

Event::state Event::get_state(const std::type_info & ti, const RawHandle & handle) const{
    check(ti, handle, "get_state");
    const member_data & md = member_datas[handle.index];
    if(md.data == nullptr) return state::nonexistent;
    return md.valid ? state::valid : state::invalid;
}

Event::~Event(){
    // nothing to do: member data belongs to member_datas
}

Event::RawHandle MutableEvent::get_raw_handle(const std::type_info & ti, const std::string & name){
    Event::RawHandle result = structure.get_raw_handle(ti, name);
    if(structure.member_infos.size() > member_datas.size()){
        member_datas.resize(structure.member_infos.size());
    }
    return result;
}
