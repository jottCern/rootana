#include "root-utils.hpp"
#include "TKey.h"
#include "TDirectory.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TMethodCall.h"
#include "TFileMerger.h"

#include "base/include/log.hpp"
#include "Cintex/Cintex.h"

#include <stdexcept>
#include <cassert>
#include <unordered_map>
#include <set>
#include <limits.h>

using namespace std;
using namespace ra;


namespace{

struct sdummy{
  sdummy(){
      TH1::AddDirectory(false);
      ROOT::Cintex::Cintex::Enable();
  }
};
sdummy d;

unordered_map<string, TKey*> get_keys(TDirectory * dir){
    assert(dir != 0);
    unordered_map<string, TKey*> result;
    TList * keys = dir->GetListOfKeys();
    TIter next(keys);
    while(TObject * key_ = next()){
        TKey * key = static_cast<TKey*>(key_);
        result[key->GetName()] = key;
    }
    return move(result);
}

struct s_info {
    std::map<std::string, size_t> ttree_nentries_total;
};

void merge(TDirectory * lhs, const std::vector<TDirectory*> & rhs, const string & dirname, s_info & info){
    auto logger = Logger::get("ra.root-utils.merge");
    const size_t n = rhs.size();
    LOG_DEBUG("entering merge for directory " << dirname << lhs->GetName() << " with " << n << " other directories");
    unordered_map<string, TKey*> lhs_keys(get_keys(lhs));
    std::vector<unordered_map<string, TKey*> > rhs_keys;
    rhs_keys.reserve(n);
    for(size_t i=0; i<n; ++i){
        rhs_keys.emplace_back(get_keys(rhs[i]));
        if(rhs_keys.back().size() != lhs_keys.size()){
            LOG_THROW("Different number of keys in the directories to merge.");
        }
    }
    
    // iterate over lhs keys and merge each one with rhs:
    for(auto & lit : lhs_keys){
        string l_class = lit.second->GetClassName();
        std::vector<TKey*> current_rhs_keys(n);
        for(size_t i=0; i<n; ++i){
             auto rit = rhs_keys[i].find(lit.first);
             if(rit == rhs_keys[i].end()){
                 LOG_THROW("did not find object " << dirname << lit.first << " in all TDirectories to merge with");
             }
             if(l_class != rit->second->GetClassName()){
                 LOG_THROW("object '" << dirname << lit.first << "' has different types in files to merge: left type: "
                            << l_class << "; right type: " << rit->second->GetClassName());
             }
             current_rhs_keys[i] = rit->second;
        }
               
        TObject * left_object = lit.second->ReadObj();
        if(!left_object){
            LOG_THROW("Could not read object for key '" << dirname << lit.first + "'");
        }
        // if it is a TDirectory, merge recursively:
        if(TDirectory * left_tdir = dynamic_cast<TDirectory*>(left_object)){
            std::vector<TDirectory*> rhs_dirs(n);
            for(size_t i=0; i<n; ++i){
                 rhs_dirs[i] = static_cast<TDirectory*>(current_rhs_keys[i]->ReadObj());
            }
            LOG_DEBUG("recursively merging directory " << dirname << lit.first);
            merge(left_tdir, rhs_dirs, dirname + lit.first + '/', info);
            continue;
        }
        else{
            // otherwise, call the 'merge' method with a TList of all rhs TObjects
            TList rhs_object_list;
            for(size_t i=0; i<n; ++i){
                rhs_object_list.Add(current_rhs_keys[i]->ReadObj());
            }
            
            TTree * tree = dynamic_cast<TTree*>(left_object);
            if(tree){
                //tree->SetAutoSave(0); // if not doing it, large TTrees will autosave during merging
                // and in the process write a new TKey to the old file already,
                // but we want to delete it later in all cases ...
                size_t & ntot = info.ttree_nentries_total[dirname + lit.first];
                ntot = tree->GetEntries();
                TIter next(&rhs_object_list);
                for(size_t i=0; i<n; ++i){
                    ntot += static_cast<TTree*>(next())->GetEntries();
                }
                LOG_DEBUG("expecting " << ntot << " entries for TTree " << dirname << lit.first << " after merging");
            }
            TMethodCall mergeMethod;
            mergeMethod.InitWithPrototype(left_object->IsA(), "Merge", "TCollection*" );
            if(!mergeMethod.IsValid()){
                LOG_THROW("object '" + lit.first + "' (class '" + l_class + "') has no 'Merge' method");
            }
            mergeMethod.SetParam((Long_t)&rhs_object_list);
            LOG_DEBUG("about to merge '" << dirname << lit.first << "' (class: " << l_class << ")");
            mergeMethod.Execute(left_object);
                        
            // remove the original TKey in the output file:
            lit.second->Delete();
            delete lit.second;
            // most objects have to be written explicitly ... except TTrees apparently ...
            if(!tree){
                lhs->cd();
                left_object->Write();
            }
        }
    }
}

std::string get_realpath(const string & fname){
    std::unique_ptr<char[]> path(new char[PATH_MAX]);
    const char * res = realpath(fname.c_str(), path.get());
    if(res == 0){
        return "<unresolved>";
    }
    return path.get();
}

}


void ra::merge_rootfiles(const std::string & file1, const std::vector<std::string> & rhs_filenames){
    auto logger = Logger::get("ra.root-utils.merge");
    LOG_DEBUG("entering merge_rootfiles file1=" << file1 << " with " << rhs_filenames.size() << " other files");
    if(rhs_filenames.empty()) return;
    TFile f1(file1.c_str(), "update");
    if(!f1.IsOpen()){
        LOG_THROW("could not open root file '" << file1 << "' for update");
    }
    
    // keep a list of files to detect cases in which a file is merged more than once:
    // (NOTE: originally, this was based on TFile::GetUUID, but it turns out that this is not always  unique(!)).
    std::set<std::string> filenames;
    filenames.insert(get_realpath(file1));
    
    std::vector<std::unique_ptr<TFile>> rhs_tfiles;
    std::vector<TDirectory*> rhs_tdirs(rhs_filenames.size());
    for(size_t i=0; i<rhs_filenames.size(); ++i){
        rhs_tfiles.emplace_back(new TFile(rhs_filenames[i].c_str(), "read"));
        if(!rhs_tfiles.back()->IsOpen()){
            LOG_THROW("could not open root file '" << rhs_filenames[i] << "' for reading");
        }
        auto res = filenames.insert(get_realpath(rhs_filenames[i]));
        if(!res.second){
            LOG_THROW("root file '" << rhs_filenames[i] << "' appears more than once for merging");
        }
        rhs_tdirs[i] = rhs_tfiles.back().get();
    }
    s_info info;
    merge(&f1, rhs_tdirs, "", info);
    
    // this is necessary to write the TTree:
    f1.cd();
    f1.Write();
    f1.Close();
    
    // TODO: check the TTree entries, if requested.
    LOG_DEBUG("exiting merge_rootfiles");
}

