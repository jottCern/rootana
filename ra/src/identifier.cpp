#include "identifier.hpp"
#include <stdexcept>

namespace {

struct identifier_registry{
    
    identifier_registry(): next_id(0){}
    
    int register_or_get(const std::string & name){
        auto it = name_to_id.find(name);
        if(it==name_to_id.end()){
            int id = next_id++;
            name_to_id[name] = id;
            id_to_name[id] = name;
            return id;
        }
        else{
            return it->second;
        }
    }
    
    std::unordered_map<std::string, int> name_to_id;
    std::unordered_map<int, std::string> id_to_name;
    int next_id;
} ireg;

}

namespace ra{

identifier::identifier(const std::string & s): id_(ireg.register_or_get(s)){
}

identifier::identifier(const char * c): id_(ireg.register_or_get(c)){
}

std::string identifier::name() const{
    auto & id_to_name = ireg.id_to_name;
    auto it = id_to_name.find(id_);
    if(it==id_to_name.end()) throw std::invalid_argument("asked for name of invalid id");
    return it->second;
}

}
