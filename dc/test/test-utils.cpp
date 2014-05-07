#include "utils.hpp"
#include <boost/test/unit_test.hpp>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/signal.h>

#include <string.h>

// set up signal handler to not ignore SIGCHLD. This put child processes in zombie state, so that they can
// be waited for

using namespace std;

namespace{
    
void chld_handler(int){}
    
int setup_handler(){
    struct sigaction sa;
    sa.sa_handler = &chld_handler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // avoid EINTR loop in waitpid
    return 0;
}

int dummy = setup_handler();

}


void check_child_success(pid_t pid){
    int status = 0;
    pid_t pid2;
    do{
        pid2 = waitpid(pid, &status, 0);
    }while(pid2 < 0 && errno == EINTR);
    if(pid2 < 0){
        cerr << "child success: waitpid returned error: " << strerror(errno) << endl;
    }
    BOOST_CHECK_EQUAL(pid, pid2);
    BOOST_CHECK_EQUAL(bool(WIFEXITED(status)), true);
    BOOST_CHECK_EQUAL(WEXITSTATUS(status), 0);
}

