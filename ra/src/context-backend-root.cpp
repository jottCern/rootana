#include "context-backend.hpp"
#include "base/include/utils.hpp"
#include "base/include/ptree-utils.hpp"

#include "TClass.h"
#include "TFile.h"
#include "TTree.h"
#include "TEmulatedCollectionProxy.h"
#include "TH1.h"

using namespace ra;
using namespace std;

class TTreeInputManager: public InputManagerBackend {
public:
    
    TTreeInputManager(Event & event_, const ptree & cfg);
    
    virtual void declare_input(const char * bname, const std::string & event_member_name, void * addr, const std::type_info & ti) override;
    
    virtual size_t setup_input_file(const string & treename, const std::string & filename) override;
    
    virtual void read_event(size_t ievent) override;

    virtual size_t nbytes_read() override {
        size_t result = bytes_read;
        bytes_read = 0;
        return result;
    }
    
private:
    struct branchinfo {
        TBranch * branch;
        const std::type_info & ti; // this is always a non-pointer type.
        Event::RawHandle handle;
        void * addr; // address of an object of type ti inside the event container
        
        branchinfo(TBranch * branch_, const std::type_info & ti_, const Event::RawHandle & handle_, void * addr_): branch(branch_), ti(ti_), handle(handle_), addr(addr_){}
    };
    
    void read_branch(branchinfo & bi);
    
    size_t current_ientry; // for installing read_branch as callback for the Event container, need to know what current index is.
    size_t nentries;
    size_t bytes_read;
    std::map<std::string, branchinfo> bname2bi;
    bool lazy;
    
    std::unique_ptr<TFile> file;
};

REGISTER_INPUT_MANAGER_BACKEND(TTreeInputManager, "root")

void TTreeInputManager::declare_input(const char * bname, const string & event_member_name, void * addr, const std::type_info & ti){
    if(bname2bi.find(bname) != bname2bi.end()){
         throw runtime_error("InputManager: input for branch name '" + string(bname) + "' declared multiple times. This is not allowed.");
    }
    Event::RawHandle handle = event.get_raw_handle(ti, event_member_name);
    bname2bi.insert(pair<string, branchinfo>(bname, branchinfo(0, ti, handle, addr)));
}

namespace{

void write_type_info(ostream & out, TClass * class_, EDataType type_){
    if(class_){
        out << class_->GetName();
        out << "(c++ name: '" << demangle(class_->GetTypeInfo()->name()) << "')";
    }
    else{
        auto tn = TDataType::GetTypeName(type_);
        if(tn == string("")){
            out << "(type not known to ROOT!)";
        }
        else{
            out << tn;
        }
    }
}

}

TTreeInputManager::TTreeInputManager(Event & event_, const ptree & cfg): InputManagerBackend(event_), current_ientry(0), nentries(0), bytes_read(0) {
    lazy = ptree_get<bool>(cfg, "lazy", false);
}

