#include "channel.hpp"
#include <boost/test/unit_test.hpp>
#include <sys/types.h>
#include <sys/wait.h>

#include "utils.hpp"
       
#define NULLPTR (void*)0

using namespace dc;
using namespace std;


namespace{

struct channelpair{
    Channel c1, c2;
    channelpair(Channel && c1_, Channel && c2_): c1(move(c1_)), c2(move(c2_)){}
};

channelpair channel_pipe(IOManager & iom){
    int pipes[2];
    int res = pipe(pipes);
    BOOST_REQUIRE_EQUAL(res, 0);
    return channelpair(Channel(pipes[0], iom), Channel(pipes[1], iom));
}

class mymessage: public dc::Message{
public:
    int i, j;
    std::string msg;
    

    virtual void write_data(Buffer & out) const{
        out << i << j << msg;
    }
    
    virtual void read_data(Buffer & in){
        in >> i >> j >> msg;
    }
};

REGISTER_MESSAGE(mymessage, "ctest:mym");
    
}


BOOST_AUTO_TEST_SUITE(channel)


BOOST_AUTO_TEST_CASE(pipetest0){
    IOManager iom;
    channelpair p = channel_pipe(iom);
    
    unique_ptr<Message> received_message;
    p.c1.set_read_handler([&](unique_ptr<Message> m){received_message = move(m); });
    
    bool write_complete = false;
    mymessage m;
    m.i = 23;
    m.j = 42;
    m.msg = "hello, world!";
    p.c2.write(m, [&]{write_complete = true; p.c2.close(); });
    
    iom.process();
    p.c1.close();
    
    BOOST_CHECK(write_complete);
    BOOST_REQUIRE_NE(received_message.get(), NULLPTR);
    BOOST_REQUIRE(typeid(*received_message) == typeid(mymessage));
    
    mymessage & rmsg = dynamic_cast<mymessage&>(*received_message);
    BOOST_CHECK_EQUAL(rmsg.i, m.i);
    BOOST_CHECK_EQUAL(rmsg.j, m.j);
    BOOST_CHECK_EQUAL(rmsg.msg, m.msg);
}

BOOST_AUTO_TEST_CASE(large_message){
    IOManager iom;
    channelpair p = channel_pipe(iom);
    
    bool read_handler_called = false;
    int error = 0;
    mymessage m;
    m.i = 23;
    m.j = 42;
    m.msg.assign(1 << 21, 'a');
    p.c1.set_read_handler([&](unique_ptr<Message> m){read_handler_called = true;});
    p.c1.set_error_handler([&](int code){error = code;});
    
    bool write_complete = false;
    p.c2.write(m, [&]{write_complete = true; p.c2.close();});
    
    iom.process();
    
    // c2:
    //BOOST_CHECK(write_complete); // can be fulfilled or not: reader drops connection
    // as soon as header is received. Depending on the internal OS buffers, this might be
    // before or after the complete message has been sent
    // write handler called or not, c2 should be closed:
    BOOST_CHECK(p.c2.closed());
    // c1:
    BOOST_CHECK(!read_handler_called);
    BOOST_CHECK(error == EMSGSIZE);
    BOOST_CHECK(p.c1.closed());
}

BOOST_AUTO_TEST_CASE(pipetest1){
    const int nchildren = 10;
    int pipes[nchildren * 2];
    int child_read_fd = -1;
    pid_t children[nchildren];
    pid_t pid;
    for(int i=0; i<nchildren; ++i){
        int res = pipe(&pipes[2*i]);
        BOOST_REQUIRE(res == 0);
        pid = fork();
        if(pid==0){
            // in the child, close write end:
            child_read_fd = pipes[2*i];
            close(pipes[2*i+1]);
            break;
        }
        else{
            // in the parent, close read end:
            close(pipes[2*i]);
            children[i] = pid;
        }
    }
    if(pid==0){
        // in the child, read messages until the channel is closed, then exit:
        IOManager iom;
        Channel c(child_read_fd, iom);
        unique_ptr<Message> received_message;
        auto mypid = getpid();
        while(true){
            c.set_read_handler([&](unique_ptr<Message> m){received_message = move(m); });
            iom.process();
            //cout << mypid << ": received message: " << typeid(*received_message).name() << endl;
            mymessage & m = dynamic_cast<mymessage&>(*received_message);
            BOOST_CHECK(m.i == mypid);
            if(c.closed()) break;
        }
        exit(0);
    }
    // in the parent, send some messages to all children; then close their connection.
    IOManager iom;
    mymessage m;
    m.i = 23; // will be process id ...
    m.j = 43;
    m.msg = "abc";
    std::list<Channel> channels;
    std::vector<bool> sent(nchildren, false);
    for(int i=0; i<nchildren; ++i){
        channels.emplace_back(pipes[2*i+1], iom);
        Channel & c = channels.back();
        m.i = children[i];
        c.write(m, [&sent,i,&c]{sent[i] = true; c.close();});
    }
    iom.process();
    for(int i=0; i<nchildren; ++i){
        BOOST_CHECK(sent[i]);
    }
    
    // wait for all children and check that they exited normally:
    for(int i=0; i<nchildren; ++i){
        check_child_success(children[i]);
    }
}


BOOST_AUTO_TEST_SUITE_END()
