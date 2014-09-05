#include "ra/include/selections.hpp"
#include "ra/include/context.hpp"
#include "ivftree.hpp"
#include "ivfutils.hpp"

#include <boost/algorithm/string.hpp>

using namespace ra;
using namespace std;
using namespace ivf;

class MinTvQualSelection: public Selection{
public:
    MinTvQualSelection(const ptree & cfg, InputManager & in, OutputManager &){
        disc = ptree_get<float>(cfg, "disc", .0);
        h_tvquality = in.get_handle<float>("tvquality");
    }
    
    bool operator()(const Event & event){
        float qual = event.get<float>(h_tvquality);
        return (qual > disc);
    }
    
private:
    float disc;
    Event::Handle<float> h_tvquality;
};


REGISTER_SELECTION(MinTvQualSelection)



class GeneralDecaySelection: public Selection{
public:
    GeneralDecaySelection(const ptree & cfg, InputManager & in, OutputManager &){
        disc = ptree_get<float>(cfg, "disc", .0);
        h_thisProcessFlags = in.get_handle<Flags>("thisProcessFlags");
    }
    
    bool operator()(const Event & event){
        const Flags & tp_flag = event.get(h_thisProcessFlags);
        float prim_flag = tp_flag[GeantPrimary];
        float dec_flag = tp_flag[GeantDecay];
        return ((prim_flag+dec_flag) > disc);
    }
    
private:
    float disc;
    Event::Handle<Flags> h_thisProcessFlags;
};


REGISTER_SELECTION(GeneralDecaySelection)




class ZPosSelection: public Selection{
public:
    ZPosSelection(const ptree & cfg, InputManager & in, OutputManager &){
        minz = ptree_get<float>(cfg, "minz", -100.);
        maxz = ptree_get<float>(cfg, "maxz", 100.);
        h_recoPos = in.get_handle<Point>("recoPos");
    }
        
    bool operator()(const Event & event){
        const Point & position = event.get(h_recoPos);
        float vertz = position.z();
        return (minz <= vertz && vertz <= maxz );
    }

private:
    float minz;
    float maxz;
    Event::Handle<Point> h_recoPos;
};


REGISTER_SELECTION(ZPosSelection)


class NTrackSelection: public Selection{
public:
    NTrackSelection(const ptree & cfg, InputManager & in, OutputManager &){
        minnt = ptree_get<int>(cfg, "minnt", 0);
        maxnt = ptree_get<int>(cfg, "maxnt", 200);
        h_numberTracks = in.get_handle<int>("numberTracks");
    }
        
    bool operator()(const Event & event){
        int numTracks = event.get(h_numberTracks);
        return (minnt <= numTracks && numTracks <= maxnt );
    }
                
private:
    int minnt;
    int maxnt;
    Event::Handle<int> h_numberTracks;
};


REGISTER_SELECTION(NTrackSelection)


class MassSelection: public Selection{
public:
    MassSelection(const ptree & cfg, InputManager & in, OutputManager &){
        minm = ptree_get<float>(cfg, "minm", 0.);
        maxm = ptree_get<float>(cfg, "maxm", 200.);
        h_p4daughters = in.get_handle<LorentzVector>("p4daughters");
    }
        
    bool operator()(const Event & event){
        const LorentzVector & p4 = event.get<LorentzVector>(h_p4daughters);
        float mass = p4.mass();
        return (minm < mass && mass < maxm );
    }
                
private:
    float minm;
    float maxm;
    Event::Handle<LorentzVector> h_p4daughters;
};


REGISTER_SELECTION(MassSelection)


class PtSelection: public Selection{
public:
    PtSelection(const ptree & cfg, InputManager & in, OutputManager &){
        minpt = ptree_get<float>(cfg, "minpt", 0.);
        maxpt = ptree_get<float>(cfg, "maxpt", 1000.);
        h_p4daughters = in.get_handle<LorentzVector>("p4daughters");
    }
        
