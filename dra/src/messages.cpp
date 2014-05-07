#include "messages.hpp"

using namespace dra;

REGISTER_MESSAGE(Configure, "dra:conf")
REGISTER_MESSAGE(Process, "dra:p")
REGISTER_MESSAGE(ProcessResponse, "dra:pr")
REGISTER_MESSAGE(Close, "dra:close")
REGISTER_MESSAGE(Merge, "dra:merge")
REGISTER_MESSAGE(Stop, "dra:stop")
