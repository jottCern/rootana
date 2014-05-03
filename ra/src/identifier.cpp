#include "identifier.hpp"
#include <stdexcept>

namespace {

struct identifier_registry{
    
    static identifier_registry & instance(){
        static identifier_registry i;
        return i;
    }
    
    identifier_registry(): next_id(0){}
    
    size_t register_or_get(const std::string & name){
        auto it = name_to_id.find(name);
        if(it==name_to_id.end()){
            size_t id = next_id++;
            name_to_id[name] = id;
            id_to_name[id] = name;
            return id;
        }
        else{
            return it->second;
        }
    }
    
    std::unordered_map<std::string, size_t> name_to_id;
    std::unordered_map<size_t, std::string> id_to_name;
    size_t next_id;
};

}

ra::identifier::identifier(const std::string & s): id_(identifier_registry::instance().register_or_get(s)){
}

ra::identifier::identifier(const char * c): id_(identifier_registry::instance().register_or_get(c)){
}

std::string ra::identifier::name() const{
    auto & id_to_name = identifier_registry::instance().id_to_name;
    auto it = id_to_name.find(id_);
    if(it==id_to_name.end()) throw std::invalid_argument("asked for name of invalid id");
    return it->second;
}

