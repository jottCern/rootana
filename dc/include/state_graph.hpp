#ifndef DC_STATE_GRAPH_HPP
#define DC_STATE_GRAPH_HPP

#include <set>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
#include <typeinfo>

#include "buffer.hpp"

namespace dc {

template<typename TAG>
struct Id{
   bool operator<(const Id & rhs) const{
      return id_ < rhs.id_;
   }
   
   bool operator==(const Id & rhs) const{
      return id_ == rhs.id_;
   }
   
   bool operator!=(const Id & rhs) const{
      return id_ != rhs.id_;
   }
   
   size_t id() const{
       return id_;
   }
   
   Id(): id_(static_cast<size_t>(-1)){}
   explicit Id(size_t id): id_(id){}

private:
   size_t id_;
};


// create unique ids (unique within one IdManager instance), and give them names.
// The type T is used for ids; it should be copyable, constructible via T(size_t), support
// to be used as key in a map (in particular support T1 < T2 comparison).
// T can be something like int or void*, but it's advisable to use Id<your_own_tag> for compile-time type-checking.
template<typename T>
class IdManager{
public:
    IdManager(): next_id(0){}
    
    T create(const std::string & name){
        if(exists(name)) throw std::runtime_error("id '" + name + "' already exists!");
        T result(next_id++);
        id_to_name.insert(std::make_pair(result, name));
        name_to_id.insert(std::make_pair(name, result));
        return result;
    }
    
    T create_anonymous(){
        T result(next_id++);
        anonymous_ids.insert(result);
        return result;
    }

    T get(const std::string & name) const{
        auto it = name_to_id.find(name);
        if(it == name_to_id.end()) throw std::runtime_error("unknown id name '" + name + "'");
        return it->second;
    }

    std::set<T> get_all() const{
        std::set<T> res(anonymous_ids);
        for(auto & it : id_to_name){
            res.insert(it.first);
        }
        return res;
    }

    bool exists(const std::string & name) const{
        return name_to_id.find(name) != name_to_id.end();
    }
    
    bool exists(const T & id) const{
        return id_to_name.find(id) != id_to_name.end();
    }

    std::string name(const T & id) const{
        auto it = id_to_name.find(id);
        if(it == id_to_name.end()) throw std::runtime_error("unknown id");
        return it->second;
    }


private:
    size_t next_id;
    std::set<T> anonymous_ids;
    std::map<std::string, T> name_to_id;
    std::map<T, std::string> id_to_name;
};



/** \brief Graph description of an algorithm, suitable for distributed computing
 * 
 * An algorithm is decribed by the possible worker states and state transitions to allow
 * for some automated handling of some aspects of the distribution.
 * 
 * As the StateGraph for a given algorithm stays the same over the lifetime of the program, it would
 * be possible in principle to define a StateGraph not through an object but via TMP techniques. However,
 * this is hard in practice, so the object version is chosen here.
 * 
 * Formally the stategraph is a set of states (the nodes in the graph) and state transitions (directed edges). Each
 * state transition is initiated by a Message of a certain type; this information is also part of the StateGraph.
 * For a given state, the next type can be uniquely determined by the Message type. Also, an edge is uniquely determined
 * by the initial and final node (without the message type).
 * 
 * There are always the three pre-defined states "start", "stop", and "fail" which represent, respectively, the initial state, the
 * (successful) final state and a universal error state. State transitions to "failed" are always implicitly possible (although
 * an explicit state transition definition would be legal).
 * (NOTE: failure could even be "taken out" of the definition of the state graph and handled externally, but so far
 * "failed" is just another state of a worker, which makes some code somewhat easier).
 * 
 * Under certain temporary runtime conditions, certain state transitions are not allowed. For example,
 * if there is no more data to process, the state transition(s) corresponding to processing data are unavailable.
 * To express that in the StateGraph formalism, all transitions are made part of a "restriction set" where a restriction set
 * is the (named) set of forbidden transitions. Restriction sets are defined at the level of the StateGraph; the actual meaning
 * of forbidden transitions and the currently active are dynamic information and therefore not part of the StateGraph which
 * only encodes static information.
 */
class StateGraph {
public:
    
    struct StateIdTag{};
    struct RestrictionSetIdTag{};
    struct PrioritySetIdTag{};
    
    typedef Id<StateIdTag> StateId;
    typedef Id<RestrictionSetIdTag> RestrictionSetId;
    typedef Id<PrioritySetIdTag> PrioritySetId;
    
    StateGraph(const StateGraph &) = delete;
    
    
    StateGraph() {
        add_state("start");
        add_state("stop");
        add_state("failed");
    }
    
