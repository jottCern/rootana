#include "utils.hpp"
#include <cxxabi.h>
#include <unistd.h>

#include <list>

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
