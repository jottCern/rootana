#include "local.hpp"
#include "utils.hpp"
#include <boost/test/unit_test.hpp>
#include "ra/include/analysis.hpp"
#include "ra/include/context.hpp"
#include "ra/include/event.hpp"
#include "base/include/ptree-utils.hpp"

#include "TFile.h"
#include "TTree.h"

#include <fstream>
#include <set>

using namespace dra;
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

vector<int> get_tree_intdata(const string & filename, const string & branchname){
    TFile f(filename.c_str(), "read");
    TTree * tree = dynamic_cast<TTree*>(f.Get("events"));
    BOOST_REQUIRE(tree);
    int i;
    tree->SetBranchAddress(branchname.c_str(), &i);
    int maxk = tree->GetEntries();
    vector<int> result;
    result.reserve(maxk);
    for(int k=0; k < maxk; ++k){
        tree->GetEntry(k);
        result.push_back(i);
    }
    return result;
}

int fail_on = -1;

class test_module: public ra::AnalysisModule {
public:
    test_module(const ptree & cfg);
    virtual void begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out);
    virtual void process(Event & event);
    //virtual void begin_in_file(TFile & file){}
private:
    int offset;
    int outdata;
};


test_module::test_module(const ptree & cfg){
    offset = ptree_get<int>(cfg, "offset", 0);
}

void test_module::begin_dataset(const s_dataset & dataset, InputManager & in, OutputManager & out){
    in.declare_event_input<int>("intdata");
    out.declare_event_output<int>("intdata_out");
}

void test_module::process(Event & event){
    int data = event.get<int>("intdata");
    if(fail_on != -1 && fail_on == data){
        throw runtime_error("failure");
    }
    event.set<int>("intdata_out", data + offset);
}

REGISTER_ANALYSIS_MODULE(test_module);

string maketempdir(){
    char pattern[] = "/tmp/tc.XXXXXX";
    char * result = mkdtemp(pattern);
    if(result==0){
        throw runtime_error("error from mkdtemp");
    }
    return result;
}

void write_config(const string & outfilename, const string & in_pattern, const string & outdir, int offset){
    ofstream out(outfilename.c_str());
    out << "options {\n"
     " blocksize 237\n"
     " output_dir " << outdir << "\n"
     "}\n"
     "dataset {\n"
     " name testdataset\n"
     " treename events\n"
     " file-pattern " << in_pattern << "\n"
     "}\n"
     "modules { testm { type test_module \n"
     "  offset " << offset << "\n"
     "} }";
}

}


BOOST_AUTO_TEST_SUITE(processing)

BOOST_AUTO_TEST_CASE(processing1){
    const int offset = 2835985;
    const int offset_processing = 23;
    string tmpdir = maketempdir();
    cout << "processing1 test in " << tmpdir << endl;
    create_test_tree(tmpdir + "/testA.root", offset, 1000);
    write_config(tmpdir + "/cfg.cfg", tmpdir + "/test*.root", tmpdir, offset_processing);
    
    local_run(tmpdir + "/cfg.cfg", 1);
    
    // test output:
    string outfilename = tmpdir + "/testdataset.root";
    auto data = get_tree_intdata(outfilename, "intdata_out");
    BOOST_REQUIRE_EQUAL(data.size(), 1000);
    for(size_t i=0; i<data.size(); ++i){
        BOOST_CHECK_EQUAL(data[i], offset + i + offset_processing);
    }
}

// two workers, with merging:
BOOST_AUTO_TEST_CASE(processing2){
    const int offset = 2835985;
    const int offset_processing = 23;
    string tmpdir = maketempdir();
    cout << "processing2 test in " << tmpdir << endl;
    create_test_tree(tmpdir + "/testA.root", offset, 1000);
    create_test_tree(tmpdir + "/testB.root", offset + 2000, 1000);
    write_config(tmpdir + "/cfg.cfg", tmpdir + "/test*.root", tmpdir, offset_processing);
    
    local_run(tmpdir + "/cfg.cfg", 2);
    
    // check output, this time as set:
    set<int> expected_data;
    for(int i=0; i<1000; ++i){
        expected_data.insert(offset + offset_processing + i);
        expected_data.insert(offset + offset_processing + 2000 + i);
    }
    
    string outfilename = tmpdir + "/testdataset.root";
    auto data = get_tree_intdata(outfilename, "intdata_out");
    BOOST_REQUIRE_EQUAL(data.size(), 2000);
    
    set<int> out_data;
    out_data.insert(data.begin(), data.end());
    BOOST_REQUIRE_EQUAL(out_data.size(), 2000);
    BOOST_REQUIRE_EQUAL(expected_data.size(), 2000);
    BOOST_CHECK(std::equal(out_data.begin(), out_data.end(), expected_data.begin()));
}

