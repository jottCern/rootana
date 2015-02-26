#include "root-utils.hpp"
#include "TKey.h"
#include "TDirectory.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TMethodCall.h"
//#include "TFileMerger.h"
#include "Cintex/Cintex.h"
#include "TEmulatedCollectionProxy.h"

#include "base/include/log.hpp"
#include "base/include/utils.hpp"

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


namespace {

void * allocate_type(const std::type_info & ti, std::function<void (void*)> & deallocator){
    TClass * class_ = TBuffer::GetClass(ti);
    EDataType dt = TDataType::GetType(ti);
    void * result;
    if(class_){
        result = class_->New();
        deallocator = class_->GetDelete();
    }
    else{
        if(dt == kOther_t || dt == kNoType_t){
            throw runtime_error("allocate_type with type called which is not known to ROOT");
        }
        result = new uint64_t(0); // largest primitive type
        deallocator = [](void * addr) {delete reinterpret_cast<uint64_t*>(addr);};
    }
    return result;
}


char DataTypeToChar(EDataType datatype){
    switch(datatype) {
        case kChar_t:     return 'B';
        case kUChar_t:    return 'b';
        case kBool_t:     return 'O';
        case kShort_t:    return 'S';
        case kUShort_t:   return 's';
        case kCounter:
        case kInt_t:      return 'I';
        case kUInt_t:     return 'i';
        case kDouble_t:
        case kDouble32_t: return 'D';
        case kFloat_t:
        case kFloat16_t:  return 'F';
        case kLong_t:     return sizeof(Long_t) == sizeof(Long64_t) ? 'L' : 'I';
        case kULong_t:    return sizeof(ULong_t) == sizeof(ULong64_t) ? 'l' : 'i';
        case kchar:       return 0; // unsupported
        case kLong64_t:   return 'L';
        case kULong64_t:  return 'l';

        case kCharStar:   return 'C';
        case kBits:       return 0; //unsupported

        case kOther_t:
        case kNoType_t:
        default:
            return 0;
    }
}

const std::type_info & get_type(TBranch * branch){
    TClass * branch_class = 0;
    EDataType branch_dtype = kNoType_t;
    int res = branch->GetExpectedType(branch_class, branch_dtype);
    if (res > 0) {
        throw runtime_error("input branch '" + string(branch->GetName()) + "': error calling GetExpectedType");
    }
    if(branch_class){
        return *branch_class->GetTypeInfo();
    }
    else{
        // normalize branch type to use explicit sizes for some types:
        if(branch_dtype == kLong_t){
            if(sizeof(Long_t) == sizeof(Long64_t)){
                branch_dtype = kLong64_t;
            }
            else{
                assert(sizeof(Long_t) == sizeof(Int_t));
                branch_dtype = kInt_t;
            }
        }
        else if(branch_dtype == kULong_t){
            if(sizeof(ULong_t) == sizeof(ULong64_t)){
                branch_dtype = kULong64_t;
            }
            else{
                assert(sizeof(Long_t) == sizeof(Int_t));
                branch_dtype = kUInt_t;
            }
        }
        switch(branch_dtype){
            case kChar_t:     return typeid(Char_t);
            case kUChar_t:    return typeid(UChar_t);
            case kBool_t:     return typeid(Bool_t);
            case kShort_t:    return typeid(Short_t);
            case kUShort_t:   return typeid(UShort_t);
            case kCounter:
            case kInt_t:      return typeid(Int_t);
            case kUInt_t:     return typeid(UInt_t);
            case kDouble_t:
            case kDouble32_t: return typeid(Double_t);
            case kFloat_t:
            case kFloat16_t:  return typeid(Float_t);
            case kLong64_t:   return typeid(int64_t);
            case kULong64_t:  return typeid(uint64_t);
            
            case kLong_t:     // cannot happen as of above normalization
            case kULong_t:    assert(false);

            case kOther_t:
            case kNoType_t:
            default: throw runtime_error("unknown non-class type for branch '" + string(branch->GetName()) + "' / type not known to ROOT");
        }
    }
}


void write_type_info(ostream & out, const std::type_info & ti) {
    out << "'" << demangle(ti.name()) << "'";
    TClass * class_ = TBuffer::GetClass(ti);
    EDataType dt = TDataType::GetType(ti);
    if (class_) {
        out << " (ROOT class name: '" << class_->GetName() << "')";
    }
    else {
        auto tn = TDataType::GetTypeName(dt);
        if (tn == string("")) {
            out << " (type not known to ROOT!)";
        }
        else {
            out << " (ROOT name: '" << tn << "')";
        }
    }
}

namespace detail {
bool is_class(const std::type_info & ti){
    return TBuffer::GetClass(ti) != nullptr;
}
}

}


