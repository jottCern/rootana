#ifndef BASE_REGISTRY_HPP
#define BASE_REGISTRY_HPP

#include <memory>
#include <map>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <iostream>
#include "utils.hpp"


/** \brief A central (program-wide) registry to build objects based on their name
 * 
 * This class is central in a plugin system, where objects of a concrete type that is unknown
 * to a component should be constructed at runtime. This is done with this Registry class which
 * can create objects of previously registered classes inheriting from a common base class.
 * 
 * Derived classes have to register at the registry before they can be build. This is usually done
 * at startup of the program by using the register_ method from a global scope; see the REGISTER_DEFC
 * macro as an example of how to do that.
 * 
 * Modules that want to instantiate classes can use the \c build method which accepts the key (for looking
 * up the right type) and the constructor arguments to use.
 * 
 * Note that the registries for different \c base_types and \c ctypes... are completely separate. This means
 * that the registry used to build an object has to be the same (in terms of \c base_type and \c ctypes...) as the
 * one used to register the type previously. It also means that it is possible to register classes with the same derived
 * type multiple times if this is done with different base classes and/or constructor types.
 * 
 * \c base_type_ is the base type of the object to construct, usually an abstract class
 * 
 * \c key_type_ is the key to use for looking up types; often a string
 * 
 * \c ctypes... are the constructor argument types required for constructing the concrete classes inheriting from \c base_type_
 */
template<typename base_type_, typename key_type_, typename... ctypes>
class Registry{
public:
    typedef base_type_ base_type;
    typedef key_type_ key_type;
    
    /**
     * Register T and allow it to be found under the given key.
     * T has to derive from \c base_class and take \c ctypes... as constructor arguments
     */
    template<typename T>
    static int register_(const key_type & key){
        return instance().iregister<T>(key);
    }
    
    /// Build an object according to the given key and constructor arguments
    static std::unique_ptr<base_type> build(const key_type & key, ctypes... params){
        return instance().ibuild(key, params...);
    }
    
    /// Lookup the key for the concrete type of b.
    static key_type key(const base_type & b){
        return instance().ikey(b);
    }
    
    Registry(const Registry &) = delete;
    Registry(Registry &&) = delete;
private:
    Registry(){}
    
    // implementation: each static method has a non-static counterpart
    // which is called on the singleton returned by instance().
    
    static Registry & instance(){
        static Registry i;
        return i;
    }
    
    std::unique_ptr<base_type> ibuild(const key_type & key, ctypes... params) const;
    
    template<typename T>
    int iregister(const key_type & key);
    
    key_type ikey(const base_type & b) const;
    
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

// used for error messages:
template<typename T>
inline std::string registry_name(){
    return "Registry<" + demangle(typeid(T).name()) + ">";
}



template<typename base_type, typename key_type, typename... ctypes>
template<typename T>
int Registry<base_type, key_type, ctypes...>::iregister(const key_type & key){
    static_assert(std::is_base_of<base_type, T>::value, "registerd type T must be derived from base_type");
    auto it = key_to_factory.find(key);
    if(it != key_to_factory.end()){
        std::stringstream ss;
        ss << registry_name<base_type>() << ": tried to register type with same key twice (key: <" << key << ">)";
        throw std::runtime_error(ss.str());
    }
    key_to_factory.insert(make_pair(key, std::unique_ptr<Factory>(new FactoryDefault<T>())));
    type_to_key[typeid(T)] = key;
    return 0;
}

template<typename base_type, typename key_type, typename... ctypes>
key_type Registry<base_type, key_type, ctypes...>::ikey(const base_type & b) const {
    auto it = type_to_key.find(typeid(b));
    if(it == type_to_key.end()){
        throw std::runtime_error(registry_name<base_type>() + "::key: type not registered");
    }
    return it->second;
}

template<typename base_type, typename key_type, typename... ctypes>
std::unique_ptr<base_type> Registry<base_type, key_type, ctypes...>::ibuild(const key_type & key, ctypes... params) const {
    auto it = key_to_factory.find(key);
    if(it==key_to_factory.end()){
        std::stringstream ss;
        ss << registry_name<base_type>() << ": did not find registered type of key '" << key << "'";
        throw std::runtime_error(ss.str());
    }
    try{
        return (*it->second)(params...);
    }
    catch(std::exception & ex){
        std::stringstream ss;
        ss << registry_name<base_type>() << ": exception while trying to build type of key <" << key + ">: " << ex.what();
        throw std::runtime_error(ss.str());
    }
    catch(...){
        std::cerr << registry_name<base_type>() << ": about to re-throw exception while trying to build type of key <" << key + ">" << std::endl;
        throw;
    }
}

// register a default-contructible type T with base type BT and key type KT with the given key
#define REGISTER_DEFC(T, BT, KT, key) namespace { int dummy##T = ::Registry<BT,KT>::register_<T>(key); }

#endif
