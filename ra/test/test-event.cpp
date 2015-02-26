#include <boost/test/unit_test.hpp>

#include "event.hpp"
#include "context-backend.hpp"
#include "config.hpp"
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
    EventStructure es;
    auto h = es.get_handle<int>("test");
    
    Event e(es);
    BOOST_CHECK(e.get_state(h) == Event::state::nonexistent);
    BOOST_CHECK_THROW(e.get(h), std::runtime_error);
    BOOST_CHECK_EQUAL(e.size(), 1);
    
    e.set(h, 5);
    BOOST_CHECK_EQUAL(e.get(h), 5);
    
    e.get(h) = 10;
    BOOST_CHECK_EQUAL(e.get(h), 10);
    
    int * ptr = &e.get(h);
    
    e.set_validity(h, false);
    BOOST_CHECK_THROW(e.get(h), std::runtime_error);
    BOOST_CHECK_EQUAL(&e.get(h, false), ptr);
}

BOOST_AUTO_TEST_CASE(raw_set){
    
    bool destructor_called = false;
    
    {
        MutableEvent event;
        Event::RawHandle hraw = event.get_raw_handle(typeid(int), "testint");
        Event::Handle<int> h = event.get_handle<int>("testint");
        BOOST_CHECK_EQUAL(event.size(), 1);
        int i = 5;
        event.set(typeid(int), hraw, &i, [&destructor_called](void*){ destructor_called = true; });
        int value = event.get(h);
        BOOST_CHECK_EQUAL(value, 5);
        i = 42;
        value = event.get(h);
        BOOST_CHECK_EQUAL(value, 42);
    }
    BOOST_CHECK(destructor_called);
}


BOOST_AUTO_TEST_CASE(raw_set2){
    MutableEvent event;
    Event::RawHandle hraw = event.get_raw_handle(typeid(int), "testint");
    int i = 5;
    event.set(typeid(int), hraw, &i, [](void*){});
    // setting twice should not succeed, as this would undermine the guarantee
    // that (once set to non-null value) the address of a data member cannot change.
    BOOST_CHECK_THROW(event.set(typeid(int), hraw, &i, [](void*){}), std::exception);
}

// test that addresses of the data in Event do not change across calls to 'set'.
BOOST_AUTO_TEST_CASE(addresses){
    EventStructure es;
    auto h = es.get_handle<int>("test");
    
    Event e(es);
    e.set(h, 5);
    
    int & idata = e.get(h);
    BOOST_CHECK_EQUAL(idata, 5);
    
    e.set(h, 8);
    BOOST_CHECK_EQUAL(idata, 8);
    BOOST_CHECK_EQUAL(&(e.get(h)), &idata);
}

// check that getting handles with same name/type is the same:
BOOST_AUTO_TEST_CASE(handles){
    EventStructure es;
    auto h0 = es.get_handle<int>("test");
    
    Event e(es);
    
    e.set(h0, 5);
    BOOST_CHECK_EQUAL(es.name(h0), "test");
    
    auto h1 = es.get_handle<int>("test");
    BOOST_CHECK_EQUAL(e.get(h1), 5);
    
    BOOST_CHECK_EQUAL(es.name(h1), "test");
    
    // same name, other type:
    auto h2 = es.get_handle<float>("test");
    BOOST_CHECK_THROW(e.get_state(h2), std::runtime_error);
    BOOST_CHECK_EQUAL(es.name(h2), "test");
}

BOOST_AUTO_TEST_CASE(unset){
    EventStructure es;
    auto h_itest = es.get_handle<int>("itest");
    
    Event e(es);
    
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
    EventStructure es;
    auto h = es.get_handle<int>("itest");
    
    Event e(es);
    BOOST_CHECK_EQUAL(e.get_state(h), Event::state::nonexistent);
    int n_callback_called = 0;
    e.set(h, -1);
    int & iref = e.get(h);
    auto callback = [&](){++n_callback_called; e.set(h, 5); };
    std::function<void ()> cb(callback);
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
    std::function<void (void)> null_function;
    e.set_get_callback(h, null_function);
    iref = 0;
    BOOST_CHECK_THROW(iresult = e.get(h), std::runtime_error);
}



BOOST_AUTO_TEST_CASE(read){
    // check that reading intdata works:
    EventStructure es;
    auto in = InputManagerBackendRegistry::build("root", es, ptree());
    in->declare_event_input<int>("intdata");
    Event event(es);
    size_t nentries = in->setup_input_file(event, "test", "tree.root");
    auto h_intdata = es.get_handle<int>("intdata");
    BOOST_REQUIRE_EQUAL(nentries, size_t(100));
    for(int i=0; i<100; ++i){
        in->read_event(event, i);
        int idata = event.get(h_intdata);
        BOOST_CHECK_EQUAL(idata, i+1);
    }
    //BOOST_CHECK_THROW(event.get<double>("doubledata"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(read_lazy){
    EventStructure es;
    ptree cfg;
    cfg.add_child("lazy", ptree("true"));
    auto in = InputManagerBackendRegistry::build("root", es, cfg);
    in->declare_event_input<int>("intdata");
    Event event(es);
    auto h_intdata = in->get_handle<int>("intdata");
    size_t nentries = in->setup_input_file(event, "test", "tree.root");
    BOOST_REQUIRE_EQUAL(nentries, size_t(100));
    for(int i=0; i<100; ++i){
        in->read_event(event, i);
        int idata = event.get(h_intdata);
        BOOST_CHECK_EQUAL(idata, i+1);
    }
    // we should have read 100 ints now:
    BOOST_CHECK_EQUAL(in->nbytes_read(), size_t(100) * sizeof(int));
}

