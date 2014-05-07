#include <boost/test/unit_test.hpp>
#include "log.hpp"
#include "utils.hpp"
#include "base/include/utils.hpp"
#include <fstream>

#include <sys/wait.h>

using namespace std;

BOOST_AUTO_TEST_SUITE(testlog)

BOOST_AUTO_TEST_CASE(logsink_file){
    //NOTE: might not be reliable, if file exists and owned by someone else ...
    const char * test_fname = "/tmp/logsink_file_test";
    unlink(test_fname);
    {
       auto logsink = LogSinkFactory::get_sink(test_fname);
       logsink->append("abc");
    }
    ifstream in(test_fname);
    string s;
    in >> s;
    BOOST_CHECK_EQUAL(s, "abc");
    BOOST_CHECK(in.eof());
}

// with relative path
BOOST_AUTO_TEST_CASE(logsink_file_rel){
    //NOTE: might not be reliable, if file exists and owned by someone else ...
    const char * test_fname = "logsink_file_test";
    unlink(test_fname);
    {
       auto logsink = LogSinkFactory::get_sink(test_fname);
       logsink->append("abc");
    }
    ifstream in(test_fname);
    string s;
    in >> s;
    BOOST_CHECK_EQUAL(s, "abc");
    BOOST_CHECK(in.eof());
    unlink(test_fname); // ignore error ...
}


BOOST_AUTO_TEST_CASE(logger0){
    const char * fname = "logger0test-out";
    unlink(fname); // ignore errors
    list<LoggerConfiguration> conf;
    conf.emplace_back("logger0test", LoggerConfiguration::set_outfile, fname);
    Logger::set_configuration(move(conf));
    auto logger = Logger::get("logger0test");
    
    LOG_ERROR("my error message 1");
    LOG_ERROR("my error message 2");
    
    logger.reset(); // this should close the logger and the underlying sink.

    ifstream in(fname);
    string s;
    getline(in, s);
    BOOST_CHECK_NE(s.find("message 1"), string::npos);
    getline(in, s);
    BOOST_CHECK_NE(s.find("message 2"), string::npos);
    getline(in, s);
    BOOST_CHECK(in.eof());
    unlink(fname); // ignore error ...
}

BOOST_AUTO_TEST_CASE(logger_before_fork){
    list<LoggerConfiguration> conf;
    conf.emplace_back("logtest", LoggerConfiguration::set_outfile, "logtest-out");
    Logger::set_configuration(move(conf));
    auto logger = Logger::get("logtest");
    int pid = dofork();
    if(pid == 0){
        //sleep(1);
        LOG_ERROR("child");
        logger.reset();
        exit(0);
    }
    LOG_ERROR("parent");
    check_child_success(pid);
    logger.reset();
    
    // there should be two files now: "logtest" and "logtest.p<childpid>", both with a single line:
    ifstream in("logtest-out");
    BOOST_REQUIRE(in.good());
    std::string line;
    getline(in, line);
    BOOST_CHECK_NE(line.find("parent"), string::npos);
    getline(in, line);
    BOOST_CHECK(line.empty());
    BOOST_CHECK(in.eof());
    in.close();
    unlink("logtest-out");
    
    stringstream fn2;
    fn2 << "logtest-out.p" << pid;
    string fname2 = fn2.str();
    ifstream in2(fname2.c_str());
    BOOST_REQUIRE(in2.good());
    getline(in2, line);
    BOOST_CHECK_NE(line.find("child"), string::npos);
    getline(in2, line);
    BOOST_CHECK(line.empty());
    BOOST_CHECK(in2.eof());
    in2.close();
    unlink(fn2.str().c_str());
}

BOOST_AUTO_TEST_CASE(logger_after_fork){
    list<LoggerConfiguration> conf;
    conf.emplace_back("logtest2", LoggerConfiguration::set_outfile, "logtest2-out");
    Logger::set_configuration(conf);
    //cout << "parent pid: "<< getpid() << endl;
    int pid = dofork();
    if(pid == 0){
        //sleep(1);
        //cout << "child pid: "<< getpid() << endl;
        auto logger = Logger::get("logtest2");
        LOG_ERROR("child");
        logger.reset(); // FIXME: this should not be necessary
        exit(0);
    }
    auto logger = Logger::get("logtest2");
    LOG_ERROR("parent");
    check_child_success(pid);
    logger.reset();
    
    // see logging_before_fork; it should be the same here:
    ifstream in("logtest2-out");
    BOOST_REQUIRE(in.good());
    std::string line;
    getline(in, line);
    BOOST_CHECK_NE(line.find("parent"), string::npos);
    getline(in, line);
    BOOST_CHECK(line.empty());
    BOOST_CHECK(in.eof());
    in.close();
    unlink("logtest2-out");
    
    stringstream fn2;
    fn2 << "logtest2-out.p" << pid;
    string fname2 = fn2.str();
    ifstream in2(fname2.c_str());
    BOOST_REQUIRE(in2.good());
    getline(in2, line);
    BOOST_CHECK_NE(line.find("child"), string::npos);
    getline(in2, line);
    BOOST_REQUIRE(line.empty());
    BOOST_CHECK(in2.eof());
    in2.close();
    unlink(fn2.str().c_str());
}

BOOST_AUTO_TEST_SUITE_END()
