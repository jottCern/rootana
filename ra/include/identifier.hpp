#ifndef RA_IDENTIFIER_HPP
#define RA_IDENTIFIER_HPP

#include <string>
#include <unordered_map>

namespace ra{
    
/** \brief Program-unique identifiers
 *
 * Identifiers can be used as faster replacement for strings used to identify all sorts of things
 * such as histograms or event data in the input / output, etc. As with strings, the user is who
 * gives meaning to the identifier strings by convention.
 * 
 * identifier objects internally only consist of an id and are can be used as key in a map. They are thus
 * usually *much* faster than performing the same operation with strings.
 * 
 * Note that identifiers are (currently) *not* thread-safe.
 */
class identifier{
public:
    identifier(): id_(-1){}
    identifier(const char * c);
    identifier(const std::string & s);
    identifier(const identifier &) = default;
    identifier(identifier &&) = default;
    identifier & operator=(const identifier & rhs) = default;
    identifier & operator=(identifier && rhs) = default;
    
    bool operator==(const identifier & other) const{
        return id_ == other.id_;
    }
    
    bool operator!=(const identifier & other) const{
        return id_ != other.id_;
    }
    
    bool operator<(const identifier & other) const{
        return id_ < other.id_;
    }
    
    ssize_t id() const{
        return id_;
    }

    std::string name() const;
        
private:
    ssize_t id_;
};

}

namespace std{
    
template<>
struct hash< ra::identifier>{
    typedef ra::identifier argument_type;
    typedef size_t result_type;
    
    std::hash<ssize_t> hasher;
    
    result_type operator()(const ra::identifier & id) const{
        return hasher(id.id());
    }
};

}

#define ID(name) static ::ra::identifier name(#name)


#endif

