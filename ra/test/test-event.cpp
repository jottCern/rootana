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
    BOOST_CHECK(e.get_state<int>("test") == Event::state::nonexistent);
    BOOST_CHECK_THROW(e.get<int>("test"), std::runtime_error);
    
    e.set<int>("test", 5);
    BOOST_CHECK_EQUAL(e.get<int>("test"), 5);
    
    e.get<int>("test") = 10;
    BOOST_CHECK_EQUAL(e.get<int>("test"), 10);
    
    int * ptr = &e.get<int>("test");
    
    e.set_state<int>("test", Event::state::invalid);
    BOOST_CHECK_THROW(e.get<int>("test"), std::runtime_error);
    BOOST_CHECK_EQUAL(&e.get<int>("test", false), ptr);
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

BOOST_AUTO_TEST_CASE(unset){
    Event e;
    BOOST_CHECK_EQUAL(e.get_state<int>("itest"), Event::state::nonexistent);
    
    e.set<int>("itest", 17);
    BOOST_CHECK_EQUAL(e.get_state<int>("itest"), Event::state::valid);
    BOOST_REQUIRE_EQUAL(e.get<int>("itest"), 17);
    const void * itestptr0 = &e.get<int>("itest");
    
    // unsetting makes present and allocated and 'get' behave as expected:
    e.set_state<int>("itest", Event::state::invalid);
    BOOST_CHECK_EQUAL(e.get_state<int>("itest"), Event::state::invalid);
    BOOST_CHECK_THROW(e.get<int>("itest"), std::runtime_error);
    
    // unsetting twice does not harm:
    e.set_state<int>("itest", Event::state::invalid);
    BOOST_CHECK_EQUAL(e.get_state<int>("itest"), Event::state::invalid);
    
    e.set<int>("itest", 23);
    BOOST_REQUIRE_EQUAL(e.get<int>("itest"), 23);
    const void * itestptr1 = &e.get<int>("itest");
    BOOST_CHECK_EQUAL(itestptr0, itestptr1);
    
    // setting to nonexistent is not allowed:
    BOOST_CHECK_THROW(e.set_state<int>("itest", Event::state::nonexistent), std::invalid_argument);
    // setting a nonexistent member to valid/invalid is not allowed:
    BOOST_CHECK_THROW(e.set_state<int>("itest_nonexistent", Event::state::valid), std::invalid_argument);
    BOOST_CHECK_THROW(e.set_state<int>("itest_nonexistent", Event::state::invalid), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(get_callback){
    Event e;
    BOOST_CHECK_EQUAL(e.get_state<int>("itest"), Event::state::nonexistent);
    
    int n_callback_called = 0;
    e.set<int>("itest", -1);
    int & iref = e.get<int>("itest");
    auto callback = [&](){++n_callback_called; iref = 5;};
    e.set_get_callback<int>("itest", callback);
    
    // set_get_callback should reset status to allocated:
    BOOST_CHECK_EQUAL(e.get_state<int>("itest"), Event::state::invalid);
    
    int iresult = e.get<int>("itest");
    BOOST_CHECK_EQUAL(iresult, 5);
    BOOST_CHECK_EQUAL(n_callback_called, 1);
    
    // calling 'get' again should not call callback again:
    iresult = e.get<int>("itest");
    BOOST_CHECK_EQUAL(iresult, 5);
    BOOST_CHECK_EQUAL(n_callback_called, 1);
    
    // but should call callback again if presence is reset:
    e.set_state<int>("itest", Event::state::invalid);
    iref = 0;
    iresult = e.get<int>("itest");
    BOOST_CHECK_EQUAL(iresult, 5);
    BOOST_CHECK_EQUAL(n_callback_called, 2);
    
    // check resetting the callback:
    e.set_state<int>("itest", Event::state::invalid);
    e.reset_get_callback<int>("itest");
    iref = 0;
    BOOST_CHECK_THROW(iresult = e.get<int>("itest"), std::runtime_error);
}



BOOST_AUTO_TEST_CASE(read){
    TFile f("tree.root", "read");
    BOOST_REQUIRE(f.IsOpen());
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


BOOST_AUTO_TEST_CASE(read_lazy){
    TFile f("tree.root", "read");
    BOOST_REQUIRE(f.IsOpen());
    TTree * tree = dynamic_cast<TTree*>(f.Get("test"));
    BOOST_REQUIRE(tree);
    
    Event event;
    TTreeInputManager in(event, true);
    in.declare_event_input<int>("intdata");
    in.setup_tree(tree);
    
    identifier intdata("intdata");
    
    BOOST_REQUIRE_EQUAL(in.entries(), size_t(100));
    for(int i=0; i<100; ++i){
        in.read_entry(i);
        int idata = event.get<int>(intdata);
        BOOST_CHECK_EQUAL(idata, i+1);
    }
    // we should have read 100 ints now:
    BOOST_CHECK_EQUAL(in.nbytes_read(), size_t(100) * sizeof(int));
}

// use inputmanager to read data without using an Event
BOOST_AUTO_TEST_CASE(read_noevent){
    TFile f("tree.root", "read");
    BOOST_REQUIRE(f.IsOpen());
    TTree * tree = dynamic_cast<TTree*>(f.Get("test"));
    BOOST_REQUIRE(tree);
    
    Event event;
    TTreeInputManager in(event);
    int int_in;
    in.declare_input("intdata", "", &int_in, typeid(int));
    in.setup_tree(tree);
    
    BOOST_REQUIRE_EQUAL(in.entries(), size_t(100));
    for(int i=0; i<100; ++i){
        in.read_entry(i);
        BOOST_CHECK_EQUAL(int_in, i+1);
    }
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
    Event event;
    TFileOutputManager fout(outfile, "eventtree", event);
    OutputManager & out = fout;    
    
    out.declare_event_output<int>("my_int");
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
    Event event;
    TFileOutputManager fout(outfile, "eventtree", event);
    
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
    Event event;
    TFileOutputManager fout(outfile, "eventtree", event);
    
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

/*
BOOST_AUTO_TEST_CASE(visitor){
    Event event;
    event.set<int>("membername", 15);
    
    std::vector<Event::Element> elements;
    event.visit([&elements](Event::Element elem){
            elements.emplace_back(move(elem));
        });
    
    BOOST_REQUIRE_EQUAL(elements.size(), 1);
    BOOST_CHECK_EQUAL(get<0>(elements[0]).name(), "membername");
    BOOST_CHECK_EQUAL(get<1>(elements[0]).name(), typeid(int).name());
    BOOST_CHECK_EQUAL(*reinterpret_cast<const int*>(get<2>(elements[0])), 15);
    BOOST_CHECK_EQUAL(get<3>(elements[0]), true);     
}*/

BOOST_AUTO_TEST_SUITE_END()