size_t TTreeInputManager::setup_input_file(const string & treename, const std::string & filename){
    file.reset(new TFile(filename.c_str(), "read"));
    if(!file->IsOpen()){
        throw runtime_error("TTreeInputManager::setup_input_file: Error opening root file '" + filename + "'");
    }
    TTree * tree = dynamic_cast<TTree*>(file->Get(treename.c_str()));
    if(!tree){
        throw runtime_error("TTreeInputManager::setup_input_file: did not find TTree '" + treename + "' in file '" + filename + "'");
    }
    nentries = 0;
    for(auto & name_bi : bname2bi){
        const string & bname = name_bi.first;
        branchinfo & bi = name_bi.second;
        TBranch * branch = tree->GetBranch(bname.c_str());
        if(branch==0){
            throw runtime_error("Could not find branch '" + bname + "' in tree '" + tree->GetName() + "'");
        }
        // now do SetBranchAddress, using address-to-address in case of a class, and simple address otherwise.
        // get branch type:
        TClass * branch_class = 0;
        EDataType branch_dtype = kNoType_t;
        int res = branch->GetExpectedType(branch_class, branch_dtype);
        if(res > 0){
            throw runtime_error("input branch '" + bname + "': error reading type information");
        }
        
        // get address type:
        TClass * address_class = TBuffer::GetClass(bi.ti);
        EDataType address_dtype = kNoType_t;
        TDataType * address_tdatatype = TDataType::GetDataType(TDataType::GetType(bi.ti));
        if(address_tdatatype){
            address_dtype = EDataType(address_tdatatype->GetType());
        }
        
        // compare if they match; note that it's an error if the class or data type has not been found:
        if((branch_class && address_class) && ((branch_class == address_class) || (branch_class->GetTypeInfo() == address_class->GetTypeInfo()))){
            branch->SetAddress(&bi.addr);
        }
        else if(branch_dtype != kNoType_t && (branch_dtype == address_dtype)){
            branch->SetAddress(bi.addr);
        }
        else{ // this is an error:
            stringstream ss;
            ss << "Type error for branch '" << bname << "': branch holds type '";
            write_type_info(ss, branch_class, branch_dtype);
            ss << "', but the type given in declare_input was '";
            write_type_info(ss, address_class, address_dtype);
            ss << "'";
            throw runtime_error(ss.str());
        }
        bi.branch = branch;
        // if we're lazy, install the callbacks:
        if(lazy){
            boost::optional<std::function< void()> > callback(std::bind(&TTreeInputManager::read_branch, this, ref(bi)));
            event.set_get_callback(bi.ti, bi.handle, std::move(callback));
        }
    }
    nentries = tree->GetEntries();
    return nentries;
}


void TTreeInputManager::read_branch(branchinfo & bi){
    int res =  bi.branch->GetEntry(current_ientry);
    if(res < 0){
        stringstream ss;
        ss << "Error from TBranch::GetEntry reading entry " << current_ientry;
        throw runtime_error(ss.str());
    }
    bytes_read += res;
}


void TTreeInputManager::read_event(size_t ientry){
    if(ientry >= nentries) throw runtime_error("read_entry called with index beyond current number of entries");
    current_ientry = ientry;
    if(lazy){
        // mark all invalid to trigger read:
        for(auto & name_bi : bname2bi){
            event.set_validity(name_bi.second.ti, name_bi.second.handle, false);
        }
    }
    else{
        // read all:
        for(auto & name_bi : bname2bi){
            read_branch(name_bi.second);
            event.set_validity(name_bi.second.ti, name_bi.second.handle, true);
        }
    }
}



class TFileOutputManager: public OutputManagerBackend {
public:
    // outfile ownership is taken by the TFileOutputManager.
    TFileOutputManager(Event & event, const std::string & event_treename, const std::string & base_outfilename);
    
    virtual void put(const char * name, TH1 * t) override;
    virtual void declare_event_output(const char * name, const std::string & event_member_name, const void * addr, const std::type_info & ti) override;
    virtual void declare_output(const identifier & tree_id, const char * name, const void * t, const std::type_info & ti) override;
    virtual void write_output(const identifier & tree_id) override;
    
    virtual void write_event() override;
    
    // write and close the underlying TFile. Any put or write after that is illegal.
    // Called from the destructor automatically, so often no need to do it explicitly.
    virtual void close() override;
    
    virtual ~TFileOutputManager();
    
private:
    std::unique_ptr<TFile> outfile;
    std::string event_treename;
    TTree * event_tree; // owned by outfile
    std::vector<std::pair<Event::RawHandle, const std::type_info*>> event_members; // list of event members to read before the write; important for lazy read
    std::map<identifier, TTree*> trees; // additional trees beyond the event tree
    std::list<void*> ptrs; // keep a list of pointers, so we can give root the *address* of the pointer
};


class TFileOutputManagerOperations: public OutputManagerOperations {
public:
    virtual std::string filename_extension() const {
        return "root";
    }
};

REGISTER_OUTPUT_MANAGER_BACKEND(TFileOutputManager, TFileOutputManagerOperations, "root")


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

TFileOutputManager::TFileOutputManager(Event & event_, const string & event_treename_, const string & base_outfilename):
    OutputManagerBackend(event_), event_treename(event_treename_),event_tree(0){
        string filename_full = base_outfilename + ".root";
    outfile.reset(new TFile(filename_full.c_str(), "recreate"));
    if(!outfile->IsOpen()){
        throw runtime_error("Error opening output file '" + filename_full + "'");
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
