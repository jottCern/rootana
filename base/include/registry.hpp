#ifndef BASE_REGISTRY_HPP
#define BASE_REGISTRY_HPP

#include <memory>
#include <map>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <iostream>

template<typename base_type_, typename key_type_, typename... ctypes>
class Registry{
public:
    
    typedef base_type_ base_type;
    typedef key_type_ key_type;
    
    static Registry & instance(){
        static Registry i;
        return i;
    }
    
    std::unique_ptr<base_type> ibuild(const key_type & key, ctypes... params) const;
    
    template<typename T>
    int iregister(const key_type & key);
    
    //const std::type_info & type(const key_type & key) const;
    
    key_type ikey(const base_type & b) const;

    
    static std::unique_ptr<base_type> build(const key_type & key, ctypes... params){
        return instance().ibuild(key, params...);
    }
    
    static key_type key(const base_type & b){
        return instance().ikey(b);
    }
    
    template<typename T>
    static int register_(const key_type & key){
        return instance().iregister<T>(key);
    }
    
private:
    Registry(){}
    
    struct Factory{
        virtual std::unique_ptr<base_type> operator()(ctypes... params) const = 0;
        virtual ~Factory(){}
    };
    
    template<typename T>
    struct FactoryDefault: public Factory{
        virtual std::unique_ptr<base_type> operator()(ctypes... params) const{
            return std::unique_ptr<base_type>(new T(params...));
        }
        virtual ~FactoryDefault(){}
    };
    
    std::map<std::type_index, key_type> type_to_key;
    std::map<key_type, std::unique_ptr<Factory> > key_to_factory;
};


template<typename base_type, typename key_type, typename... ctypes>
template<typename T>
int Registry<base_type, key_type, ctypes...>::iregister(const key_type & key){
    static_assert(std::is_base_of<base_type, T>::value, "registerd type T must be derived from base_type");
    auto it = key_to_factory.find(key);
    if(it != key_to_factory.end()){
        std::stringstream ss;
        ss << "Registry: tried to register type with same key twice (key: <" << key << ">)";
        throw std::runtime_error(ss.str());
    }
    //key_type keytmp = key;
    key_to_factory.insert(make_pair(key, std::unique_ptr<Factory>(new FactoryDefault<T>())));
    type_to_key[typeid(T)] = key;
    return 0;
}

template<typename base_type, typename key_type, typename... ctypes>
key_type Registry<base_type, key_type, ctypes...>::ikey(const base_type & b) const {
    auto it = type_to_key.find(typeid(b));
    if(it == type_to_key.end()){
        throw std::runtime_error("Registry::key: type not registered");
    }
    return it->second;
}

template<typename base_type, typename key_type, typename... ctypes>
std::unique_ptr<base_type> Registry<base_type, key_type, ctypes...>::ibuild(const key_type & key, ctypes... params) const {
    auto it = key_to_factory.find(key);
    if(it==key_to_factory.end()){
        std::stringstream ss;
        ss << "Registry: did not find registered type of key <" << key << ">";
        throw std::runtime_error(ss.str());
    }
    try{
        return (*it->second)(params...);
    }
    catch(std::exception & ex){
        std::stringstream ss;
        ss << "Registry: exception while trying to build type of key <" << key + ">: " << ex.what();
        throw std::runtime_error(ss.str());
    }
    catch(...){
        std::cerr << "Registry: about to re-throw exception while trying to build type of key <" << key + ">" << std::endl;
        throw;
    }
}


// register a default-contructible type T with base type BT and key type KT with the given key
#define REGISTER_DEFC(T, BT, KT, key) namespace { int dummy##T = ::Registry<BT,KT>::register_<T>(key); }

#endif
