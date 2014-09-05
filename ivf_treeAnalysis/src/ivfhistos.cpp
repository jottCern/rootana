#include "ra/include/analysis.hpp"
#include "ra/include/event.hpp"
#include "ra/include/config.hpp"
#include "ra/include/context.hpp"
#include "ra/include/hists.hpp"
#include <boost/algorithm/string.hpp>

// #include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include "TH1D.h"
#include "TH2D.h"
#include "ivftree.hpp"
#include "ivfutils.hpp"


using namespace ra;
using namespace std;
using namespace ivf;




class IvfPosHists: public Hists{
public:
    IvfPosHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, InputManager & in, OutputManager & out): Hists(dirname, dataset, out){
        book<TH2D>("ZRho", 560, -70, 70, 300, 0, 30);
        book<TH1D>("Rho1D", 300, 0, 30);
        book<TH1D>("VertMass", 300, 0, 30);
        book<TH1D>("TrackMult", 20, 0, 20);
        book<TH1D>("VertPt", 200, 0, 100);
        book<TH1D>("FlightDirSig3d", 300, 0, 30);
        
        h_recoPos = in.get_handle<Point>("recoPos");
        h_p4daughters = in.get_handle<LorentzVector>("p4daughters");
        h_numberTracks = in.get_handle<int>("numberTracks");
        h_impactSig3D = in.get_handle<float>("impactSig3D");
    }
    
    virtual void fill(const identifier & id, double x){
        get(id)->Fill(x);
    }
    
    virtual void fill(const identifier & id, double x, double y){
        get(id)->Fill(x, y);
    }
    
    virtual void process(Event & e){
        const Point & position = e.get<Point>(h_recoPos);
        const LorentzVector & lorvec = e.get<LorentzVector>(h_p4daughters);
        int trackmult = e.get<int>(h_numberTracks);
        float flightdirSig3d = e.get<float>(h_impactSig3D);
        
        ID(ZRho); ID(Rho1D); ID(VertMass);
        ID(TrackMult); ID(VertPt); ID(FlightDirSig3d);
        
        fill(ZRho, position.z(), position.rho());
        fill(Rho1D, position.rho());
        fill(VertMass, lorvec.M());
        fill(TrackMult, trackmult);
        fill(VertPt, lorvec.pt());
        fill(FlightDirSig3d, flightdirSig3d);
    }
    
private:
    Event::Handle<Point> h_recoPos;
    Event::Handle<LorentzVector> h_p4daughters;
    Event::Handle<int> h_numberTracks;
    Event::Handle<float> h_impactSig3D;
};

REGISTER_HISTS(IvfPosHists)




class IvfCatHists: public Hists{
public:
    IvfCatHists(const ptree & cfg, const std::string & dirname, const s_dataset & dataset, InputManager & in, OutputManager & out): Hists(dirname, dataset, out){
        discriminator_ = ptree_get<float>(cfg, "discriminator", 0.);
        std::string catstr = ptree_get<std::string>(cfg, "categories", "AllGeant");
        debug = ptree_get<bool>(cfg, "debug", false);
        cat_name = dirname;
        stringstream disc_str;
        disc_str << discriminator_;
        string histname = disc_str.str();
        
        ID(AllGeant);
        ID(AllDecay);
        identifier catid(catstr);

        if (AllGeant == catid){
            for (int i = 1; i < GEANT_ARRAY_SIZE; ++i)
                catstrings_.push_back(cattostr(i));
        }
        else if (AllDecay == catid) {
            for (int i = GEANT_ARRAY_SIZE; i < CAT_ARRAY_SIZE; ++i)
                catstrings_.push_back(cattostr(i));
        }
        else {
            boost::split(catstrings_, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
        }

        processes_histname = "Processes"+histname;
        book<TH2D>("Processes"+histname, catstrings_.size(), 0, catstrings_.size(), 100, 0., 1.);
        for (size_t i = 1; i <= catstrings_.size(); ++i){
            get(processes_histname)->GetXaxis()->SetBinLabel(i, catstrings_[i-1].c_str());
        }
        
        h_thisProcessFlags = in.get_handle<Flags>("thisProcessFlags");
    }
    
    virtual void fill(const identifier & id, double x, double y){
        get(id)->Fill(x, y);
    }
    
    virtual void process(Event & e){
        Flags & flags = e.get<Flags>(h_thisProcessFlags);
        if (debug) cout << "=== " << cat_name << " histogram filled!" << endl;
        for (size_t i = 0; i < catstrings_.size(); ++i){
            //if (flags[strtocat(catstrings_[i])] > discriminator_)
                fill(processes_histname, i, flags[strtocat(catstrings_[i])]);
        }
    }
    
private:
    
    std::vector<string> catstrings_;
    float discriminator_;
    bool debug;
    std::string cat_name;
    
    Event::Handle<Flags> h_thisProcessFlags;
    ra::identifier processes_histname;
};

REGISTER_HISTS(IvfCatHists)
