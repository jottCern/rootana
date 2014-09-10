#include "context-backend.hpp"
#include "base/include/utils.hpp"
#include "base/include/ptree-utils.hpp"

#include "TClass.h"
#include "TFile.h"
#include "TTree.h"

using namespace ra;
using namespace std;

class TTreeInputManager: public InputManagerBackend {
public:
    virtual void declare_input(const char * bname, const std::string & event_member_name, void * addr, const std::type_info & ti) override;
    
    TTreeInputManager(Event & event_, const ptree & cfg);
    
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
        Event::RawHandle handle; // can be an invalid handle in case addr is outside the event container
        void * addr; // address of an object of type ti, either into the event container or to somewhere else
        
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
    event.invalidate_all();
    if(ientry >= nentries) throw runtime_error("read_entry called with index beyond current number of entries");
    current_ientry = ientry;
    if(!lazy){
        // read all:
        for(auto & name_bi : bname2bi){
            read_branch(name_bi.second);
            event.set_validity(name_bi.second.ti, name_bi.second.handle, true);
        }
    }
}

