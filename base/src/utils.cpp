#include "utils.hpp"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cxxabi.h>
#include <list>
#include <system_error>

std::string demangle(const char * typename_){
    int status;
    char * demangled = abi::__cxa_demangle(typename_, 0, 0, &status);
    if(status != 0 or demangled == 0){
        free(demangled);
        return typename_;
    }
    std::string result(demangled);
    free(demangled);
    return result;
}

namespace {

std::list<std::pair<std::function<void ()>, int> > & fork_callbacks(){
    static std::list<std::pair<std::function<void ()>, int> > cbs;
    return cbs;
}
    
}

void atfork(std::function<void()> callback, int when){
    auto & callbacks = fork_callbacks();
    callbacks.emplace_back(move(callback), when);
}

int dofork(){
    int pid = fork();
    auto & callbacks = fork_callbacks();
    for(auto & cb : callbacks){
        if((cb.second & atfork_parent) > 0 && pid > 0){
            cb.first();
        }
        if((cb.second & atfork_child) > 0 && pid == 0){
            cb.first();
        }
    }
    return pid;
}


bool is_directory(const std::string & path){
    struct stat s;
    errno = 0;
    int res = stat(path.c_str(), &s);
    if(res < 0){
        return false;
    }
    else{
        return S_ISDIR(s.st_mode);
    }
}

bool is_regular_file(const std::string & path){
    struct stat s;
    errno = 0;
    int res = stat(path.c_str(), &s);
    if(res < 0){
        return false;
    }
    else{
        return S_ISREG(s.st_mode);
    }
}


std::string realpath(const std::string & path){
    char * result = realpath(path.c_str(), NULL);
    if(result == 0){
        throw std::system_error(errno, std::system_category(), "realpath('" + path + "')");
    }
    std::string sresult(result);
    free(result);
    return sresult;
}
