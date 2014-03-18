#include "utils.hpp"
#include <cxxabi.h>

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

