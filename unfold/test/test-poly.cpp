#include <boost/test/unit_test.hpp>
#include <iostream>

#include "poly.hpp"

using namespace std;

namespace {
    
string to_string(const poly & p){
    stringstream ss;
    p.print(ss);
    return ss.str();
}

string to_string(const poly_matrix & p){
    stringstream ss;
    p.print(ss);
    return ss.str();
}
    
}

BOOST_AUTO_TEST_SUITE(polytest)

BOOST_AUTO_TEST_CASE(simple){
    poly p;
    BOOST_CHECK_EQUAL(to_string(p), "0");
    
    poly p1(1.0);
    BOOST_CHECK_EQUAL(to_string(p1), "1");
    
    p1 *= p;
    BOOST_CHECK_EQUAL(to_string(p1), "0");
}


BOOST_AUTO_TEST_CASE(multiply){
    variable x("x"), y("y");
    
    // 2 * x**2 + x*y:
    poly px2(x);
    px2 *= x;
    px2 *= 2.0;
    
    poly pxy(x);
    pxy *= y;
    
    px2 += pxy;
    
    BOOST_CHECK_EQUAL(to_string(px2), "x y + 2 x^2");
    
    // check multiply with 0:
    poly ptest0(pxy);
    ptest0 *= 0;
    BOOST_CHECK_EQUAL(to_string(ptest0), "0");
    
    // check multiply with 1:
    poly ptest1(px2);
    ptest1 *= 1;
    BOOST_CHECK_EQUAL(to_string(ptest1), "x y + 2 x^2");
    
    // check multiply with itself:
    poly ptest_sq(px2);
    ptest_sq += 1.0;
    // 1 + 2 * x**2 + x*y
    ptest_sq *= ptest_sq;
    BOOST_CHECK_EQUAL(to_string(ptest_sq), "1 + 2 x y + 4 x^2 +  x^2 y^2 + 4 x^3 y + 4 x^4");   
}

BOOST_AUTO_TEST_CASE(polym){
    poly_matrix A("a", 2, 3);
    poly_matrix B("b", 3, 2);
    
    BOOST_CHECK_EQUAL(to_string(A), "a_0_0 | a_0_1 | a_0_2\na_1_0 | a_1_1 | a_1_2\n");
    BOOST_CHECK_EQUAL(to_string(B), "b_0_0 | b_0_1\nb_1_0 | b_1_1\nb_2_0 | b_2_1\n");
    
    poly_matrix AB = A*B;
    BOOST_CHECK_EQUAL(to_string(AB), "a_0_0 b_0_0 +  a_0_1 b_1_0 +  a_0_2 b_2_0 | a_0_0 b_0_1 +  a_0_1 b_1_1 +  a_0_2 b_2_1\na_1_0 b_0_0 +  a_1_1 b_1_0 +  a_1_2 b_2_0 | a_1_0 b_0_1 +  a_1_1 b_1_1 +  a_1_2 b_2_1\n");
}


BOOST_AUTO_TEST_SUITE_END()

