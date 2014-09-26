#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/context.hpp"
#include "ra/include/config.hpp"
#include "ra/include/controller.hpp"
#include "base/include/ptree-utils.hpp"

#include <boost/test/unit_test.hpp>
#include <fstream>

#include "TFile.h"
#include "TTree.h"

using namespace ra;
using namespace std;


namespace {
    
void create_test_tree(const string & filename, int i0, int nentries){
    TFile f(filename.c_str(), "recreate");
    BOOST_REQUIRE(f.IsOpen());
    TTree * tree = new TTree("events", "events");
    int i;
    tree->Branch("intdata", &i, "intdata/I");
    for(int k=0; k<nentries; ++k){
        i = i0 + k;
        tree->Fill();
    }
    tree->Write();
    delete tree;
}

vector<int> ids_seen;

class test_module: public ra::AnalysisModule {
public:
    test_module(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
private:
    int offset;
    int outdata;
    Event::Handle<int> h_intdata;
};


test_module::test_module(const ptree & cfg){
    offset = ptree_get<int>(cfg, "offset", 0);
}

void test_module::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    in.declare_event_input<int>("intdata");
    h_intdata = in.get_handle<int>("intdata");
}

void test_module::process(Event & event){
    int id = event.get(h_intdata);
    ids_seen.push_back(id);
}

REGISTER_ANALYSIS_MODULE(test_module);

class test_module_copy: public ra::AnalysisModule {
public:
    test_module_copy(const ptree & cfg){}
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
        in.declare_event_input<int>("intdata");
        out.declare_event_output<int>("intdata");
    }
    virtual void process(Event & event){}
};

REGISTER_ANALYSIS_MODULE(test_module_copy)

string maketempdir(){
    char pattern[] = "/tmp/tc.XXXXXX";
    char * result = mkdtemp(pattern);
    if(result==0){
        throw runtime_error("error from mkdtemp");
    }
    return result;
}

}

BOOST_AUTO_TEST_SUITE(controller)

BOOST_AUTO_TEST_CASE(simple){
    const int offset = 2835985;
    string indir = maketempdir();
    create_test_tree(indir + "/test.root", offset, 1000);
    {
    ofstream configstr(indir + "/cfg.cfg");
    configstr << "options {}\n"
     "dataset {\n"
     " name testdataset\n"
     " treename events\n"
     " file-pattern " << indir << "/*.root\n"
     "}\n"
     "modules { testm { type test_module } }";
    }
    
    s_config conf(indir + "/cfg.cfg");
    
    {
       AnalysisController ac(conf, false);
    
       BOOST_CHECK_THROW(ac.start_file(0), invalid_argument);
       ac.start_dataset(0, indir + "/out");
       BOOST_CHECK_THROW(ac.start_file(1), invalid_argument);
       ac.start_file(0);
       
       ids_seen.clear();
       AnalysisController::ProcessStatistics s;
       ac.process(0, 1000, &s);
       BOOST_CHECK_GT(s.nbytes_read, 1000); // should be around 4000 ...
    }
    // TODO: check for resource leaks (=open files)
    BOOST_REQUIRE_EQUAL(ids_seen.size(), 1000);
    for(int i=0; i<1000; ++i){
        BOOST_CHECK_EQUAL(ids_seen[i], i + offset);
    }
}

BOOST_AUTO_TEST_CASE(lazy){
    string indir = maketempdir();
    const int offset = 9824;
    create_test_tree(indir + "/test.root", offset, 1000);
    {
    ofstream configstr(indir + "/cfg.cfg");
    configstr << 
     "input {\n"
     "   type root\n"
     "   lazy true \n"
     "}\n"
     "dataset {\n"
     " name testdataset\n"
     " treename events\n"
     " file-pattern " << indir << "/*.root\n"
     "}\n"
     "modules { testm { type test_module_copy } }";
    }
    
    s_config conf(indir + "/cfg.cfg");
    
    {
       AnalysisController ac(conf, false);
       ac.start_dataset(0, indir + "/out");
       ac.start_file(0);
       ac.process(0, 1000, 0);
    }
    // read out.root: should have same structure as in:
    TFile out((indir + "/out.root").c_str(), "read");
    TTree * tree = dynamic_cast<TTree*>(out.Get("events"));
    BOOST_REQUIRE(tree);
    BOOST_REQUIRE_EQUAL(tree->GetEntries(), 1000);
    int id = -1;
    tree->SetBranchAddress("intdata", &id);
    for(int i=0; i<1000; ++i){
        tree->GetEntry(i);
        BOOST_CHECK_EQUAL(id, offset + i);
    }
}



BOOST_AUTO_TEST_CASE(morefiles){
    const int offset0 = 2835985;
    const int offset1 = 9875674;
    string indir = maketempdir();
    create_test_tree(indir + "/test0.root", offset0, 1000);
    create_test_tree(indir + "/test1.root", offset1, 1000);
    {
    ofstream configstr(indir + "/cfg.cfg");
    configstr << "options {}\n"
     "dataset {\n"
     " name testdataset\n"
     " treename events\n"
     " file-pattern " << indir << "/*.root\n"
     "}\n"
     "modules { testm { type test_module } }";
    }
    
    s_config conf(indir + "/cfg.cfg");
    
    ids_seen.clear();
    {
       AnalysisController ac(conf, false);
       ac.start_dataset(0, indir + "/out");
       ac.start_file(0);
       ac.process(0, size_t(-1));
       ac.start_file(1);
       ac.process(0, size_t(-1));
    }
    // TODO: check for resource leaks (=open files)
    BOOST_REQUIRE_EQUAL(ids_seen.size(), 2000);
    for(int i=0; i<1000; ++i){
        BOOST_CHECK_EQUAL(ids_seen[i], i + offset0);
    }
    for(int i=0; i<1000; ++i){
        BOOST_CHECK_EQUAL(ids_seen[i + 1000], i + offset1);
    }
}


BOOST_AUTO_TEST_SUITE_END()
