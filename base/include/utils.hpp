#ifndef BASE_UTILS_HPP
#define BASE_UTILS_HPP

#include <string>
#include <functional>

std::string demangle(const char * typename_);

// fork with callbacks: register callbacks with atfork
// and perform the actual fork with dofork (do not use fork directly, or
// th callbacks will not be called). The callbacks will be executed directly
// after the fork.

enum atfork_when { atfork_child = 1, atfork_parent = 1 << 1 };

// when should be a combinaion of one or more of atfork_child and atfork_parent.
void atfork(std::function<void()> callback, int when);
int dofork();


// filesystem functions. Symlinks are followed to judge the type.
// These routines also return false if there is not enough access permissions
// (you can check errno to see whether this was the case).
bool is_directory(const std::string & path);
bool is_regular_file(const std::string & path);

// fails with an exception in case realpath(3) returns an error
std::string realpath(const std::string & path);

#endif
