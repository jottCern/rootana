#ifndef BASE_CONFIG_HPP
#define BASE_CONFIG_HPP

// common tools for boost ptree as configuration

#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>

// lexical cast for boolean, to be able to read boolean values:
namespace boost {
    template<> 
    inline bool lexical_cast<bool, std::string>(const std::string& arg) {
        std::istringstream ss(arg);
        bool b;
        ss >> std::boolalpha >> b;
        return b;
    }

    template<>
    inline std::string lexical_cast<std::string, bool>(const bool& b) {
        std::ostringstream ss;
        ss << std::boolalpha << b;
        return ss.str();
    }
}



// try a lexical cast, adding some useful debug info in case of failure by including "what" in the exception string
template<typename T>
T try_cast(const std::string & what, const std::string & s){
    try{
        return boost::lexical_cast<T>(s);
    }
    catch(boost::bad_lexical_cast & ex){
        throw std::runtime_error(what + ": error casting '" + s + "' to type " + typeid(T).name());
    }
}

template<typename T>
T ptree_get(const boost::property_tree::ptree & cfg, const std::string & path){
    try{
        std::string rawvalue = cfg.get<std::string>(path);
        return try_cast<T>(path, std::move(rawvalue));
    }
    catch(const boost::property_tree::ptree_bad_path &){
        throw std::runtime_error("did not find path '" + path + "'");
    }
}

// get with default
template<typename T>
T ptree_get(const boost::property_tree::ptree & cfg, const std::string & path, const T & def){
    boost::optional<std::string> rawvalue = cfg.get_optional<std::string>(path);
    if(rawvalue){
        return try_cast<T>(path, std::move(*rawvalue));
    }
    else{
        return def;
    }
}


#endif
