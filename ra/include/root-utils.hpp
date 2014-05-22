#ifndef RA_ROOT_UTILS_HPP
#define RA_ROOT_UTILS_HPP

#include <string>
#include <memory>
#include <stdexcept>
#include <cassert>
#include "TFile.h"

namespace ra {

// merge all rootfiles into file1. file1 has to exist (it will be updated during the merge).
// all files must have the same keys.
void merge_rootfiles(const std::string & file1, const std::vector<std::string> & rhs_filenames);


// get a (copy of a) histogram from an open root file, with error checking and readable error messages
template<typename T>
inline std::unique_ptr<T> gethisto(TFile & infile, const std::string & name){
    assert(infile.IsOpen());
    auto * t = infile.Get(name.c_str());
    if(!t){
        throw std::runtime_error("did not find '" + name + "' in file '" + infile.GetName() + "'");
    }
    T * tmpt = dynamic_cast<T*>(t);
    if(!tmpt){
        throw std::runtime_error("found '" + name + "' in file '" + infile.GetName() + "' but it has wrong type");
    }
    std::unique_ptr<T> result(static_cast<T*>(tmpt->Clone()));
    result->SetDirectory(0);
    return result;
}


}

#endif
