#include "message.hpp"

using namespace dc;

namespace {
uint64_t null_typecode = 0xffffffffffffffff;
}

Message::~Message(){}

Buffer & dc::operator<<(Buffer & out, const Message & m){
    out << MessageTypeRegistry::instance().key(m);
    m.write_data(out);
    return out;
}

Buffer & dc::operator<<(Buffer & out, const std::unique_ptr<Message> & m){
    if(m) return out << *m;
    else{
        return out << null_typecode;
    }
}

Buffer & dc::operator>>(Buffer & in, std::unique_ptr<Message> & m){
    uint64_t typecode;
    in >> typecode;
    if(typecode==null_typecode){
       m.reset();
    }
    else{
       m = MessageTypeRegistry::instance().build(typecode);
       m->read_data(in);
    }
    return in;
}



uint64_t dc::string_to_uint64(const char * name){
    uint64_t result = 0;
    // convert message to 64 bit integer by using 6 bits for each character.
    // (I use uint64_t everywhere to avoid implicit conversion to int which could screw up everything ...).
    const uint64_t bits_per_char = 6;
    uint64_t i=0;
    for(; i<10; ++i){
        if(name[i]=='\0') break;
        // map the valid characters to a code in the numerical range 0..63, in this order: 0-9:A-Z_a-z
        uint64_t code;
        if(name[i] >= '0' and name[i] <= ':'){ //0-9: in ASCII are consecutive ...
            code = name[i] - '0'; // characters '0'-'9' are codes 0-9; ':' is 10
        }
        else if(name[i] >= 'A' and name[i] <= 'Z'){
            code = name[i] + 11 - 'A'; // 'A' is 11 ... 'Z' is 11 + 26 - 1 = 36
        }
        else if(name[i]=='_'){
            code = 37;
        }
        else if(name[i] >= 'a' and name[i] <= 'z'){
            code = name[i] - 'a' + 38; // 'a' is 38, ... 'z' is 38+26-1 = 63
        }
        else{
            throw std::logic_error(std::string("encountered invalid character in message name '") + name + "'");
        }
        result |= (code << (i * bits_per_char));
    }
    if(name[i]!='\0') throw std::logic_error(std::string("name '") + name + "too long.");
    return result;
}