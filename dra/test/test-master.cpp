#include "master.hpp"
#include <boost/test/unit_test.hpp>

using namespace dra::detail;
using namespace std;

BOOST_AUTO_TEST_SUITE(master)

BOOST_AUTO_TEST_CASE(indexranges0){
    IndexRanges ir(0, 1);
    BOOST_CHECK(!ir.empty());
    BOOST_CHECK_EQUAL(ir.size(), 1);
    
    IndexRanges ir2(0, 0);
    BOOST_CHECK(ir2.empty());
    
    BOOST_CHECK_THROW(IndexRanges(1, 0), invalid_argument);
    
    IndexRanges iempty;
    BOOST_CHECK(iempty.empty());
    BOOST_CHECK_EQUAL(iempty.size(), 0);
}

BOOST_AUTO_TEST_CASE(ir_consume){
    IndexRanges ir(0, 10);
    BOOST_CHECK_EQUAL(ir.size(), 10);
    
    auto i = ir.consume(5);
    BOOST_CHECK_EQUAL(i.first, 0);
    BOOST_CHECK_EQUAL(i.second, 5);
    BOOST_CHECK_EQUAL(ir.size(), 5);
    
    i = ir.consume(2);
    BOOST_CHECK_EQUAL(i.first, 5);
    BOOST_CHECK_EQUAL(i.second, 7);
    BOOST_CHECK_EQUAL(ir.size(), 3);
    
    i = ir.consume(30);
    BOOST_CHECK_EQUAL(i.first, 7);
    BOOST_CHECK_EQUAL(i.second, 10);
    BOOST_CHECK_EQUAL(ir.size(), 0);
    
    BOOST_CHECK(ir.empty());
    
    BOOST_CHECK_THROW(ir.consume(1), invalid_argument);
    
    ir = IndexRanges(0, 10);
    ir.consume(10);
    BOOST_CHECK(ir.empty());
}

BOOST_AUTO_TEST_CASE(indexranges_union_empty){
    IndexRanges ir(0, 10);
    IndexRanges ir2(1, 1);
    ir.disjoint_union(ir2);
    ir.consume(10);
    BOOST_CHECK(ir.empty());
}

BOOST_AUTO_TEST_CASE(indexranges_union){
    IndexRanges ir(0, 10);
    IndexRanges ir2(9, 10);
    
    BOOST_CHECK_THROW(ir.disjoint_union(ir2), invalid_argument);
}

BOOST_AUTO_TEST_CASE(indexranges_unionfail){
    IndexRanges ir(0, 10);
    IndexRanges ir2(10, 20);
    
    ir.disjoint_union(ir2);
    auto i = ir.consume(10);
    BOOST_CHECK_EQUAL(i.first, 0);
    BOOST_CHECK_EQUAL(i.second, 10);
    ir.consume(10);
    BOOST_CHECK(ir.empty());
    
    IndexRanges ir3(19, 21);
    BOOST_CHECK_THROW(ir2.disjoint_union(ir3), invalid_argument);
    IndexRanges ir4(10, 11);
    BOOST_CHECK_THROW(ir2.disjoint_union(ir4), invalid_argument);
}


BOOST_AUTO_TEST_CASE(indexranges_union3){
    IndexRanges ir(0, 10);
    IndexRanges ir2(10, 23);
    IndexRanges ir3(23, 40);
    
    // combine by "filling the middle" in the end
    ir.disjoint_union(ir3);
    ir.disjoint_union(ir2);
    
    auto i = ir.consume(40);
    BOOST_CHECK_EQUAL(i.first, 0);
    BOOST_CHECK_EQUAL(i.second, 40);
    BOOST_CHECK(ir.empty());
}

BOOST_AUTO_TEST_CASE(indexranges_union4){
    IndexRanges ir(0, 10);
    IndexRanges ir2(10, 23);
    IndexRanges ir3(23, 40);
    IndexRanges ir4(40, 50);
    
    // combine by "filling the middle" in the end
    ir.disjoint_union(ir3);
    ir2.disjoint_union(ir4);
    ir.disjoint_union(ir2);
    
    ir.consume(50);
    BOOST_CHECK(ir.empty());
}