InTree::InTree(TTree * tree_, bool lazy_): tree(tree_), lazy(lazy_){
    n_entries = tree->GetEntries();
}

void InTree::open_branch(const std::type_info & ti, const std::string & branchname, Event & event, const Event::RawHandle & handle){
    for(const auto & bi : branch_infos){
        if(bi.branchname == branchname){
            throw invalid_argument("InTree::open_branch: tried to open branch '" + branchname + "' twice.");
        }
    }
    if(branchname.empty()){
        throw invalid_argument("InTree::open_branch: tried to open empty branchname in tree '" + string(tree->GetName()) + "'");
    }
    TBranch * branch = tree->GetBranch(branchname.c_str());
    if(branch == nullptr){
        throw runtime_error("InTree::open_branch: Did not find branch '" + branchname + "'");
    }
    const std::type_info & branch_type = get_type(branch);
    if(branch_type != ti){
        stringstream ss;
        ss << "InTree: Type error for branch '" << branchname << "': branch holds type ";
        write_type_info(ss, branch_type);
        ss << ", but tried to read into object of type ";
        write_type_info(ss, ti);
        ss << "";
        throw runtime_error(ss.str());
    }
    void * addr = event.get(ti, handle, Event::state::nonexistent);
    if(addr == nullptr){
        std::function<void (void*)> eraser;
        addr = allocate_type(ti, eraser);
        event.set(ti, handle, addr, move(eraser));
        event.set_validity(ti, handle, false);
    }
    if(detail::is_class(branch_type)){
        ptrs.push_back(addr);
        branch->SetAddress(&ptrs.back());
    }
    else{
        branch->SetAddress(addr);
    }
    branch_infos.emplace_back(branchname, branch, event, handle);
    auto & bi = branch_infos.back();
    if(lazy){
        event.set_get_callback(ti, handle, [&bi, this](){ this->read_branch(bi); });
    }
}

void InTree::read_branch(const binfo & bi){
    int res =  bi.branch->GetEntry(current_index);
    if(res < 0){
        stringstream ss;
        ss << "Error from TBranch::GetEntry reading entry " << current_index;
        throw runtime_error(ss.str());
    }
    bi.event.set_validity(bi.handle, true);
    bytes_read += res;
}

void InTree::get_entry(int64_t index){
    if(index < 0 || index >= n_entries){
        throw runtime_error("InTree::get_entry: index out of bounds");
    }
    current_index = index;
    if(lazy){
        for(const auto & bi : branch_infos){
            bi.event.set_validity(bi.handle, false);
        }
    }
    else{
        // read all:
        for(const auto & bi : branch_infos){
            read_branch(bi);
        }
    }
}

OutTree::OutTree(TTree * t): tree(t){
}

void OutTree::create_branch(const std::type_info & ti, const std::string & branchname, Event & event, const Event::RawHandle & handle){
    branch_infos.emplace_back(ti, branchname, event, handle);
    auto & bi = branch_infos.back();
    auto class_ = TBuffer::GetClass(ti);
    bi.is_class = class_ != nullptr;
    if(class_){
        assert(dynamic_cast<TEmulatedCollectionProxy*>(class_->GetCollectionProxy())==0); // if this fails, it means T is a STL without compiled STL dictionary
        std::function<void (void*)> deallocator;
        void * obj = allocate_type(ti, deallocator); // dummy object, only for the time of creating the branch. Actual address is set in append (object is not guaranteed to exist in Event at this point).
        bi.branch = tree->Branch(branchname.c_str(), class_->GetName(), &obj);
        deallocator(obj);
    }
    else{
        EDataType dt = TDataType::GetType(ti);
        if(dt==kOther_t or dt==kNoType_t) throw invalid_argument("tree_branch: unknown type");
        auto root_branch_type = DataTypeToChar(dt);
        assert(root_branch_type!=0);
        uint64_t tmp; // dummy object, see above.
        bi.branch = tree->Branch(branchname.c_str(), &tmp, (branchname + '/' + root_branch_type).c_str());
    }
}

void OutTree::append(){
    for(auto & bi : branch_infos){
        // make sure to read event content (even in case of lazy read / lazy evaluate):
        void * obj = bi.event.get(bi.ti, bi.handle);
        assert(obj != nullptr);
        // (re-)set branch addresses, if not yet done. Note that now, the object must be available in the event.
        if(!bi.address_setup){
            if(bi.is_class){
                addresses.push_back(obj);
                bi.branch->SetAddress(&addresses.back());
            }
            else{
                bi.branch->SetAddress(obj);
            }
            bi.address_setup = true;
        }
    }
    tree->Fill();
}

