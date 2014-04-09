#include "utils.hpp"
#include "base/include/log.hpp"

#include <stdexcept>
#include <sstream>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;
using namespace ra;


namespace{
    
// the file exists (requires paths leading to it being accessible as well ...)
bool file_exists(const string & path){
    struct stat buf;
    int res = stat(path.c_str(), &buf);
    return res == 0;
}

std::string get_self_path(){
    static std::string result;
    if(!result.empty()) return result;
    char path[4096];
    ssize_t s = readlink("/proc/self/exe", path, 4096);
    if(s < 0 || s >= 4096){
        throw runtime_error("could not read /proc/self/exe");
    }
    else{
        path[s] = '\0';
    }
    string result_tmp(path);
    size_t p = result_tmp.rfind('/');
    if(p == string::npos){
        throw runtime_error("could not find '/' in path from /proc/self/exe");
    }
    result = result_tmp.substr(0, p + 1);
    return result;
}

// non-const sta::search_paths:
std::list<std::string> & nc_search_paths(){
    static std::list<std::string> sa;
    bool init(false);
    if(!init){
        // add some standard paths:
        sa.push_back(get_self_path());
        sa.push_back(get_self_path() + "../");
        sa.push_back(get_self_path() + "../lib/");
        init = true;
    }
    return sa;
}

}

const std::list<std::string> & ra::search_paths(){
    return nc_search_paths();
}

void ra::add_searchpath(const std::string & path, int index){
    std::list<std::string> & sa = nc_search_paths();
    if(index < 0 || (size_t)index > sa.size()){
        index = sa.size();
    }
    if(path.empty() || path[0]!='/'){
        throw invalid_argument("add_search_path: path '" + path + "' invalid: has to be absolute and non-empty!");
    }
    // make sure the path ends with '/':
    std::string p = path;
    if(p[p.size()-1]!='/') p+= '/';
    auto it = sa.begin();
    std::advance(it, index);
    sa.insert(it, p);
}

std::string ra::resolve_file(const std::string & path){
    if(path.empty()) return "";
    const std::list<std::string> & sa = search_paths();
    for(const string & p : sa){
        if(file_exists(p + path)){
            return p + path;
        }
    }
    return "";
}

void ra::load_lib(const string & path){
    Logger & logger = Logger::get("dra.utils");
    string actual_path = resolve_file(path);
    if(actual_path.empty()){
        throw runtime_error("did not find library '" + path + "'");
    }
    void * handle = dlopen(actual_path.c_str(), RTLD_NOW | RTLD_DEEPBIND);
    if(handle==0){
        const char * errstr = dlerror();
        if(errstr==0) errstr = "<dlerror returned NULL>";
        LOG_ERROR("dlopen of '" << path << "' (resolved to: '" << actual_path << "') failed: " << errstr);
        throw runtime_error("dlopen failed");
    }
}


// progress bar:
namespace{
void gettime(timespec & t){
    clock_gettime(CLOCK_MONOTONIC, &t);
}

// dt in seconds
double dt(const timespec & t0, const timespec & t1){
    return (t0.tv_sec - t1.tv_sec) + (t0.tv_nsec - t1.tv_nsec) / 1e9;
}
}