    bool operator()(const Event & event){
        const LorentzVector & p4 = event.get<LorentzVector>(h_p4daughters);
        float pt = p4.Pt();
        return (minpt < pt && pt < maxpt );
    }
                
private:
    float minpt;
    float maxpt;
    Event::Handle<LorentzVector> h_p4daughters;
};


REGISTER_SELECTION(PtSelection)


class EtaSelection : public Selection{
public:
    EtaSelection(const ptree & cfg, InputManager & in, OutputManager &){
        mineta = ptree_get<float>(cfg, "mineta", -100.);
        maxeta = ptree_get<float>(cfg, "maxeta", 100.);
        h_recoPos = in.get_handle<Point>("recoPos");
    }
        
    bool operator()(const Event & event){
        const Point & position = event.get<Point>(h_recoPos);
        float eta = position.eta();
        return (mineta < eta && eta < maxeta );
    }
                
private:
    float mineta;
    float maxeta;
    Event::Handle<Point> h_recoPos;
};


REGISTER_SELECTION(EtaSelection)

class Sig3DSelection : public Selection{
public:
    Sig3DSelection(const ptree & cfg, InputManager & in, OutputManager &){
        minsig = ptree_get<float>(cfg, "minsig", 0.);
        maxsig = ptree_get<float>(cfg, "maxsig", 1000.);
        h_impactSig3D = in.get_handle<float>("impactSig3D");
    }
        
    bool operator()(const Event & event){
        float sig3D = event.get<float>(h_impactSig3D);
        return (minsig < sig3D && sig3D < maxsig );
    }
                
private:
    float minsig;
    float maxsig;
    Event::Handle<float> h_impactSig3D;
};


REGISTER_SELECTION(Sig3DSelection)


class Rho1DSelection : public Selection{
public:
    Rho1DSelection(const ptree & cfg, InputManager & in, OutputManager &){
        minrho = ptree_get<float>(cfg, "minrho", 0.);
        maxrho = ptree_get<float>(cfg, "maxrho", 1000.);
        h_recoPos = in.get_handle<Point>("recoPos");
    }
        
    bool operator()(const Event & event){
        const Point & position = event.get<Point>(h_recoPos);
        float rho = position.rho();
        return (minrho < rho && rho < maxrho );
    }
                
private:
    float minrho;
    float maxrho;
    Event::Handle<Point> h_recoPos;
};


REGISTER_SELECTION(Rho1DSelection)


//===========CALCULATE ERRORS FIRST, BUT HAVE ONLY THE ONE FROM SV!!!=============     

// class FLengthSigSelection: public Selection{
// public:
//     FLengthSigSelection(const ptree & cfg, OutputManager &){
//         minnt = ptree_get<float>(cfg, "minnt", 0);
//         maxnt = ptree_get<float>(cfg, "maxnt", 200);
//     }
//         
//     bool operator()(const Event & event){
//         ID(numberTracks);
//         int numTracks = event.get<int>(numberTracks);
//         return (minz <= numTracks && numTracks <= maxz );
//     }
//                 
// private:
//     float minnt;
//     float maxnt;
// };
// 
// 
// REGISTER_SELECTION(FLengthSigSelection)



