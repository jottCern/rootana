#include "channel.hpp"
#include "base/include/log.hpp"

#include <boost/bind.hpp>
#include <set>

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/socket.h>

using namespace dc;
using namespace std;

#define CHANNEL_LOG(sev, msg) LOG(logger, sev, "fd=" << fd << "; " << msg);


Channel::Channel(int fd_, IOManager & iom_): logger(Logger::get("dc.Channel")), fd(fd_), iom(iom_), max_message_size(1 << 20), io_handler_active(false){
    iom.add(fd, boost::bind(&Channel::io_handler, this, _1, _2));
}

Channel::Channel(Channel && other): logger(other.logger), fd(other.fd), iom(other.iom), max_message_size(other.max_message_size),
   bin(move(other.bin)), bout(move(other.bout)),
   error_handler(move(other.error_handler)), read_handler(move(other.read_handler)), write_handler(move(other.write_handler)),
   io_handler_active(other.io_handler_active) {
   other.error_handler = boost::none;
   other.read_handler = boost::none;
   other.write_handler = boost::none;
   other.io_handler_active = false;
   other.fd = -1;
   // update event handler:
   iom.set_event_handler(fd, boost::bind(&Channel::io_handler, this, _1, _2));
}

Channel::~Channel(){
    CHANNEL_LOG(loglevel::debug, "~Channel");
    if(io_handler_active){
        LOG_THROW("~Channel called while its io handler is active. This is not allowed.");
    }
    close();
}

void Channel::close(){
    if(fd < 0) return;
    iom.remove(fd, true);
    fd = -1;
}

void Channel::handle_error(int errorcode){
    if(errorcode != ECONNRESET){
        CHANNEL_LOG(loglevel::error, "handle_error: system error message: " << strerror(errorcode));
    }
    else{
        CHANNEL_LOG(loglevel::debug, "handle_error: connection reset");
    }
    close();
    if(error_handler){
        (*error_handler)(errorcode);
    }
}

void Channel::set_read_handler(read_handler_type handler){
    if(fd < 0){
        LOG_WARNING("set_read_handler called on invalid/closed channel");
        return;
    }
    if(read_handler){
        LOG_THROW("set_read_handler called while another read handler was still active");
    }
    bin.seek_resize(0);
    read_handler = move(handler);
    update_io_registration();
}

void Channel::set_error_handler(error_handler_type handler){
    if(fd < 0){
        LOG_WARNING("set_error_handler called on invalid/closed channel");
        return;
    }
    error_handler = handler;
}

void Channel::write(const Message & m, write_handler_type handler){
    if(fd < 0){
        LOG_WARNING("write called on invalid/closed channel");
        return;
    }
    // ensure that there is no current write:
    if(write_handler){
        CHANNEL_LOG(loglevel::error, "write called while another write is active; will throw");
        throw invalid_argument("Channel::write called, but another write is active!");
    }
    CHANNEL_LOG(loglevel::debug, "writing message of type " << typeid(m).name());
    bout.seek(0);
    bout << static_cast<uint64_t>(0);
    bout << m;
    // now write the correct size:
    uint64_t size = bout.size();
    bout.seek(0);
    *reinterpret_cast<uint64_t*>(bout.data()) = size;
    write_handler = move(handler);
    update_io_registration();
}

namespace{
    
struct setter_resetter{
    bool & b;
    explicit setter_resetter(bool & b_): b(b_){
        b = true;
    }
    ~setter_resetter() {
        b = false;
    }
};

}


void Channel::io_handler(IOEvent e, int error){
    LOG_DEBUG("entering Channel_io_handler, fd=" << fd);
    assert(!io_handler_active); // should never happen: io_handler is only
    // called from IOManager::process, and IOManager::process is already protected from calling itself
    setter_resetter s(io_handler_active);
    if(fd < 0){
        LOG_WARNING("io_handler called on invalid/closed channel");
        return;
    }
    if(e==IOEvent::in){
        CHANNEL_LOG(loglevel::debug, "io_handler called with in event");
        perform_reads();
        if(fd > 0) update_io_registration();
    }
    else if(e==IOEvent::out){
        CHANNEL_LOG(loglevel::debug, "io_handler called with out event");
        perform_writes();
        if(fd > 0) update_io_registration();
    }
    else{
        // handle error: remove from the watch list and notify user:
        handle_error(error);
    }
}

