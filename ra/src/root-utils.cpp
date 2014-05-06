#include "root-utils.hpp"
#include "TKey.h"
#include "TDirectory.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TMethodCall.h"

#include "base/include/log.hpp"

#include <stdexcept>
#include <cassert>

using namespace std;
using namespace ra;


namespace{

struct sdummy{
  sdummy(){
      TH1::AddDirectory(false);
  }
};
sdummy d;
    

void merge(TDirectory * lhs, TDirectory * rhs){
    Logger & logger = Logger::get("ra.root-utils.merge");
    LOG_DEBUG("entering merge for directories " << lhs->GetName() << " and " << rhs->GetName());
    map<string, TKey*> lhs_keys;
    map<string, TKey*> rhs_keys;
    
    {
        TList * l_keys = lhs->GetListOfKeys();
        TIter next(l_keys);
        while(TObject * key_ = next()){
            TKey * key = dynamic_cast<TKey*>(key_);
            assert(key);
            lhs_keys[key->GetName()] = key;
        }
    }
    {
        TList * r_keys = rhs->GetListOfKeys();
        TIter next(r_keys);
        while(TObject * key_ = next()){
            TKey * key = dynamic_cast<TKey*>(key_);
            assert(key);
            rhs_keys[key->GetName()] = key;
        }
    }
    
    LOG_DEBUG("Found " << lhs_keys.size() << " objects in left TDirectory and " << rhs_keys.size() << " in right TDirectory");
    if(lhs_keys.size() != rhs_keys.size()) throw runtime_error("merge: directories have not the same number of entries!");
    
    lhs->cd();
    
    // iterate over lhs keys and merge each one with rhs:
    for(auto & lit : lhs_keys){
        auto rit = rhs_keys.find(lit.first);
        if(rit == rhs_keys.end()) throw runtime_error("merge: object '" + lit.first + "' not found in right list");
        // depending on the type, make different stuff now:
        string l_class = lit.second->GetClassName();
        // but first checki that it is the same class:
        if(l_class != rit->second->GetClassName()) throw runtime_error("merge: object '" + lit.first + "' has different type in left and right hand of merge");
        // now try to call the "Merge" method of the left hand object:
        TObject * left_object = lit.second->ReadObj();
        if(!left_object) throw runtime_error("could not read object '" + lit.first + "'");
        // if it is a TDirectory, merge recursively:
        if(TDirectory * left_tdir = dynamic_cast<TDirectory*>(left_object)){
            TDirectory * right_tdir = dynamic_cast<TDirectory*>(rit->second->ReadObj());
            assert(right_tdir);
            LOG_DEBUG("recursively merging directory " << left_tdir->GetName());
            merge(left_tdir, right_tdir);
            lhs->cd();
            continue;
        }
        else{
            // otherwise, call the 'merge' method:
            TTree * tree = dynamic_cast<TTree*>(left_object);
            int nentries_expected = 0;
            if(tree){
                tree->SetAutoSave(0); // if not doing it, large TTrees will autosave during merging
                // and in the process write a new TKey to the old file already,
                // but we want to delete it later in all cases ...
                nentries_expected = tree->GetEntries() + dynamic_cast<TTree*>(rit->second->ReadObj())->GetEntries();
            }
            TMethodCall mergeMethod;
            mergeMethod.InitWithPrototype(left_object->IsA(), "Merge", "TCollection*" );
            if(!mergeMethod.IsValid()){
                LOG_THROW("object '" + lit.first + "' (class '" + l_class + "') has no 'Merge' method");
            }
            TList l;
            l.Add(rit->second->ReadObj());
            mergeMethod.SetParam((Long_t)&l);
            LOG_DEBUG("about to merge '" << left_object->GetName() << "' (class: " << l_class << ")");
            mergeMethod.Execute(left_object);
            
            // remove the original TKey in the output file:
            lit.second->Delete();
            delete lit.second;
            left_object->Write();
            
            if(tree){
                if(tree->GetEntries() != nentries_expected){
                    LOG_THROW("Merging TTree '" + lit.first + "': number of entries after merging is not the sum of entries before merging!");
                }
            }
        }
    }
}

}


void ra::merge_rootfiles(const std::string & file1, const std::string & file2){
    TFile f1(file1.c_str(), "update");
    TFile f2(file2.c_str(), "read");
    merge(&f1, &f2);
    f1.cd();
    f1.Write();
    f1.Close();
}
