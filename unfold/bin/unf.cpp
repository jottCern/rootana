#include <vector>
#include <iostream>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <set>

#include "unfold.hpp"
#include "root.hpp"
#include "base/include/ptree-utils.hpp"

#include "TH2D.h"
#include "TH1D.h"
#include "TDirectory.h"
#include "TFile.h"

#include <boost/property_tree/info_parser.hpp>

using namespace std;

std::unique_ptr<TFile> new_tfile(const string & fname, const string & options){
    std::unique_ptr<TFile> result(TFile::Open(fname.c_str(), options.c_str()));
    if(!result || !result->IsOpen()){
        throw runtime_error("Could not open root file '" + fname + "'");
    }
    return result;
}


int main(int argc, char ** argv){
    if(argc!=2){
        cerr << "Usage: " << argv[0] << " <cfg file>" << endl;
        return 1;
    }
    
    TH1::AddDirectory(false);
    cout << "[main] Building configuration from setup ..." << endl;
    
    ptree cfg;
    boost::property_tree::read_info(argv[1], cfg);
    
    auto options_cfg = cfg.get_child("options");
    
    // read in histograms describing the unfolding problem:
    auto infile = new_tfile(ptree_get<string>(options_cfg, "infile"), "read");
    
    // build problem:
    ptree problem_cfg = cfg.get_child("problem");
    Problem problem(*infile, problem_cfg);

    // build unfolder:
    ptree unfolder_cfg = cfg.get_child("unfolder");
    auto unfolder = UnfolderRegistry::build(ptree_get<string>(unfolder_cfg, "type"), problem, unfolder_cfg);
    
    // build modules to run:
    std::vector<std::unique_ptr<Module> > modules;
    auto outfile = new_tfile(ptree_get<string>(options_cfg, "outfile"), "recreate");
    auto modules_cfg = cfg.get_child("modules");
    for(auto & mcfg : modules_cfg){
        TDirectory * outdir = outfile->mkdir(mcfg.first.c_str());
        modules.emplace_back(ModuleRegistry::build(ptree_get<string>(mcfg.second, "type"), mcfg.second, *infile, *outdir));
    }
        
    cout << "[main] Running regularization scan ..." << endl;
    TDirectory * dir = outfile->mkdir("regularization");
    dir->cd();
    unfolder->do_regularization_scan(*dir);
    size_t i=0;
    for(auto & m : modules){
        cout << "[main] Running " << m->out().GetName() << " ..." << endl;
        m->out().cd();
        m->run(problem, *unfolder);
        ++i;
    }
}
