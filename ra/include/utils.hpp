#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <map>
#include <list>
#include <boost/lexical_cast.hpp>
#include "identifier.hpp"

namespace boost {
    template<> 
    bool lexical_cast<bool, std::string>(const std::string& arg);
    template<>
    std::string lexical_cast<std::string, bool>(const bool& b);
}


// see cmssw: DataFormats/Math/interface/deltaR.h
// from 0 to +pi
template<typename T1, typename T2>
inline auto deltaPhi(const T1 & t1, const T2 & t2) -> decltype(t1.phi()){
    typedef decltype(t1.phi()) Float;
    Float p1 = t1.phi(); 
    Float p2 = t2.phi();
    auto dp=std::abs(p1-p2);
    if (dp>Float(M_PI)) dp-=Float(2*M_PI);
    return dp;
}

template<typename T1, typename T2>
inline auto deltaR2(const T1 & t1, const T2 & t2) -> decltype(t1.eta()) {
    typedef decltype(t1.eta()) Float;
    Float dp = deltaPhi(t1, t2);
    Float de = t1.eta() - t2.eta();
    return de*de + dp*dp;
} 
   
// do not use it: always cut in deltaR2!
template<typename T1, typename T2>
inline auto deltaR(const T1 & t1, const T2 & t2) -> decltype(t1.eta()) {
    return std::sqrt(deltaR2(t1,t2));
} 

namespace ra {

const std::list<std::string> & search_paths();
void add_searchpath(const std::string & path, int index = 0); // index is the position for the new path in the list, -1 or numbers too large will add to the end.
std::string resolve_file(const std::string & path);

    
void load_lib(const std::string & path);


/** \brief Display several kinds of "live" information on the stdout tty.
 * 
 * Does nothing if stdout is not a tty.
 * 
 * 
 * The pattern is used in the constructor to define how progress is displayed. It
 * is kind of a format string: '% (identifier) [|preprocess option|*] [printf specification]'.
 * 
 * Use %% for literal '%'
 * 
 * printf specification can only be '...s', '...f' or '...ld'.
 * 
 * where preprocess option can be: 
 *   - 'rate' to print the rate of the value instead of the total.
 *   - 'percentof_identifier' to get the ratio * 100 to the given identifier
 * 
 * Predefined identifiers:
 *  - rtime: real time since creation of progress_bar
 * 
 * Example:
 * 
 * "Processing %(ifile)ld / %(totalfiles)ld    %(ifile)|percentof_totalfiles|5.2f%%"
 * 
 * and then:
 * 
 * \code
 * progress_bar b;
 * identifier ifile("file"), totalfiles("totalfiles");
 * 
 * b.report(totalfiles, 10);
 * 
 * for(int i=0; i<nfiles; ++i){
 *    b.report(ifile, i);
 * }
 * \endcode
 * 
 *
 *
 */
class progress_bar{
public:
    explicit progress_bar(const std::string & pattern_, double autoprint_dt_ = 0.1);
    ~progress_bar();
    
    // print current format string with latest values
    void print(bool update_time = true);
    
    // check whether enough time has passed since last print and if yes, print current values.
    void check_autoprint();
    
    void set(const identifier & id, double value);
    void set(const identifier & id, ssize_t value);
    void set(const identifier & id, const std::string & value);
    
    void set(const identifier & id, size_t value){
        set(id, static_cast<ssize_t>(value));
    }
    
    void set(const identifier & id, int value){
        set(id, static_cast<ssize_t>(value));
    }
    
    void set(const identifier & id, unsigned int value){
        set(id, static_cast<ssize_t>(value));
    }
    

private:
    enum etype{ type_f, type_ld, type_s};
    
    struct formatspec{
        identifier id;
        etype type;
        bool rate;
        bool is_percent_of;
        identifier percent_of;
        std::string printf_formatstring;
        
        formatspec(): rate(false){}
    };
    
    
    bool tty;
    int chars_written;
    
    // pattern, broken into literal parts and single "format" parts starting with a '%':
    // first, print literal[0] (which can be empty), then  dynamic[0], etc.
    std::vector<std::string> literals;
    std::vector<formatspec> formats;
    
    // timing:
    timespec start;
    double next_update;
    double autoprint_dt;

    // current values:
    std::map<identifier, double> fvals;
    std::map<identifier, ssize_t> ldvals;
    std::map<identifier, std::string> svals;
    
    identifier rtime;
};


}

#endif
