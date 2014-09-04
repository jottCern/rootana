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
    auto h = e.get_handle<int>("test");
    BOOST_CHECK(e.get_state(h) == Event::state::nonexistent);
    BOOST_CHECK_THROW(e.get(h), std::runtime_error);
    
    e.set(h, 5);
    BOOST_CHECK_EQUAL(e.get(h), 5);
    
    e.get(h) = 10;
    BOOST_CHECK_EQUAL(e.get(h), 10);
    
    int * ptr = &e.get(h);
    
    e.set_validity(h, false);
    BOOST_CHECK_THROW(e.get(h), std::runtime_error);
    BOOST_CHECK_EQUAL(&e.get(h, false), ptr);
}


// test that addresses of the data in Event do not change across calls to 'set'.
BOOST_AUTO_TEST_CASE(addresses){
    Event e;
    auto h = e.get_handle<int>("test");
    e.set(h, 5);
    
    int & idata = e.get(h);
    BOOST_CHECK_EQUAL(idata, 5);
    
    e.set(h, 8);
    BOOST_CHECK_EQUAL(idata, 8);
    BOOST_CHECK_EQUAL(&(e.get(h)), &idata);
}

// check that getting handles with same name/type is the same:
BOOST_AUTO_TEST_CASE(handles){
    Event e;
    auto h0 = e.get_handle<int>("test");
    e.set(h0, 5);
    
    BOOST_CHECK_EQUAL(e.name(h0), "test");
    
    auto h1 = e.get_handle<int>("test");
    BOOST_CHECK_EQUAL(e.get(h1), 5);
    
    BOOST_CHECK_EQUAL(e.name(h1), "test");
    
    // same name, other type:
    auto h2 = e.get_handle<float>("test");
    BOOST_CHECK_EQUAL(e.get_state(h2), Event::state::nonexistent);
    BOOST_CHECK_EQUAL(e.name(h2), "test");
}

BOOST_AUTO_TEST_CASE(unset){
    Event e;
    auto h_itest = e.get_handle<int>("itest");
    BOOST_CHECK_EQUAL(e.get_state(h_itest), Event::state::nonexistent);
    
    e.set(h_itest, 17);
    BOOST_CHECK_EQUAL(e.get_state(h_itest), Event::state::valid);
    BOOST_REQUIRE_EQUAL(e.get(h_itest), 17);
    const void * itestptr0 = &e.get(h_itest);
    
    // unsetting makes present and allocated and 'get' behave as expected:
    e.set_validity(h_itest, false);
    BOOST_CHECK_EQUAL(e.get_state(h_itest), Event::state::invalid);
    BOOST_CHECK_THROW(e.get(h_itest), std::runtime_error);
    
    // unsetting twice does not harm:
    e.set_validity(h_itest, false);
    BOOST_CHECK_EQUAL(e.get_state(h_itest), Event::state::invalid);
    
    e.set(h_itest, 23);
    BOOST_REQUIRE_EQUAL(e.get(h_itest), 23);
    const void * itestptr1 = &e.get(h_itest);
    BOOST_CHECK_EQUAL(itestptr0, itestptr1);
}