// use inputmanager to read data without using an Event
/*
BOOST_AUTO_TEST_CASE(read_noevent){
    Event event;
    ptree cfg;
    auto in = InputManagerBackendRegistry::build("root", event, cfg);
    int int_in;
    in->declare_input("intdata", "", &int_in, typeid(int));
    size_t nentries = in->setup_input_file("test", "tree.root");
    BOOST_REQUIRE_EQUAL(nentries, size_t(100));
    for(int i=0; i<100; ++i){
        in->read_event(i);
        BOOST_CHECK_EQUAL(int_in, i+1);
    }
}*/

BOOST_AUTO_TEST_CASE(read_wrong_type){
    // it should not work to read intdata as double:
    EventStructure es;
    ptree cfg;
    auto in = InputManagerBackendRegistry::build("root", es, cfg);
    in->declare_event_input<double>("intdata");
    Event event(es);
    BOOST_CHECK_THROW(in->setup_input_file(event, "test", "tree.root"), std::runtime_error);
}



BOOST_AUTO_TEST_CASE(outtree){
    { // create output file:
    EventStructure es;
    auto out = OutputManagerBackendRegistry::build("root", es, "eventtree", "out");
    auto h_my_int = out->declare_event_output<int>("my_int");
    Event event(es);
    for(int i=0; i<100; ++i){
        event.set(h_my_int, i);
        out->write_event(event);
    }
    }
    
    // check that the output file is Ok:
    EventStructure es;
    ptree cfg;
    auto in = InputManagerBackendRegistry::build("root", es, cfg);
    auto h_my_int = in->declare_event_input<int>("my_int");
    Event inevent(es);
    size_t nevents = in->setup_input_file(inevent, "eventtree", "out.root");
    BOOST_REQUIRE_EQUAL(nevents, size_t(100));
    for(size_t i=0; i<nevents; ++i){
        in->read_event(inevent, i);
        int idata = inevent.get(h_my_int);
        BOOST_CHECK_EQUAL(idata, i);
    }
}


// output tree in a directory within the output file:
BOOST_AUTO_TEST_CASE(outtree_dir){
    {
    EventStructure es;
    
    auto out = OutputManagerBackendRegistry::build("root", es, "dir/eventtree", "out");
    auto h_my_int = out->declare_event_output<int>("my_int");
    Event event(es);
    for(int i=0; i<100; ++i){
        event.set(h_my_int, i);
        out->write_event(event);
    }
    }
    
    // check that the output file is Ok:
    EventStructure es;
    
    ptree cfg;
    auto in = InputManagerBackendRegistry::build("root", es, cfg);
    auto h_my_int = in->declare_event_input<int>("my_int");
    Event inevent(es);
    size_t nevents = in->setup_input_file(inevent, "dir/eventtree", "out.root");
    BOOST_REQUIRE_EQUAL(nevents, size_t(100));
    for(int i=0; i<100; ++i){
        in->read_event(inevent, i);
        int idata = inevent.get(h_my_int);
        BOOST_CHECK_EQUAL(idata, i);
    }
}


BOOST_AUTO_TEST_CASE(outhist){
    EventStructure es;
    auto out = OutputManagerBackendRegistry::build("root", es, "eventtree", "outhist");
    
    TH1D * histo = new TH1D("h1", "h1", 100, 0, 1);
    out->put("histname", histo);
    histo->Fill(0.5);
    
    out->close();
    
    // check that h1 was written as "outhist":
    TFile f("outhist.root", "read");
    TH1D * inhist = dynamic_cast<TH1D*>(f.Get("histname"));
    BOOST_REQUIRE(inhist);
    BOOST_CHECK_EQUAL(inhist->GetEntries(), 1);
}

BOOST_AUTO_TEST_CASE(outhist_dir){
    EventStructure es;
    auto out = OutputManagerBackendRegistry::build("root", es, "eventtree", "outhist_dir");
    
    TH1D * histo = new TH1D("h1", "h1", 100, 0, 1);
    out->put("dir1/dir2/dir3/histname", histo);
    histo->Fill(0.5);
    
    out->close();
    
    // check that h1 was written as "outhist":
    TFile f("outhist_dir.root", "read");
    TH1D * inhist = dynamic_cast<TH1D*>(f.Get("dir1/dir2/dir3/histname"));
    BOOST_REQUIRE(inhist);
    BOOST_CHECK_EQUAL(inhist->GetEntries(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

