#ifndef RA_CONFIG_HPP
#define RA_CONFIG_HPP

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>

#include <map>
#include <vector>
#include <stdexcept>

#include "base/include/registry.hpp"
#include "base/include/log.hpp"
#include "base/include/ptree-utils.hpp"
#include "fwd.hpp"
#include "utils.hpp" // for bool conversion

using boost::property_tree::ptree;


// This header defines data structures containing the configuration file data. The hierarchy
// resembles the one in the config file:
// the top-level class is s_config which contains "s_options" (corresponding to the "options" section),
// a vector of "s_dataset" (which in turn contaion a vector of "s_file"). Note that plugin-objects, i.e.
// the AnalysisModules and the InputTree are *not* constructed. Rather, they ptree configuration is saved to
// allow construction when they are needed.

namespace ra {

// expand glob pattern
std::vector<std::string> glob(const std::string& pattern);

struct s_options{
    int blocksize;
    int maxevents_hint;
    std::string output_dir;
    std::vector<std::string> libraries;
    std::vector<std::string> searchpaths;
    bool keep_unmerged;
    
    explicit s_options(const ptree & options_cfg);
    s_options(const s_options &) = delete;
    s_options();
};

struct s_dataset{
    struct s_file{
        std::string path;
        int nevents; // -1 = auto-detect
        unsigned int skip;
        
        explicit s_file(const boost::property_tree::ptree & tree);
        s_file(std::string s);
    };
    
    struct s_tags{
        // the tags as plain key/value pairs
        std::map<std::string, std::string> keyval;
        
        // get as type T. throws a runtime_exception if not found. Cast from string to T is done with boost
        // any_cast which might throw a boost::bad_any_cast exception
        template<typename T>
        T get(const std::string & key) const{
            auto it = keyval.find(key);
            if(it==keyval.end()) throw std::runtime_error("dataset tag '" + key + "' not found");
            try{
                return boost::lexical_cast<T>(it->second);
            }
            catch(boost::bad_lexical_cast &){
                throw std::runtime_error("cannot convert dataset tag '" + key + "' (value: '" + it->second + "') to type " + typeid(T).name());
            }
        }
        
        // get as type T. Returns the default value def if the key is not found. Cast from string to T is done with boost
        // any_cast which might throw a boost::bad_any_cast exception
        template<typename T>
        T get(const std::string & key, const T & def) const{
            auto it = keyval.find(key);
            if(it==keyval.end()) return def;
            try{
                return boost::lexical_cast<T>(it->second);
            }
            catch(boost::bad_lexical_cast &){
                throw std::runtime_error("cannot convert dataset tag '" + key + "' (value: '" + it->second + "') to type " + typeid(T).name());
            }
        }
    
        explicit s_tags(const ptree & tree);
        s_tags(){}
    };
    
    std::string name;
    std::string treename;
    s_tags tags;
    std::vector<s_file> files;
    size_t filenames_hash;
    
    explicit s_dataset(const ptree & cfg);
};

// the representation of a complete config file:
struct s_config {
    s_options options;
    std::vector<s_dataset> datasets;
    ptree modules_cfg;
    
    explicit s_config(const std::string & filename);
};

}

#endif
