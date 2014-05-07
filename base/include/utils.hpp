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

#endif