BOOST_AUTO_TEST_CASE(processing3){
    const int offset = 2835985;
    const int offset_processing = 23;
    const int nfiles = 23;
    const int nworkers = 16;
    const int nevents = 100000; // per file
    string tmpdir = maketempdir();
    cout << "processing3 test in " << tmpdir << endl;
    for(int ifile = 0; ifile < nfiles; ++ifile){
        stringstream fname;
        fname << tmpdir << "/test" << ifile << ".root";
        create_test_tree(fname.str(), offset + ifile * nevents, nevents);
    }
    write_config(tmpdir + "/cfg.cfg", tmpdir + "/test*.root", tmpdir, offset_processing);
    local_run(tmpdir + "/cfg.cfg", nworkers);
        
    // check output:
    set<int> expected_data;
    for(int i=0; i<nevents; ++i){
        for(int ifile = 0; ifile < nfiles; ++ifile){
            expected_data.insert(offset + ifile * nevents + offset_processing + i);
        }
    }
    
    string outfilename = tmpdir + "/testdataset.root";
    auto data = get_tree_intdata(outfilename, "intdata_out");
    BOOST_REQUIRE_EQUAL(data.size(), nevents * nfiles);
    
    set<int> out_data;
    out_data.insert(data.begin(), data.end());
    BOOST_REQUIRE_EQUAL(out_data.size(), nevents * nfiles);
    BOOST_REQUIRE_EQUAL(expected_data.size(), nevents * nfiles);
    BOOST_CHECK(std::equal(out_data.begin(), out_data.end(), expected_data.begin()));
}

// test working on more than one dataset:
BOOST_AUTO_TEST_CASE(datasets){
    const int offsetA = 23985;
    const int offsetB = 2398567;
    const int offset = 87765;
    string tmpdir = maketempdir();
    cout << "processing/datasets test in " << tmpdir << endl;
    create_test_tree(tmpdir + "/testA1.root", offsetA, 1000);
    create_test_tree(tmpdir + "/testA2.root", offsetA + 1000, 1000);
    create_test_tree(tmpdir + "/testB1.root", offsetB, 1000);
    create_test_tree(tmpdir + "/testB2.root", offsetB + 1000, 1000);
    
    {
        ofstream out((tmpdir + "/cfg.cfg").c_str());
        out << "options {\n"
        " blocksize 237\n"
        " output_dir " << tmpdir << "\n"
        "}\n"
        "dataset {\n"
        " name testdatasetA\n"
        " treename events\n"
        " file-pattern " << tmpdir << "/testA*.root\n"
        "}\n"
        "dataset {\n"
        " name testdatasetB\n"
        " treename events\n"
        " file-pattern " << tmpdir << "/testB*.root\n"
        "}\n"
        "modules { testm { type test_module \n"
        "  offset " << offset << "\n"
        "} }";
    }
    local_run(tmpdir + "/cfg.cfg", 2);
    
    for(int id=0; id<2; ++id){ // id==0: A, id==1: B
        set<int> expected_data;
        const int input_offset = id==0 ? offsetA : offsetB;
        for(int i=0; i<2000; ++i){
            expected_data.insert(input_offset + offset + i);
        }
    
        string outfilename = tmpdir + "/testdataset";
        if(id==0) outfilename += "A.root";
        else outfilename += "B.root";
        auto data = get_tree_intdata(outfilename, "intdata_out");
        BOOST_REQUIRE_EQUAL(data.size(), 2000);
    
        set<int> out_data;
        out_data.insert(data.begin(), data.end());
        BOOST_REQUIRE_EQUAL(out_data.size(), 2000);
        BOOST_REQUIRE_EQUAL(expected_data.size(), 2000);
        BOOST_CHECK(std::equal(out_data.begin(), out_data.end(), expected_data.begin()));

    }
}


// TODO: test failing workers!


BOOST_AUTO_TEST_SUITE_END()