progress_bar::progress_bar(const std::string & pattern, double autoprint_dt_): chars_written(0), next_update(0), autoprint_dt(autoprint_dt_), rtime("rtime") {
    tty = isatty(1);
    gettime(start);
    size_t p = 0;
    while(p < pattern.size()){
        // first find literal:
        size_t nextp = pattern.find('%', p);
        literals.emplace_back();
        while(nextp != string::npos && nextp+1 < pattern.size() && pattern[nextp+1]=='%'){
            literals.back() += pattern.substr(p, nextp - p);
            p = nextp+1;
            nextp = pattern.find('%', p+2);
        }
        if(nextp==string::npos){
            // the rest is literal:
            literals.back() += pattern.substr(p);
            break;
        }
        else{
            literals.back() += pattern.substr(p, nextp - p);
        }
        //cout << " literal: '" << literals.back() << "'" << endl;
        
        // now interpret the format string starting at nextp:
        // should start with '(identifier)'
        if(nextp+1 == pattern.size() || pattern[nextp+1]!='(') throw runtime_error("expected (");
        size_t pend = pattern.find(')', nextp);
        if(pend==string::npos || pend <= nextp+2) throw runtime_error("expected '(identifier)'");
        string idname = pattern.substr(nextp+2, pend-nextp-2);
        
        formatspec spec;
        spec.id = identifier(idname);
        
        // parse || part:
        nextp = pend+1;
        while(nextp < pattern.size() && pattern[nextp]=='|'){
            pend = pattern.find('|', nextp+1);
            if(pend==string::npos || pend <= nextp+1) throw runtime_error("expected '|filter|'");
            string filtername = pattern.substr(nextp+1, pend-nextp-1);
            if(filtername.find("percentof_")==0){
                spec.is_percent_of = true;
                spec.percent_of = identifier(filtername.substr(10));
            }
            else if(filtername == "rate"){
                spec.rate = true;
            }
            else{
                throw runtime_error("unknown filter '" + filtername + "'");
            }
            nextp = pend+1;
        }
        // everything from nextp to the next 'f', 's' or 'ld' is the format string, 
        // whatever comes first:
        pend = string::npos;
        pend = std::min(pend, pattern.find('f', nextp));
        pend = std::min(pend, pattern.find('s', nextp));
        pend = std::min(pend, pattern.find("ld", nextp));
        if(pend==string::npos) throw runtime_error("expected 'f', 's', or 'ld' at end of format string");
        
        if(pattern[pend]=='f') spec.type = type_f;
        else if(pattern[pend]=='s') spec.type = type_s;
        else if(pattern[pend]=='l') spec.type = type_ld;
        
        // set pend to the position right after 'f'/'s'/'ld':
        pend += 1 + (pattern[pend]=='l'? 1 : 0);
        spec.printf_formatstring = "%" + pattern.substr(nextp, pend - nextp);
        //cout << " format string: " << spec.printf_formatstring << " for '" << spec.id.name() << "'" << endl;
        formats.emplace_back(std::move(spec));
        p = pend;
    }
}

progress_bar::~progress_bar(){
    print();
    cout << endl;
}


void progress_bar::print(bool update_time) {
    if(update_time){
        timespec now;
        gettime(now);
        double total_time = dt(now, start);
        set(rtime, total_time);
    }
    cout << "\033[" << chars_written << "D";
    
    int chars_written_now = 0;
    for(size_t i=0; i<literals.size(); ++i){
        cout << literals[i];
        chars_written_now += literals[i].size();
        if(i>=formats.size())continue;
        char c[500];
        double value;
        switch(formats[i].type){
            case type_ld:
                chars_written_now += snprintf(c, 500, formats[i].printf_formatstring.c_str(), ldvals[formats[i].id]);
                break;
            case type_s:
                chars_written_now += snprintf(c, 500, formats[i].printf_formatstring.c_str(), svals[formats[i].id].c_str());
                break;
            case type_f:
                value = fvals[formats[i].id];
                if(formats[i].rate) value /= fvals[rtime];
                if(formats[i].is_percent_of) value *= 100. / fvals[formats[i].percent_of];
                chars_written_now += snprintf(c, 500, formats[i].printf_formatstring.c_str(), value);
        }
        cout << c;
    }
    if(chars_written_now < chars_written){
        // fill with spaces:
        char c[chars_written - chars_written_now + 1];
        std::fill(c, c + chars_written - chars_written_now, ' ');
        c[chars_written - chars_written_now] = '\0';
        cout << c;
        cout << "\033[" << (chars_written - chars_written_now) << "D";
    }
    chars_written = chars_written_now;
    cout << flush;
}

void progress_bar::check_autoprint() {
    timespec now;
    gettime(now);
    double total_time = dt(now, start);
    if(total_time > next_update){
        set(rtime, total_time);
        print(false);
        next_update = total_time + autoprint_dt;
    }    
}


void progress_bar::set(const identifier & id, double d){
    if(!tty) return;
    fvals[id] = d;
}

void progress_bar::set(const identifier & id, ssize_t i){
    if(!tty) return;
    ldvals[id] = i;
    fvals[id] = i;
}

void progress_bar::set(const identifier & id, const string & s){
    if(!tty) return;
    svals[id] = s;
}


