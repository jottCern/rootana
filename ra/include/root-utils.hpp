#ifndef ROOT_UTILS_HPP
#define ROOT_UTILS_HPP

#include <string>

namespace ra{

// merge rootfile file2 into file1. Both files must exist and contain the same objects.
void merge_rootfiles(const std::string & file1, const std::string & file2);

}

#endif
