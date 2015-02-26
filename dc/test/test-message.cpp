#include "message.hpp"
#include "base/include/log.hpp"
#include "netutils.hpp"

#include <iostream>
#include <memory>
#include <boost/test/unit_test.hpp>

#define NULLPTR (void*)0

using namespace dc;
using namespace std;

namespace {

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

}

REGISTER_MESSAGE(mymessage, "mym");


BOOST_AUTO_TEST_SUITE(message)


BOOST_AUTO_TEST_CASE(test1){
    mymessage m;
    m.i = 1;
    m.j = 2;
    m.msg = "hello, world!";

    BOOST_CHECKPOINT("writing buffer");
    Buffer b;
    b << m;
    BOOST_CHECK_GT(b.size(), 0u);
    BOOST_CHECK_EQUAL(b.position(), b.size());
    BOOST_CHECK_NE(b.data(), NULLPTR);
    
    BOOST_CHECKPOINT("reading buffer");
    
    std::unique_ptr<Message> m_read_ptr;
    b.seek(0);
    BOOST_CHECK_GT(b.size(), 0u);
    BOOST_CHECK_EQUAL(b.position(), 0u);
    b >> m_read_ptr;
    BOOST_REQUIRE_NE(m_read_ptr.get(), NULLPTR);
    BOOST_REQUIRE(typeid(*m_read_ptr) == typeid(mymessage));
    
    mymessage & m_read = *dynamic_cast<mymessage*>(m_read_ptr.get());
    BOOST_CHECK_EQUAL(m_read.i, m.i);
    BOOST_CHECK_EQUAL(m_read.j, m.j);
    BOOST_CHECK_EQUAL(m_read.msg, m.msg);
}

    
BOOST_AUTO_TEST_CASE(test_null){
    Buffer b;
    unique_ptr<Message> m_read;
    b.seek(0);
    b << m_read;
    
    //cout << "size of null message: " << b.size() << endl;
    
    m_read.reset(new mymessage());
    b.seek(0);
    b >> m_read;
    
    BOOST_CHECK_EQUAL(m_read.get(), NULLPTR);
}

BOOST_AUTO_TEST_SUITE_END()
