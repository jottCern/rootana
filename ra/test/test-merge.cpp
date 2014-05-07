#include <boost/test/unit_test.hpp>
#include "root-utils.hpp"

#include "TTree.h"
#include "TFile.h"
#include "TH1D.h"

using namespace std;
using namespace ra;

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

// creates a test file with histograms as given in hnames (may contain directories). They
// always have 100 bins from 0 to 1 and are all filled with d0 + m*i/n
void create_test_hfile(const string & filename, const std::vector<std::string> & hnames, double d0, double m){
    TFile f(filename.c_str(), "recreate");
    int i = 0;
    for(auto & hname: hnames){
        TH1D * histo = new TH1D(hname.c_str(), hname.c_str(), 100, 0.0, 1.0);
        histo->SetDirectory(&f);
        histo->Fill(d0 + m * i/hnames.size());
        i++;
    }
    f.cd();
    f.Write();
    f.Close();
}


}

BOOST_AUTO_TEST_SUITE(merge)

BOOST_AUTO_TEST_CASE(tree){
    create_test_tree("test0.root", 23, 1000);
    create_test_tree("test1.root", 5728, 1000);
    
    merge_rootfiles("test0.root", "test1.root");
    
    // check output:
    vector<int> merged_data = get_tree_intdata("test0.root", "intdata");
    BOOST_REQUIRE_EQUAL(merged_data.size(), 2000);
    for(int i=0; i<1000; ++i){
        BOOST_REQUIRE_EQUAL(merged_data[i], 23 + i);
        BOOST_REQUIRE_EQUAL(merged_data[i + 1000], 5728 + i);
    }
}


BOOST_AUTO_TEST_CASE(large_tree){
    const int nevents = 1000000;
    create_test_tree("test0.root", 23, nevents);
    create_test_tree("test1.root", 5728, nevents);
    
    merge_rootfiles("test0.root", "test1.root");
    
    // check output:
    vector<int> merged_data = get_tree_intdata("test0.root", "intdata");
    BOOST_REQUIRE_EQUAL(merged_data.size(), 2 * nevents);
    for(int i=0; i<nevents; ++i){
        BOOST_REQUIRE_EQUAL(merged_data[i], 23 + i);
        BOOST_REQUIRE_EQUAL(merged_data[i + nevents], 5728 + i);
    }
}

BOOST_AUTO_TEST_CASE(many_trees){
    const int nevents = 100000; // per file
    const int nfiles = 20;
    const int offset0 = 57;
    for(int ifile = 0; ifile < nfiles; ++ifile){
        stringstream ss;
        ss << "test" << ifile << ".root";
        create_test_tree(ss.str(), offset0 + ifile * nevents, nevents);
    }
    
    // merge all into first:
    for(int ifile = 1; ifile < nfiles; ++ifile){
        stringstream ss;
        ss << "test" << ifile << ".root";
        merge_rootfiles("test0.root", ss.str());
    }
    
    // check output:
    vector<int> merged_data = get_tree_intdata("test0.root", "intdata");
    BOOST_REQUIRE_EQUAL(merged_data.size(), nfiles * nevents);
    for(int i=0; i<nevents * nfiles; ++i){
        BOOST_REQUIRE_EQUAL(merged_data[i], offset0 + i);
    }
}

BOOST_AUTO_TEST_CASE(histos){
    create_test_hfile("test0.root", {"h1", "h2"}, 0.0, 0.3);
    create_test_hfile("test1.root", {"h1", "h2"}, 0.5, 0.3);
    merge_rootfiles("test0.root", "test1.root");
    
    TFile f("test0.root", "read");
    TH1D * h1 = dynamic_cast<TH1D*>(f.Get("h1"));
    BOOST_REQUIRE(h1);
    BOOST_CHECK_EQUAL(h1->GetEntries(), 2);
    BOOST_CHECK_EQUAL(h1->GetBinContent(h1->FindBin(0.0)), 1);
    BOOST_CHECK_EQUAL(h1->GetBinContent(h1->FindBin(0.5)), 1);
    
    TH1D * h2 = dynamic_cast<TH1D*>(f.Get("h2"));
    BOOST_REQUIRE(h2);
    BOOST_CHECK_EQUAL(h2->GetEntries(), 2);
    BOOST_CHECK_EQUAL(h2->GetBinContent(h2->FindBin(0.15)), 1);
    BOOST_CHECK_EQUAL(h2->GetBinContent(h2->FindBin(0.65)), 1);
}

BOOST_AUTO_TEST_SUITE_END()

