#include <unistd.h>
#include <iostream>

using namespace std;

void log(const string & message){
   static int last_pid(getpid());
   int pid = getpid();
   if(pid != last_pid){
      // handle fork: ...
      cout << "fork detected, changed pid from " << last_pid << " -> " << pid << endl;
      last_pid = pid;
   }
   cout << pid << " " << message << endl;
}


int main(){
    log("before_fork");
    auto pid = fork();
    if(pid==0){
        log("after fork, child");
    }
    log("after fork, parent");
}