    // states:
    StateId add_state(const std::string & name){
        return StateIds.create(name);
    }
    
    StateId get_state(const std::string & name) const{
        return StateIds.get(name);
    }
    
    std::string name(const StateId & s) const{
        return StateIds.name(s);
    }
    
    std::set<StateId> all_states() const{
        return StateIds.get_all();
    }
    
    //transitions:
    void add_state_transition(const StateId & from, const StateId & to, const std::type_info & mt){
        check_state_id(from);
        check_state_id(to);
        auto & edges = transitions[from];
        //check that neither StateId nor typeid is found in current edges:
        for(const auto & edge : edges){
            if(edge.to==to or edge.message_type==mt){
                throw std::invalid_argument("conflicting state transition exists");
            }
        }
        edges.emplace_back(to, mt);
    }
    
    template<typename MT>
    void add_state_transition(const StateId & from, const StateId & to){
        add_state_transition(from, to, typeid(MT));
    }
    
    std::set<StateId> next_states(const StateId & from) const{
        check_state_id(from);
        std::set<StateId> result;
        auto it = transitions.find(from);
        if(it!=transitions.end()){
            for(const Edge & e : it->second){
                result.insert(e.to);
            }
        }
        return result;
    }
    
    const std::type_info & transition_message_type(const StateId & from, const StateId & to) const{
        check_state_id(from);
        check_state_id(to);
        auto it = transitions.find(from);
        if(it!=transitions.end()){
            for(const Edge & e : it->second){
                if(e.to==to){
                    return e.message_type;
                }
            }
        }
        throw std::invalid_argument("transition " + name(from) + "->" + name(to) + " does not exist");
    }
    
    StateId next_state(const StateId & from, const std::type_info & mt) const{
        check_state_id(from);
        auto it = transitions.find(from);
        if(it!=transitions.end()){
            for(const Edge & e : it->second){
                if(e.message_type==mt){
                    return e.to;
                }
            }
        }
        throw std::invalid_argument("transition from " + name(from) + " with type " + mt.name() + " does not exist");
    }
    
    template<typename MT>
    StateId next_state(const StateId & from) const{
        return next_state(from, typeid(MT));
    }
    
    // restrictions:
    RestrictionSetId add_restriction_set(const std::string & name){
        return RestrictionSetIds.create(name);
    }
    
    RestrictionSetId get_restriction_set(const std::string & name) const {
        return RestrictionSetIds.get(name);
    }
    
    std::string name(const RestrictionSetId & rset) const {
        return RestrictionSetIds.name(rset);
    }
    
    // note: transition does not need to exist to be forbidden (although it makes not much sense ...)
    void add_restriction(const RestrictionSetId & rset, const StateId & from, const StateId & to){
        check_restriction_set_id(rset);
        check_state_id(from);
        check_state_id(to);
        restriction_set_restrictions[rset].insert(std::make_pair(from, to));
    }
    
    bool is_restricted(const RestrictionSetId & rset, const StateId & from, const StateId & to) const{
        check_restriction_set_id(rset);
        check_state_id(from);
        check_state_id(to);
        auto it = restriction_set_restrictions.find(rset);
        if(it==restriction_set_restrictions.end()) return false;
        return it->second.find(std::make_pair(from, to)) != it->second.end();
    }
    
    std::set<std::pair<StateId, StateId> > restrictions(const RestrictionSetId & rset) const{
        check_restriction_set_id(rset);
        auto it = restriction_set_restrictions.find(rset);
        if(it==restriction_set_restrictions.end()) return std::set<std::pair<StateId, StateId> >(); // the empty set
        return it->second;
    }
    
private:
    
    void check_state_id(const StateId & s) const{
        if(!StateIds.exists(s)){
            throw std::invalid_argument("invalid stateid");
        }
    }
    
    void check_restriction_set_id(const RestrictionSetId & rset) const{
        if(!RestrictionSetIds.exists(rset)){
            throw std::invalid_argument("invalid restriction set id");
        }
    }
    
    //states:
    IdManager<StateId> StateIds;
    
    //transitions, ordered by starting state (so no 'from' node here):
    struct Edge {
        StateId to;
        const std::type_info & message_type;
        
        Edge(const Edge&) = delete;
        Edge(Edge &&) = default;
        Edge & operator=(Edge &&) = default;
        Edge(const StateId & to_, const std::type_info & mt): to(to_), message_type(mt){}
    };
    
    std::map<StateId, std::vector<Edge> > transitions;
    
    // restrictions:
    IdManager<RestrictionSetId> RestrictionSetIds;
    std::map<RestrictionSetId, std::set<std::pair<StateId, StateId> > > restriction_set_restrictions;
};

}


#endif
