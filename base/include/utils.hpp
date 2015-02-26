#ifndef BASE_UTILS_HPP
#define BASE_UTILS_HPP

// various utilities, mostly C++-friendly wrappers around C standard library
// functions for filesystem functions.

#include <string>
#include <functional>
#include <vector>

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


// read upto_nbytes bytes of the file contents into a string.
// In case upto_nbytes is negative or upto_nbytes exceeds the file size,
// the whole file is read.
std::string read_file(const std::string & path, int64_t upto_nbytes);
std::string read_file(int fd, int64_t upto_nbytes);

// write the \c contents string to the file descriptor fd
void write_file(int fd, const std::string & contents);

// Similar to rename(2), but fail if the new path exists rather than replacing it.
// Implemented by making a hard link under the new path and unlinking the old path.
// In case everything is successful, returns true.
// In case the new path exists, has no effect and returns false.
// In all other error cases (from link or from unlink), a system_error exception is thrown.
// Depending on the error, the hard link with the new_path might have been created already.
bool rename_soft(const std::string & old_path, const std::string & new_path);

// recursively creates directories (with mode 0700)
void mkdir_recursive(const std::string & path);

// expand a glob expression to a list of (sorted) filenames. Throws exception
// in case of error. If no match is found, the behavior depends on \c allow_no_match:
// If false, this is considered an error and an exception is thrown. Otherwise,
// an empty vector is returned.
std::vector<std::string> glob(const std::string& pattern, bool allow_no_match = true);


// get the hostname, truncated to 256 bytes (which should never be a problem in POSIX).
// Note that this method buffers the result for performance after the first call, thus changes in hostname
// are NOT reported here. If you need that, call gethostname directly.
std::string hostname();

#endif

