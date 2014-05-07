#ifndef DC_FWMESSAGES_HPP
#define DC_FWMESSAGES_HPP

#include "message.hpp"

namespace dc{

class WorkerResponse: public Message{
public:
    enum RequestedState {
        rs_work = 0,  // worker wants to work
        rs_stop // worker wants to stop
    };
    
    int requested_state;
    std::unique_ptr<Message> response_details; // can be the null pointer
    
    WorkerResponse(): requested_state(RequestedState::rs_work){}
    virtual void write_data(Buffer & out) const{
        out << requested_state << response_details;
    }
    
    virtual void read_data(Buffer & in){
        in >> requested_state >> response_details;
    }
};

}

#endif
