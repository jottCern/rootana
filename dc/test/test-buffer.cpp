#include "buffer.hpp"

#include <boost/test/unit_test.hpp>
#include "buffer.hpp"
#include <ostream>
#include <stdexcept>

#define NULLPTR (void*)0

using namespace dc;
using namespace std;

BOOST_AUTO_TEST_SUITE(buffer)

BOOST_AUTO_TEST_CASE(allocation){
    Buffer b(200);
    
    BOOST_CHECK_EQUAL(b.reserved(), 0ul);
    BOOST_CHECK_EQUAL(b.size(), 0ul);
    BOOST_CHECK_EQUAL(b.position(), 0ul);
    BOOST_CHECK_EQUAL(b.data(), NULLPTR);
    
    b.reserve_for_write(1);
    BOOST_CHECK_EQUAL(b.reserved(), 200ul);
    BOOST_CHECK_EQUAL(b.size(), 0ul);
    BOOST_CHECK_EQUAL(b.position(), 0ul);
    BOOST_CHECK_NE(b.data(), NULLPTR);
}

BOOST_AUTO_TEST_CASE(check_for_read){
    Buffer b(200);
    int i = 9876;
    b << i;
    
    BOOST_CHECK_EQUAL(b.position(), sizeof(int));
    BOOST_CHECK_THROW(b.check_for_read(1), exception);
    b.seek(0);
    b.check_for_read(sizeof(int));
    BOOST_CHECK_THROW(b.check_for_read(sizeof(int) + 1), exception);
}

BOOST_AUTO_TEST_CASE(write_size){
    Buffer b(200);
    int i = 9876;
    b << i << i;
    
    BOOST_CHECK_EQUAL(b.position(), 2 * sizeof(int));
    BOOST_CHECK_EQUAL(b.size(), 2 * sizeof(int));
    
    b.seek(0);
    b << i;
    
    BOOST_CHECK_EQUAL(b.position(), sizeof(int));
    BOOST_CHECK_EQUAL(b.size(), sizeof(int));
}

BOOST_AUTO_TEST_CASE(write){
    Buffer b;
    
    BOOST_CHECK_EQUAL(b.position(), 0);
    BOOST_CHECK_EQUAL(b.size(), 0);
    
    int a = 23;
    b.write((const char*)&a, sizeof(int));
    
    BOOST_CHECK_EQUAL(b.size(), sizeof(int));
    BOOST_CHECK_EQUAL(b.position(), sizeof(int));
    
    for(int i=0; i<9; ++i){
        b.write((const char*)&a, sizeof(int));
    }
    
    BOOST_CHECK_EQUAL(b.size(), 10 * sizeof(int));
    BOOST_CHECK_EQUAL(b.position(), 10 * sizeof(int));
    
    b.seek(0);
    b.write((const char*)&a, sizeof(int));
    BOOST_CHECK_EQUAL(b.size(), sizeof(int));
    BOOST_CHECK_EQUAL(b.position(), sizeof(int));
}

BOOST_AUTO_TEST_CASE(int_writing){
    Buffer b;
    
    int i = 0;
    b << i;
    
    BOOST_CHECK_EQUAL(b.position(), sizeof(int));
    BOOST_CHECK_EQUAL(b.size(), sizeof(int));
    BOOST_CHECK_GE(b.reserved(), sizeof(int));
    
    b.seek(0);
    i = 1;
    BOOST_CHECK_EQUAL(b.position(), 0ul);
    b >> i;
    BOOST_CHECK_EQUAL(i, 0);
    
    
    i = 983275;
    int j = 0;
    b.seek(0);
    b << i;
    b.seek(0);
    b >> j;
    BOOST_CHECK_EQUAL(i, j);
}

BOOST_AUTO_TEST_CASE(seek){
    Buffer b;
    
    BOOST_CHECK_THROW(b.seek(1), std::out_of_range);
    BOOST_CHECK_EQUAL(b.position(), 0ul);
    
    // reservation should still make seek fail:
    b.reserve_for_write(10);
    BOOST_CHECK_THROW(b.seek(1), std::out_of_range);
    
    int i = 17;
    b << i;
    b.seek(2);
    // position should be at seek
    BOOST_CHECK_EQUAL(b.position(), 2ul);
    // size is unchanged:
    BOOST_CHECK_EQUAL(b.size(), sizeof(int));
    
    b.seek_resize(2);
    BOOST_CHECK_EQUAL(b.position(), 2ul);
    BOOST_CHECK_EQUAL(b.size(), 2ul);
}

BOOST_AUTO_TEST_CASE(movetest){
    Buffer b(200);
    int i = 42;
    b << i;
    
    BOOST_CHECK_EQUAL(b.position(), sizeof(int));
    BOOST_CHECK_EQUAL(b.size(), sizeof(int));
    BOOST_CHECK_EQUAL(b.reserved(), 200);
    
    Buffer b2(move(b));
    BOOST_CHECK_EQUAL(b2.position(), sizeof(int));
    BOOST_CHECK_EQUAL(b2.size(), sizeof(int));
    BOOST_CHECK_EQUAL(b2.reserved(), 200);
    BOOST_CHECK_EQUAL(b.data(), NULLPTR);
    
    b2.seek(0);
    int j = 0;
    b2 >> j;
    BOOST_CHECK_EQUAL(i,j);
}

BOOST_AUTO_TEST_CASE(seek_resize){
    Buffer b(200);
    
    int i = 1;
    b << i;
    
    BOOST_CHECK_EQUAL(b.reserved(), 200);
    b.seek_resize(183);
    BOOST_CHECK_EQUAL(b.position(), 183);
    BOOST_CHECK_EQUAL(b.size(), 183);
    
    b.seek_resize(200);
    BOOST_CHECK_EQUAL(b.position(), 200);
    BOOST_CHECK_EQUAL(b.size(), 200);
    
    BOOST_CHECK_THROW(b.seek_resize(201), exception);
}

BOOST_AUTO_TEST_CASE(reserve_for_write){
    const size_t cs = 4; // chunk size
    Buffer b(cs);
    b.reserve_for_write(1);
    BOOST_CHECK_EQUAL(b.position(), 0);
    BOOST_CHECK_EQUAL(b.size(), 0);
    BOOST_CHECK_EQUAL(b.reserved(), cs);
    
    void* ptr0 = b.data();
    
    // reserving a cs should be a noop;
    b.reserve_for_write(cs); 
    BOOST_CHECK_EQUAL(ptr0, (void*)b.data());
    
    b.data()[0] = 'a';
    b.data()[1] = 'b';
    
    b.seek_resize(cs);
    b.reserve_for_write(1);
    BOOST_CHECK_EQUAL(b.position(), cs);
    BOOST_CHECK_EQUAL(b.reserved(), 2 * cs);
    
    // test that re-allocation moved things correctly:
    //BOOST_CHECK_NE(ptr0, (void*)b.data()); // they can be the same: realloc does not need to change position ...
    BOOST_CHECK_EQUAL(b.data()[0], 'a');
    BOOST_CHECK_EQUAL(b.data()[1] ,'b');
}

BOOST_AUTO_TEST_SUITE_END()
