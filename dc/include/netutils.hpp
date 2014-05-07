#ifndef DC_NETUTILS_HPP
#define DC_NETUTILS_HPP

#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string>
#include <functional>
#include <memory>

#include "fwd.hpp"
#include "base/include/fwd.hpp"

namespace dc {

// return a tcp server socket listening on the specified hostname and port
int bind_to(const std::string & hostname, int port, int ai_family = AF_UNSPEC);

// connect to a tcp server at hostname:port
int connect_to(const std::string & hostname, int port, int ai_family = AF_UNSPEC);


enum class IPMode { ipv4, ipv6, ipboth };
class TcpAcceptor{
public:
    typedef std::function<void (std::unique_ptr<Channel> c)> new_connection_callback_type;

    explicit TcpAcceptor(IOManager & iom);
    
    void start(const new_connection_callback_type & cb, const std::string & addr, int port, IPMode ipm = IPMode::ipv4);
    
    // deregister socket from IOManager.
    void stop();
    
    ~TcpAcceptor();
    
private:
    void on_ssocket_event(IOEvent e, int error);
    
    std::shared_ptr<Logger> logger;
    IOManager & iom;
    new_connection_callback_type new_connection_callback;
    int ssocket;
};


}


#endif
