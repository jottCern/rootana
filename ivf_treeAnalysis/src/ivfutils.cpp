#include "ra/include/identifier.hpp"
#include "ivfutils.hpp"
#include <set>
#include <iostream>
#include <vector>


using namespace std;
using namespace ra;
using namespace ivf;

namespace{
    
const char* categories[] ={"Fake", "Hadronic", "Unknown", "Undefined", "GeantPrimary", "GeantDecay", "Compton", "Annihilation", "EIoni", "HIoni", "MuIoni", "Photon", "MuPairProd", "Conversions", "EBrem", "SynchrotronRadiation", "MuBrem", "MuNucl", "PrimaryDecay", "Proton", "BWeak", "CWeak", "FromBWeakDecayMuon", "FromCWeakDecayMuon", "ChargePion", "ChargeKaon", "Tau", "Ks", "Lambda", "Jpsi", "Xi", "SigmaPlus", "SigmaMinus"};
    
struct cat_id {

    vector<identifier> id;
    unsigned int size;

    cat_id() {
        for (int i = 0; i < CAT_ARRAY_SIZE; ++i) {
            identifier id_(categories[i]);
            id.push_back(id_);
        }
            
            size = id.size();
    }
} cat_ids;
    
struct new_cat_id {

    vector<identifier> id;
    unsigned int size;

    new_cat_id() {
        for (int i = GEANT_ARRAY_SIZE; i < CAT_ARRAY_SIZE; ++i) {
            if (i != 19){
                identifier id_(categories[i]);
                id.push_back(id_);
            }
        }
        size = id.size();
    }
    
} new_cat_ids;

}

const char* ivf::cattostr(ivf::Category cat) {
    if (cat >= CAT_ARRAY_SIZE) {
        std::cout << "WARNING: unknown category, return fake!" << std::endl;
        return "Fake";
    }
    return categories[cat];
}

const char* ivf::cattostr(int cat) {
    if (cat >= CAT_ARRAY_SIZE){
        std::cout << "WARNING: unknown category, return fake!" << std::endl;
        return "Fake";
    }
    return categories[cat];
}

ivf::Category ivf::strtocat(std::string str) {
    identifier str_id(str);
    for (unsigned int i = 0; i < cat_ids.size; ++i){
        if (str_id == cat_ids.id[i])
            return static_cast<Category>(i);
    }
    std::cout << "WARNING: " << str << " is no valid category, return Fake!" << std::endl;
    return Fake;
}

newcat::DecayCategory newcat::strtocat(std::string str) {
    
    identifier str_id(str);
    
    for (unsigned int i = 0; i < new_cat_ids.size; ++i){
        if (str_id == new_cat_ids.id[i])
            return static_cast<DecayCategory>(i);
    }
    
    std::cout << "WARNING: " << str << " is no valid category, return Fake!" << std::endl;
    return newcat::Fake;
}


