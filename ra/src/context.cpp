#include "context.hpp"
#include "config.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cassert>

#include "TBranch.h"
#include "TTree.h"
#include "TFile.h"
#include "TEmulatedCollectionProxy.h"
#include "TLeaf.h"
#include "TH1.h"

#include "base/include/utils.hpp"

using namespace ra;
using namespace std;

HistogramOutputManager::~HistogramOutputManager(){}

OutputManager::~OutputManager(){}

InputManager::~InputManager(){}






// OutputManager
namespace {
    
// see TTree.cxx:
char DataTypeToChar(EDataType datatype){

    switch(datatype) {
        case kChar_t: return 'B';
        case kUChar_t: return 'b';
        case kBool_t: return 'O';
        case kShort_t: return 'S';
        case kUShort_t: return 's';
        case kCounter:
        case kInt_t: return 'I';
        case kUInt_t: return 'i';
        case kDouble_t:
        case kDouble32_t: return 'D';
        case kFloat_t:
        case kFloat16_t: return 'F';
        case kLong_t: return 0; // unsupported
        case kULong_t: return 0; // unsupported?
        case kchar: return 0; // unsupported
        case kLong64_t: return 'L';
        case kULong64_t: return 'l';

        case kCharStar: return 'C';
        case kBits: return 0; //unsupported

        case kOther_t:
        case kNoType_t:
        default:
            return 0;
    }
}
    
// Like TTree::Branch but with types only known at runtime: given a pointer to T, setup a branch. While this is possible
// with templates, it's not if the type is only known at runtime (like here), so
// for this case, we have to so some painful stuff by reading and re-writing root code ...
void ttree_branch(TTree * tree, const char * name, void * addr, void ** addraddr, const std::type_info & ti){
    //emulate
    // template<typename T>
    // outtree->Branch(name, (T*)addr);
    // which expands to
    // outtree->BranchImpRef(name, TBuffer::GetClass(ti), TDataType::GetType(ti), const_cast<void*>(addr), 32000, 99),
    // so look there to understand how this code here works.
    TClass * class_ = TBuffer::GetClass(ti);
    if(class_){
        TClass * actualclass = class_->GetActualClass(addr);
        assert(actualclass!=0);
        assert((class_ == actualclass) || actualclass->InheritsFrom(class_));
        assert(dynamic_cast<TEmulatedCollectionProxy*>(actualclass->GetCollectionProxy())==0); // if this fails, it means T is a STL without compiled STL dictionary
        // I'd like to do now:
        // outtree->BronchExec(name, actualClass->GetName(), addr, *** kFALSE ****, bufsize, splitlevel);
        // but root doesn't let me, so work around by passing a pointer-to-pointer after all:
        tree->Bronch(name, actualclass->GetName(), addraddr);
    }
    else{
        EDataType dt = TDataType::GetType(ti);
        if(dt==kOther_t or dt==kNoType_t) throw invalid_argument("unknown type");
        char c = DataTypeToChar(dt);
        if(c==0) throw invalid_argument("unknown type");
        tree->Branch(name, addr, (string(name) + "/" + c).c_str());
    }
}

void root_cd(TDirectory & base, string path, bool create_dirs = true){
    // remove leading '/'s:
    while(!path.empty() && path[0]=='/'){
        path = path.substr(1);
    }
    if(path.empty()){
        base.cd();
    }
    else{
        size_t p = path.find('/');
        assert(p!=0); // leading '/' have been removed, so this should not happen
        string nextdir_name;
        if(p==string::npos){
            nextdir_name = path;
            path = "";
        }
        else{
            nextdir_name = path.substr(0, p);
            path = path.substr(p+1);
        }
        TDirectory * nextdir = base.GetDirectory(nextdir_name.c_str());
        if(!nextdir && create_dirs){
            nextdir = base.mkdir(nextdir_name.c_str());
        }
        if(!nextdir){
            throw runtime_error("Error: no such directory '" + nextdir_name + "' in directory '" + base.GetName() + "'");
        }
        root_cd(*nextdir, path, create_dirs);
    }
}

// split a path-string to the path part and the name part. path can be empty.
std::pair<string, string> get_path_name(const string & fullname){
    size_t p = fullname.rfind('/');
    if(p==string::npos) p = 0;
    string path = fullname.substr(0, p); // can be empty in case no '/' was found
    string name = fullname.substr(p + (p==0?0:1)); // last part of the name, without '/'
    return make_pair(move(path), move(name));
}

TTree * create_ttree(TFile & file, const string & name){
    TDirectory * dir = gDirectory;
    auto path_name = get_path_name(name);
    root_cd(file, path_name.first);
    TTree * result = new TTree(path_name.second.c_str(), path_name.second.c_str());
    gDirectory = dir;
    return result;
}


}


void TFileOutputManager::put(const char * name_, TH1 * histo){
    assert(outfile);
    // find last '/' to get directory name:
    auto path_name = get_path_name(name_);
    root_cd(*outfile, path_name.first);
    histo->SetDirectory(0);
    histo->SetName(path_name.second.c_str());
    histo->SetDirectory(gDirectory);
}

void TFileOutputManager::declare_event_output(const char * name, const string & event_member_name, const void * caddr, const std::type_info & ti){
    if(!event_tree){
        event_tree = create_ttree(*outfile, event_treename);
    }
    void * addr = const_cast<void*>(caddr);
    ptrs.push_back(addr);
    void *& ptrptr = ptrs.back();
    if(!event_member_name.empty()){
        auto handle = event.get_raw_handle(ti, event_member_name);
        event_members.emplace_back(handle, &ti);
    }
    ttree_branch(event_tree, name, addr, &ptrptr, ti);
}

void TFileOutputManager::declare_output(const identifier & tree_id, const char * name, const void * caddr, const std::type_info & ti){
    TTree *& tree = trees[tree_id];
    if(!tree){
        tree = create_ttree(*outfile, tree_id.name());
    }
    assert(tree);
    void * addr = const_cast<void*>(caddr);
    ptrs.push_back(addr);
    void *& ptrptr = ptrs.back();
    ttree_branch(tree, name, addr, &ptrptr, ti);
}

void TFileOutputManager::write_output(const identifier & tree_id){
    assert(outfile);
    auto it = trees.find(tree_id);
    if(it==trees.end()){
        throw invalid_argument("did not find tree '" + tree_id.name() + "'");
    }
    it->second->Fill();
}

void TFileOutputManager::write_event(){
    assert(outfile);
    if(event_tree){
        // read all event members; to make sure info is up to date in case of lazy reads:
        for(const auto & em : event_members){
            event.get(*em.second, em.first);
        }
        event_tree->Fill();
    }
}

void TFileOutputManager::close(){
    if(outfile){
        outfile->cd();
        outfile->Write();
        outfile.reset();
    }
}

TFileOutputManager::~TFileOutputManager(){
    close();
}
    
TFileOutputManager::TFileOutputManager(std::unique_ptr<TFile> outfile_, const string & event_treename_, Event & event_): OutputManager(event_), outfile(move(outfile_)), event_treename(event_treename_),event_tree(0){
}