class ProcessOrSelection: public Selection{
public:
    ProcessOrSelection(const ptree & cfg, InputManager & in, OutputManager &){
        std::string catstr = ptree_get<std::string>(cfg, "categories");
        boost::split(catstrings, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
        for(const string & str : catstrings){
            cats.push_back(strtocat(str));
        }
        discriminator = ptree_get<float>(cfg, "discriminator", .0);
        h_thisProcessFlags = in.get_handle<Flags>("thisProcessFlags");
    }
        
    bool operator()(const Event & event){
        const Flags & flags = event.get<Flags>(h_thisProcessFlags);
        for (Category cat : cats){
            if (flags[cat] > discriminator){
                return true;
            }
        }
        return false;
    }
    
private:
    std::vector<std::string> catstrings;
    std::vector<Category> cats;
    float discriminator;
    Event::Handle<Flags> h_thisProcessFlags;
};


REGISTER_SELECTION(ProcessOrSelection)



class ProcessAndSelection: public Selection{
public:
    ProcessAndSelection(const ptree & cfg, InputManager & in, OutputManager &){
        discriminator = ptree_get<float>(cfg, "discriminator", .0);
        std::string catstr = ptree_get<std::string>(cfg, "categories");
        boost::split(catstrings, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
        for(const string & str : catstrings){
            cats.push_back(strtocat(str));
        }
        h_thisProcessFlags = in.get_handle<Flags>("thisProcessFlags");
    }
        
    bool operator()(const Event & event){
        const Flags & flags = event.get<Flags>(h_thisProcessFlags);
        float sum = .0;
        for (Category cat : cats) {
            //cout << cattostr(cat) << " ";
            sum += flags[cat];
        }
        //cout << endl << "Sum: " << sum << endl << endl;
        return (sum > discriminator);
    }
	        
private:
    std::vector<std::string> catstrings;
    std::vector<Category> cats;
    float discriminator;
    Event::Handle<Flags> h_thisProcessFlags;
};


REGISTER_SELECTION(ProcessAndSelection)




class DecaySelection: public Selection{
public:
    DecaySelection(const ptree & cfg, InputManager & in, OutputManager &){
        catstr = ptree_get<std::string>(cfg, "categories");
        boost::split(catstrings, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
        for(const string & str : catstrings){
            cats.push_back(newcat::strtocat(str));
        }
        h_decayflags = in.get_handle<Flags>("decayflags");
    }
        
    bool operator()(const Event & event){
        const Flags & decflags = event.get<Flags>(h_decayflags);
        double sumcats = .0;
        for (newcat::DecayCategory cat : cats)
            sumcats += decflags[cat];
    
        //cout << "Sumcats: " << sumcats << endl;
    
        if (sumcats == 0) return false;
        bool result = true;
        int equalresult = 0;
        
        for (double flagval : decflags){
            if (flagval > sumcats){
            //cout << catstr << ": X" << endl;
            result = false;
            } else if (flagval == sumcats) equalresult++;

        }

        // cout << catstr << ": O" << endl;
        return (result && equalresult < 2);
    }

private:
    std::string catstr;
    std::vector<std::string> catstrings;
    std::vector<newcat::DecayCategory> cats;
    Event::Handle<Flags> h_decayflags;
};


REGISTER_SELECTION(DecaySelection)





class DecayNotSelection: public Selection{
public:
    DecayNotSelection(const ptree & cfg, InputManager & in, OutputManager &){
        std::string catstr = ptree_get<std::string>(cfg, "categories");
        boost::split(catstrings, catstr, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
        for(const string & str : catstrings){
            cats.push_back(strtocat(str));
        }
        h_thisProcessFlags = in.get_handle<Flags>("thisProcessFlags");
    }
        
    bool operator()(const Event & event){
        Flags flags = event.get<Flags>(h_thisProcessFlags);
        
        for (Category cat : cats) {
            if (flags[cat] != 0)
            return false;
        }
        
        return true;
    }
	        
private:
    std::vector<std::string> catstrings;
    std::vector<Category> cats;
    Event::Handle<Flags> h_thisProcessFlags;
};


REGISTER_SELECTION(DecayNotSelection)




class NoDoubleSelection: public Selection{
public:
    NoDoubleSelection(const ptree & cfg, InputManager & in, OutputManager &){
        numbersels = cfg.get<int>("numbersels", 2);
        std::string ids = cfg.get<string>("selections");
        vector<string> vids;
        boost::split(vids, ids, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
        for(const string & id : vids){
            selids.emplace_back(in.get_handle<bool>(id));
        }
    }
        
    bool operator()(const Event & event){
        
        int truesels = 0;
        
        for(const auto & id : selids){
            if(event.get<bool>(id)){
                truesels++;
            }
        }
        return (truesels < numbersels);
    }
	        
private:
    std::vector<Event::Handle<bool>> selids;
    int numbersels;
};


REGISTER_SELECTION(NoDoubleSelection)