BOOST_AUTO_TEST_CASE(get_callback){
    Event e;
    auto h = e.get_handle<int>("itest");
    BOOST_CHECK_EQUAL(e.get_state(h), Event::state::nonexistent);
    
    int n_callback_called = 0;
    e.set(h, -1);
    int & iref = e.get(h);
    auto callback = [&](){++n_callback_called; iref = 5;};
    boost::optional<std::function<void ()>> cb(callback);
    e.set_get_callback(h, cb);
    
    // set_get_callback should reset status to allocated:
    BOOST_CHECK_EQUAL(e.get_state(h), Event::state::invalid);
    
    int iresult = e.get(h);
    BOOST_CHECK_EQUAL(iresult, 5);
    BOOST_CHECK_EQUAL(n_callback_called, 1);
    
    // calling 'get' again should not call callback again:
    iresult = e.get(h);
    BOOST_CHECK_EQUAL(iresult, 5);
    BOOST_CHECK_EQUAL(n_callback_called, 1);
    
    // but should call callback again if presence is reset:
    e.set_validity(h, false);
    iref = 0;
    iresult = e.get(h);
    BOOST_CHECK_EQUAL(iresult, 5);
    BOOST_CHECK_EQUAL(n_callback_called, 2);
    
    // check resetting the callback:
    e.set_validity(h, false);
    e.set_get_callback(h, boost::none);
    iref = 0;
    BOOST_CHECK_THROW(iresult = e.get(h), std::runtime_error);
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
    
    auto h_intdata = event.get_handle<int>("intdata");
    
    BOOST_REQUIRE_EQUAL(in.entries(), size_t(100));
    for(int i=0; i<100; ++i){
        in.read_entry(i);
        int idata = event.get(h_intdata);
        BOOST_CHECK_EQUAL(idata, i+1);
    }
    //BOOST_CHECK_THROW(event.get<double>("doubledata"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(read_lazy){
    TFile f("tree.root", "read");
    BOOST_REQUIRE(f.IsOpen());
    TTree * tree = dynamic_cast<TTree*>(f.Get("test"));
    BOOST_REQUIRE(tree);
    
    Event event;
    TTreeInputManager in(event, true);
    in.declare_event_input<int>("intdata");
    auto h_intdata = in.get_handle<int>("intdata");
    in.setup_tree(tree);
    
    BOOST_REQUIRE_EQUAL(in.entries(), size_t(100));
    for(int i=0; i<100; ++i){
        in.read_entry(i);
        int idata = event.get(h_intdata);
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
    { // create output file:
    Event event;
    unique_ptr<TFile> outfile(new TFile("out.root", "recreate"));
    TFileOutputManager fout(move(outfile), "eventtree", event);
    OutputManager & out = fout;
    
    out.declare_event_output<int>("my_int");
    auto h_my_int = event.get_handle<int>("my_int");
    for(int i=0; i<100; ++i){
        event.get(h_my_int) = i;
        fout.write_event();
    }
    }
    
    // check that the output file is Ok:
    TFile f("out.root", "read");
    TTree * tree = dynamic_cast<TTree*>(f.Get("eventtree"));
    BOOST_REQUIRE(tree);
    Event inevent;
    TTreeInputManager in(inevent);
    in.declare_event_input<int>("my_int");
    auto h_my_int = in.get_handle<int>("my_int");
    in.setup_tree(tree);
    BOOST_REQUIRE_EQUAL(in.entries(), size_t(100));
    for(int i=0; i<100; ++i){
        in.read_entry(i);
        int idata = inevent.get(h_my_int);
        BOOST_CHECK_EQUAL(idata, i);
    }
}


// output tree in a directory within the output file:
BOOST_AUTO_TEST_CASE(outtree_dir){
    {
    Event event;
    unique_ptr<TFile> outfile(new TFile("out.root", "recreate"));
    TFileOutputManager fout(move(outfile), "dir/eventtree", event);
    OutputManager & out = fout;
    
    out.declare_event_output<int>("my_int");
    auto h_my_int = event.get_handle<int>("my_int");
    for(int i=0; i<100; ++i){
        event.get(h_my_int) = i;
        fout.write_event();
    }
    }
    
    // check that the output file is Ok:
    TFile f("out.root", "read");
    TTree * tree = dynamic_cast<TTree*>(f.Get("dir/eventtree"));
    BOOST_REQUIRE(tree);
    Event inevent;
    TTreeInputManager in(inevent);
    in.declare_event_input<int>("my_int");
    in.setup_tree(tree);
    auto h_my_int = in.get_handle<int>("my_int");
    BOOST_REQUIRE_EQUAL(in.entries(), size_t(100));
    for(int i=0; i<100; ++i){
        in.read_entry(i);
        int idata = inevent.get(h_my_int);
        BOOST_CHECK_EQUAL(idata, i);
    }
}


BOOST_AUTO_TEST_CASE(outhist){
    unique_ptr<TFile> outfile(new TFile("outhist.root", "recreate"));
    Event event;
    TFileOutputManager fout(move(outfile), "eventtree", event);
    
    TH1D * histo = new TH1D("h1", "h1", 100, 0, 1);
    fout.put("histname", histo);
    histo->Fill(0.5);
    
    fout.close();
    
    // check that h1 was written as "outhist":
    TFile f("outhist.root", "read");
    TH1D * inhist = dynamic_cast<TH1D*>(f.Get("histname"));
    BOOST_REQUIRE(inhist);
    BOOST_CHECK_EQUAL(inhist->GetEntries(), 1);
}

BOOST_AUTO_TEST_CASE(outhist_dir){
    unique_ptr<TFile> outfile(new TFile("outhist_dir.root", "recreate"));
    Event event;
    TFileOutputManager fout(move(outfile), "eventtree", event);
    
    TH1D * histo = new TH1D("h1", "h1", 100, 0, 1);
    fout.put("dir1/dir2/dir3/histname", histo);
    histo->Fill(0.5);
    
    fout.close();
    
    // check that h1 was written as "outhist":
    TFile f("outhist_dir.root", "read");
    TH1D * inhist = dynamic_cast<TH1D*>(f.Get("dir1/dir2/dir3/histname"));
    BOOST_REQUIRE(inhist);
    BOOST_CHECK_EQUAL(inhist->GetEntries(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