BOOST_AUTO_TEST_CASE(er0){
    EventRangeManager erm(2, 100);
    BOOST_CHECK(erm.available());
    auto er = erm.consume(0);
    BOOST_CHECK_EQUAL(er.ifile, 0);
    BOOST_CHECK_EQUAL(er.first, 0);
    BOOST_CHECK_EQUAL(er.last, 100);
    
    auto er2 = erm.consume(0);
    BOOST_CHECK_EQUAL(er2.ifile, 1);
    BOOST_CHECK_EQUAL(er2.first, 0);
    BOOST_CHECK_EQUAL(er2.last, 100);
    
    BOOST_CHECK(!erm.available());
    BOOST_CHECK_THROW(erm.consume(0), invalid_argument);
}

BOOST_AUTO_TEST_CASE(erm_fs){
    EventRangeManager erm(2, 100);
    erm.consume(0);
    erm.consume(0);
    BOOST_CHECK(!erm.available());
    erm.set_file_size(0, 130);
    BOOST_CHECK(erm.available());
    auto er = erm.consume(1);
    BOOST_CHECK_EQUAL(er.ifile, 0);
    BOOST_CHECK_EQUAL(er.first, 100);
    BOOST_CHECK_EQUAL(er.last, 130);
}

BOOST_AUTO_TEST_CASE(erm_fs_fail){
    EventRangeManager erm(2, 100);
    erm.set_file_size(0, 100);
    BOOST_CHECK_THROW(erm.set_file_size(0, 101), invalid_argument);
}

BOOST_AUTO_TEST_CASE(erm_add){
    EventRangeManager erm(2, 100);
    erm.set_file_size(0, 100);
    // adding beyond range or for unknown file size is forbidden:
    BOOST_CHECK_THROW(erm.add({0, 100, 101}), invalid_argument);
    BOOST_CHECK_THROW(erm.add({1, 100, 101}), invalid_argument);
    erm.set_file_size(1, 200);
    erm.consume();
    erm.consume();
    auto er3 = erm.consume();
    BOOST_CHECK(!erm.available());
    erm.add(er3);
    BOOST_CHECK(erm.available());
}

BOOST_AUTO_TEST_CASE(erm_blocksize){
    EventRangeManager erm(2, 100);
    auto er0 = erm.consume();
    BOOST_CHECK_EQUAL(er0.ifile, 0);
    erm.set_file_size(0, 350);
    
    // second worker requests file: should give unrelated ifile=1,
    // but with blocksize0, although we requested more:
    auto er1 = erm.consume(-1, 200);
    BOOST_CHECK_EQUAL(er1.ifile, 1);
    BOOST_CHECK_EQUAL(er1.first, 0);
    BOOST_CHECK_EQUAL(er1.last, 100);
    
    // the only file left is file0:
    auto er2 = erm.consume();
    BOOST_CHECK_EQUAL(er2.ifile, 0);
    BOOST_CHECK_EQUAL(er2.first, 100);
    BOOST_CHECK_EQUAL(er2.last, 200);
    
    auto er3 = erm.consume(-1, 200);
    BOOST_CHECK_EQUAL(er3.ifile, 0);
    BOOST_CHECK_EQUAL(er3.first, 200);
    BOOST_CHECK_EQUAL(er3.last, 350);
    
    BOOST_CHECK(!erm.available());
    
    // for file1, we do not know the size yet;
    // scenario 1: it is larger than blocksize0=100:
    {
        auto erm_s1(erm);
        erm_s1.set_file_size(1, 201);
        BOOST_CHECK(erm_s1.available());
        auto er = erm_s1.consume(1, 201);
        BOOST_CHECK_EQUAL(er.ifile, 1);
        BOOST_CHECK_EQUAL(er.first, 100);
        BOOST_CHECK_EQUAL(er.last, 201);
        BOOST_CHECK(!erm_s1.available());
    }
    // scenario 2: file size is smaller than blocksize0:
    {
        auto erm_s2(erm);
        erm_s2.set_file_size(1, 50);
        BOOST_CHECK(!erm_s2.available());
        // but check that adding first block with blocksize0 still succeeds,
        // and consuming returns that ...
        erm_s2.add(er1);
        BOOST_CHECK(erm_s2.available());
        
        // consuming now returns this first block (even though it extends beyond the
        // known file size!):
        auto er = erm_s2.consume(-1, 50);
        BOOST_CHECK_EQUAL(er.ifile, 1);
        BOOST_CHECK_EQUAL(er.first, 0);
        BOOST_CHECK_EQUAL(er.last, 100);
        BOOST_CHECK(!erm_s2.available());
    }
    
}


BOOST_AUTO_TEST_SUITE_END()
