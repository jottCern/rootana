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
            return HandleAccess_::create_raw_handle(static_cast<int64_t>(i));
        }
    }
    member_infos.emplace_back(name, ti);
    return HandleAccess_::create_raw_handle(static_cast<int64_t>(member_infos.size() - 1));
}


const std::type_info & EventStructure::type(const EventStructure::RawHandle & handle) const{
    auto index = HandleAccess_::index(handle);
    assert(index >= 0 && static_cast<size_t>(index) < member_infos.size());
    return member_infos[index].type;
}

std::string EventStructure::name(const RawHandle & handle){
    auto index = HandleAccess_::index(handle);
    assert(index >= 0 && static_cast<size_t>(index) < member_infos.size());
    return member_infos[index].name;
}


Event::Event(const EventStructure & es): structure(es), member_datas(es.member_infos.size()){}

void Event::fail(const std::type_info & ti, const EventStructure::RawHandle & handle, const string & msg) const{
    std::stringstream errmsg;
    auto index = EventStructure::HandleAccess_::index(handle);
    errmsg << "Event: error for handle refering to type '" << demangle(ti.name()) << "' at index " << index;
    if(index >= 0 && static_cast<size_t>(index) < structure.member_infos.size()){
        errmsg << " with name '" << structure.member_infos[index].name << "'";
    }
    errmsg << ": " << msg;
    throw std::runtime_error(errmsg.str());
}

void Event::fail(const EventStructure::RawHandle & handle, const string & msg) const{
    std::stringstream errmsg;
    auto index = HandleAccess_::index(handle);
    errmsg << "Event: error for handle refering to unknown type at index " << index;
    if(index >= 0 && static_cast<size_t>(index) < structure.member_infos.size()){
        errmsg << " with name '" << structure.member_infos[index].name << "'";
    }
    errmsg << ": " << msg;
    throw std::runtime_error(errmsg.str());
}


void Event::set(const std::type_info & ti, const RawHandle & handle, void * data, const std::function<void (void*)> & eraser){
    check(ti, handle, "set");
    auto index = HandleAccess_::index(handle);
    member_data & md = member_datas[index];
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
    auto index = HandleAccess_::index(handle);
    const member_data & md = member_datas[index];
    // handle nonexistent first (no generator callback involved here):
    if(md.data == nullptr){
        if(minimum_state == state::nonexistent) return nullptr;
        else{
            fail(ti, handle, "member data pointer is null, i.e. this member is known but was never initialized with 'set'");
        }
    }
    // generate if invalid and can be generated.
    bool has_generator = static_cast<bool>(md.generator);
    if(!md.valid && has_generator){
        try{
            md.generator();
        }
        catch(...){
            const auto & mi = structure.member_infos[index];
            std::cerr << "Exception while trying to generate element '" << mi.name << "' of type '" << demangle(ti.name()) << "'" << std::endl;
            throw;
        }
    }
    // easiest case: data is there and valid:
    if(md.valid || minimum_state == state::invalid){
        return md.data;
    }
    else{
        if(has_generator){
            fail(ti, handle, "'get_callback' called, but that did not set the element value");
        }
        fail(ti, handle, "member data is marked as invalid, i.e. this member is known and has associated memory, but was not marked valid via 'set' or 'set_validity'"); // does not return
        return nullptr; // only to make compiler happy
    }
}

void Event::check(const RawHandle & handle, const std::string & where) const {
    auto index = HandleAccess_::index(handle);
    if(index < 0 || static_cast<size_t>(index) >= member_datas.size()){
        fail(handle, where + ": index out of bounds, i.e. the handle used is invalid");
    }
}


void Event::check(const std::type_info & ti, const RawHandle & handle, const std::string & where) const{
    check(handle, where);
    auto index = HandleAccess_::index(handle);
    const auto & mi = structure.member_infos[index];
    if(mi.type!=ti){
        fail(ti, handle, where + ": type mismatch");
    }
}

void Event::set_validity(const std::type_info & ti, const EventStructure::RawHandle & handle, bool valid){
    check(ti, handle, "set_validity");
    auto index = HandleAccess_::index(handle);
    member_data & md = member_datas[index];
    md.valid = valid;
}

void Event::set_validity(const EventStructure::RawHandle & handle, bool valid){
    check(handle, "set_validity");
    auto index = HandleAccess_::index(handle);
    member_data & md = member_datas[index];
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
    auto index = HandleAccess_::index(handle);
    member_data & md = member_datas[index];
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
    auto index = HandleAccess_::index(handle);
    const member_data & md = member_datas[index];
    if(md.data == nullptr) return state::nonexistent;
    return md.valid ? state::valid : state::invalid;
}

Event::~Event(){
    // nothing to do: member data belongs to member_datas
}

Event::RawHandle MutableEvent::get_raw_handle(const std::type_info & ti, const std::string & name){
    auto result = structure.get_raw_handle(ti, name);
    if(structure.member_infos.size() > member_datas.size()){
        member_datas.resize(structure.member_infos.size());
    }
    return result;
}