void Channel::update_io_registration(){
    if(fd < 0){
        LOG_WARNING("update_io_reg called on invalid/closed channel");
        return;
    }
    // we want to have 'in' events if there's a read handler:
    bool in = read_handler;
    bool out = write_handler;
    CHANNEL_LOG(loglevel::debug, "update_io_reg: in = " << in << "; out = " << out);
    iom.set_events(fd, in, out);
}

void Channel::perform_reads(){
    assert(fd >= 0);
    CHANNEL_LOG(loglevel::debug, "perform_reads entered");
    // read/schedule reads if we have the (buffer) space or we have a partial read message:
    bool header_read = bin.position() >= sizeof(uint64_t);
    for(int i = header_read ? 1 : 0; i<2; ++i){ // i==0: read header, i==1: read data
        ssize_t to_read;
        if(i==0){
            to_read = sizeof(uint64_t) - bin.position();
            assert(to_read > 0);
        }else{
            uint64_t bufsize = *reinterpret_cast<const uint64_t*>(bin.data());
            if(bufsize <= sizeof(uint64_t)){
                CHANNEL_LOG(loglevel::error, "perform_reads: malformed message: buffer size = " << bufsize);
                handle_error(EBADMSG);
                return;
            }
            if(bufsize > max_message_size){
                CHANNEL_LOG(loglevel::error, "perform_reads: message to large, calling handle_error(EMSGSIZE)");
                handle_error(EMSGSIZE);
                return;
            }
            to_read =  bufsize - bin.position();
            assert(to_read > 0);
        }
        //CHANNEL_LOG(loglevel::debug, "perform_reads: i=" << i << "; bin.position=" << bin.position() << "; to_read=" << to_read);
        bin.reserve_for_write(to_read);
        ssize_t res;
        do{
            res = read(fd, bin.data() + bin.position(), to_read);
        }while(res < 0 && errno == EINTR);
        
        if(res > 0){
            bin.seek_resize(bin.position() + res);
            // short read: return and wait for more data:
            if(res < to_read) return;
        }
        else if(res < 0){
            int errorcode = errno;
            if(errorcode==EAGAIN){
                return;
            }
            else{
                CHANNEL_LOG(loglevel::warning, "perform_reads: read returned error " << strerror(errorcode));
                handle_error(errorcode);
                return;
            }
        }
        else if(res == 0){
            if(bin.position()==0){
                // clean close between messages; report as ECONNRESET
                //CHANNEL_LOG(loglevel::debug, "perform_reads: EOF after reading complete message; calling handle_error(ECONNRESET)");
                handle_error(ECONNRESET);
                return;
            }
            else{
                // this means the channel has been closed cleanly between messages; report as ECONNABORTED
                CHANNEL_LOG(loglevel::warning, "perform_reads: EOF in the middle of a message, calling handle_error(ECONNABORTED)");
                handle_error(ECONNABORTED);
                return;
            }
        }
        // we read everything to_read in this iteration over i:
        assert(res == to_read);
        if(i==0){
            // we just read the header; continue with data ...
            continue;
        }
        else{
            //CHANNEL_LOG(loglevel::debug, "perform_reads: read complete buffer, size=" << bin.size() << "; performing de-serialization now.");
            // convert to a message object, starting after header:
            bin.seek(sizeof(uint64_t));
            unique_ptr<Message> m;
            bin >> m;
            assert(read_handler);
            read_handler_type rh = move(*read_handler); // none
            read_handler = boost::none;
            rh(move(m));
        }
    }
}

  
void Channel::perform_writes(){
    if(fd < 0){
        LOG_WARNING("perform_writes called on invalid/closed channel");
        return;
    }
    //CHANNEL_LOG(loglevel::debug, "entering perform_writes");
    ssize_t to_write = bout.size() - bout.position();
    ssize_t res;
    do{
        res = ::write(fd, bout.data() + bout.position(), to_write);
    }while(res < 0 && errno == EINTR);
    
    if(res < 0){
        int errorcode = errno;
        if(errorcode==EAGAIN){
            return;
        }
        else{
            CHANNEL_LOG(loglevel::warning, "perform_writes: got io error on write");
            handle_error(errorcode);
            return;
        }
    }
    else if(res == to_write){
        //CHANNEL_LOG(loglevel::debug, "perform_writes: write complete buffer, calling write_handler now.");
        // call the user handler:
        assert(write_handler);
        write_handler_type wh = move(*write_handler);
        write_handler = boost::none;
        wh();
    }
    //otherwise: short write, return and wait to be called again ...
}
