#include "netutils.hpp"
#include "base/include/log.hpp"

#include "iomanager.hpp"
#include "channel.hpp"

#include <sstream>
#include <vector>
#include <unistd.h>
#include <string.h>
#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace dc;
using namespace std;
using namespace std::placeholders;


namespace{
    
// addrinfo, c++ style
struct addrinfox{
    int  ai_flags;
    int  ai_family;
    int  ai_socktype;
    int  ai_protocol;
    string ai_addr;
};

// get the addrinfo fields for hostname:port with the given address family.
//
// For server socket, one can use hostname="*" for listening sockets to listen on all devices.
// Using hostname="" for client sockets means connecting to localhost.
std::vector<addrinfox> getaddrinfox(const string & hostname, int port, int ai_family){
    auto logger = Logger::get("dc.netutils");
    addrinfo hints;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_protocol = 0;
    
    const char * node = 0;
    if(hostname == "*"){
        hints.ai_flags |= AI_PASSIVE;
    }
    else{
        if(!hostname.empty()){
            node = hostname.c_str();
        }
        // else: leave node=0, not passive; this is used for the clinet-side of a loopback connection.
    }
    
    std::stringstream strport;
    strport << port;
    addrinfo * res;
    int code = getaddrinfo(node, strport.str().c_str(), &hints, &res);
    if(code != 0){
        LOG_THROW("could not resolve hostname '" << hostname << "': " << gai_strerror(code));
    }
    vector<addrinfox> result;
    for (addrinfo * rp = res; rp != 0; rp = rp->ai_next) {
        result.push_back(addrinfox{rp->ai_flags, rp->ai_family, rp->ai_socktype, rp->ai_protocol, string(reinterpret_cast<char*>(rp->ai_addr), rp->ai_addrlen)});
    }
    freeaddrinfo(res);
    return result;
}

}


int dc::connect_to(const std::string & hostname, int port, int ai_family){
    auto logger = Logger::get("dc.netutils");
    int sockfd = 0;
    bool success = false;
    std::vector<addrinfox> addrinfos = getaddrinfox(hostname, port, ai_family);
    for (auto & ai : addrinfos) {
        LOG_DEBUG("trying to create socket for family=" << ai.ai_family << "; socktype=" << ai.ai_socktype << "; prot=" << ai.ai_protocol);
        sockfd = socket(ai.ai_family, ai.ai_socktype, ai.ai_protocol);
        if(sockfd == -1){
            LOG_DEBUG("socket creation failed: " << strerror(errno) << "; trying next addrinfo");
            continue;
        }
        else{
            LOG_DEBUG("socket creation succeeded; fd=" << sockfd);
        }
        int code = connect(sockfd, reinterpret_cast<const sockaddr*>(ai.ai_addr.data()), ai.ai_addr.size());
        if(code==-1){
            LOG_WARNING("calling connect for fd=" << sockfd << " failed: " << strerror(errno) << "; trying next addrinfo");
            close(sockfd);
            continue;
        }
        success = true;
        break;
    }
    if(!success){
        LOG_THROW("could not connect to " << hostname << ":" << port);
    }
    return sockfd;
}



int dc::bind_to(const std::string & hostname, int port, int ai_family){
    auto logger = Logger::get("dc.netutils");
    int sockfd = 0;
    bool success = false;
    std::vector<addrinfox> addrinfos = getaddrinfox(hostname, port, ai_family);
    for (auto & ai : addrinfos) {
        LOG_DEBUG("trying to create socket for family=" << ai.ai_family << "; socktype=" << ai.ai_socktype << "; prot=" << ai.ai_protocol);
        sockfd = socket(ai.ai_family, ai.ai_socktype, ai.ai_protocol);
        if(sockfd == -1){
            LOG_DEBUG("socket creation failed: " << strerror(errno) << "; trying next addrinfo");
            continue;
        }
        else{
            LOG_DEBUG("socket creation succeeded; fd=" << sockfd);
        }
        int true_ = 1;
        socklen_t len = sizeof(int);
        int code = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &true_, len);
        if(code==-1){
            LOG_WARNING("calling setsockopt for fd=" << sockfd << " failed: " << strerror(errno) << "; trying next addrinfo");
            close(sockfd);
            continue;
        }
        code = bind(sockfd, reinterpret_cast<const sockaddr*>(ai.ai_addr.data()), ai.ai_addr.size());
        if(code==-1){
            LOG_WARNING("calling bind for fd=" << sockfd << " failed: " << strerror(errno) << "; trying next addrinfo");
            close(sockfd);
            continue;
        }
        code = listen(sockfd, 128);
        if(code==-1){
            LOG_WARNING("calling listen for fd=" << sockfd << " failed: " << strerror(errno) << "; trying next addrinfo");
            close(sockfd);
            continue;
        }
        success = true;
        break;
    }
    
    if(!success){
        LOG_THROW("error in bind_to did not find any suitable addresses");
    }
    LOG_DEBUG("successfully bound server socket for " << hostname << ":" << port << " as fd=" << sockfd);
    return sockfd;
}



TcpAcceptor::TcpAcceptor(IOManager & iom_): logger(Logger::get("dc.TcpAcceptor")), iom(iom_){}

void TcpAcceptor::start(const new_connection_callback_type & cb, const string & addr, int port, IPMode ipm){
    new_connection_callback = cb;
    int aiaddr = AF_UNSPEC;
    switch(ipm){
        case IPMode::ipv4: aiaddr = AF_INET; break;
        case IPMode::ipv6: aiaddr = AF_INET6; break;
        case IPMode::ipboth: break;
    }
    ssocket = bind_to(addr, port, aiaddr);
    iom.add(ssocket, bind(&TcpAcceptor::on_ssocket_event, this, _1, _2));
    iom.set_events(ssocket, true, false);
}

void TcpAcceptor::stop(){
    if(ssocket >= 0){
        iom.remove(ssocket);
        ssocket = -1;
    }
}

TcpAcceptor::~TcpAcceptor(){
    stop();
}
   
void TcpAcceptor::on_ssocket_event(IOEvent e, int error){
    if(e==IOEvent::in){
        int connfd = accept(ssocket, 0, 0);
        if(connfd < 0){
            LOG_INFO("got 'in' event on listening socket, but accept returned -1: " << strerror(errno) << " ignoring");
            return;
        }
        LOG_INFO("new client connection with fd=" << connfd << "; creating channel and calling callback.");
        int one = 1;
        setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)); // ignore error
        unique_ptr<Channel> c(new Channel(connfd, iom));
        new_connection_callback(move(c));
    }
    else{
        // should be an error, as we never registered for out:
        assert(e == IOEvent::error);
        LOG_ERROR("error in server socket; stopping.");
        stop();
    }
}

