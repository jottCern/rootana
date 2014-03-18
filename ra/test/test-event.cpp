#include <boost/test/unit_test.hpp>

#include "event.hpp"
#include "context.hpp"
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"

using namespace ra;
using namespace std;


namespace {

int make_test_tree(const char * filename){
    TFile f(filename, "recreate");
    TTree * tree = new TTree("test", "test");
    int i;
    double d;
    vector<float> floats(3);
    vector<float> * pfloats = &floats;
    tree->Branch("intdata", &i, "intdata/I");
    tree->Branch("doubledata", &d, "doubledata/D");
    tree->Branch("floats", &pfloats);
    for(int k=0; k<100; ++k){
        i = k+1;
        d = 100. + k;
        floats = {0.3f + k, 0.4f + k, k - 1000.f};
        tree->Fill();
    }
    tree->Write();
    delete tree;
    return 0;
}

int dummy = make_test_tree("tree.root");
}


BOOST_AUTO_TEST_SUITE(event)

BOOST_AUTO_TEST_CASE(simple){
    Event e;
    BOOST_CHECK(!e.exists<int>("test"));
    BOOST_CHECK_THROW(e.get<int>("test"), std::runtime_error);
    
    e.set<int>("test", 5);
    BOOST_CHECK_EQUAL(e.get<int>("test"), 5);
    
    e.get<int>("test") = 10;
    BOOST_CHECK_EQUAL(e.get<int>("test"), 10);
    
    e.erase<int>("test");
    BOOST_CHECK_THROW(e.get<int>("test"), std::runtime_error);
}

// test that addresses of the data in Event do not change across calls to 'set'.
BOOST_AUTO_TEST_CASE(addresses){
    Event e;
    e.set<int>("test", 5);
    
    int & idata = e.get<int>("test");
    BOOST_CHECK_EQUAL(idata, 5);
    
    e.set<int>("test", 8);
    BOOST_CHECK_EQUAL(idata, 8);
    BOOST_CHECK_EQUAL(&(e.get<int>("test")), &idata);
}

BOOST_AUTO_TEST_CASE(read){
    TFile f("tree.root", "read");
    TTree * tree = dynamic_cast<TTree*>(f.Get("test"));
    BOOST_REQUIRE(tree);
    
    
    // check that reading intdata works:
    Event event;
    TTreeInputManager in(event);
    in.declare_event_input<int>("intdata");
    in.setup_tree(tree);
    
    identifier intdata("intdata");
    
    BOOST_REQUIRE_EQUAL(in.entries(), size_t(100));
    for(int i=0; i<100; ++i){
        in.read_entry(i);
        int idata = event.get<int>(intdata);
        BOOST_CHECK_EQUAL(idata, i+1);
    }
    BOOST_CHECK_THROW(event.get<double>("doubledata"), std::runtime_error);
   
}

BOOST_AUTO_TEST_CASE(read_wrong_type){
    TFile f("tree.root", "read");
    TTree * tree = dynamic_cast<TTree*>(f.Get("test"));
    BOOST_REQUIRE(tree);
    
    // it should not work to read intdata as double:
    Event event;
    TTreeInputManager in(event);
    in.declare_event_input<double>("intdata");
    BOOST_CHECK_THROW(in.setup_tree(tree), std::runtime_error);
}


BOOST_AUTO_TEST_CASE(outtree){
    TFile * outfile = new TFile("out.root", "recreate");
    TFileOutputManager fout(outfile, "eventtree");
    OutputManager & out = fout;    
    
    Event event;
    BOOST_CHECK_THROW(out.declare_output<int>(event, "my_int"), std::runtime_error);
    
    event.set<int>("my_int", 5);
    out.declare_output<int>(event, "my_int");
    for(int i=0; i<100; ++i){
        event.get<int>("my_int") = i;
        fout.write_event();
    }
    
    // important: writing and closing the outfile is not the responsibility of OutputManager ...
    outfile->cd();
    outfile->Write();
    delete outfile;
    
    // check that the output file is Ok:
    TFile f("out.root", "read");
    TTree * tree = dynamic_cast<TTree*>(f.Get("eventtree"));
    Event inevent;
    TTreeInputManager in(inevent);
    in.declare_event_input<int>("my_int");
    in.setup_tree(tree);
    BOOST_REQUIRE_EQUAL(in.entries(), size_t(100));
    for(int i=0; i<100; ++i){
        in.read_entry(i);
        int idata = inevent.get<int>("my_int");
        BOOST_CHECK_EQUAL(idata, i);
    }
}


BOOST_AUTO_TEST_CASE(outhist){
    TFile * outfile = new TFile("outhist.root", "recreate");
    TFileOutputManager fout(outfile, "eventtree");
    
    TH1D * histo = new TH1D("h1", "h1", 100, 0, 1);
    fout.put("histname", histo);
    histo->Fill(0.5);
    
    outfile->cd();
    outfile->Write();
    outfile->Close();
    
    // check that h1 was written as "outhist":
    TFile f("outhist.root", "read");
    TH1D * inhist = dynamic_cast<TH1D*>(f.Get("histname"));
    BOOST_REQUIRE(inhist);
    BOOST_CHECK_EQUAL(inhist->GetEntries(), 1);
}

BOOST_AUTO_TEST_CASE(outhist_dir){
    TFile * outfile = new TFile("outhist_dir.root", "recreate");
    TFileOutputManager fout(outfile, "eventtree");
    
    TH1D * histo = new TH1D("h1", "h1", 100, 0, 1);
    fout.put("dir1/dir2/dir3/histname", histo);
    histo->Fill(0.5);
    
    outfile->cd();
    outfile->Write();
    outfile->Close();
    
    // check that h1 was written as "outhist":
    TFile f("outhist_dir.root", "read");
    TH1D * inhist = dynamic_cast<TH1D*>(f.Get("dir1/dir2/dir3/histname"));
    BOOST_REQUIRE(inhist);
    BOOST_CHECK_EQUAL(inhist->GetEntries(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

