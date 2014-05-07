#include "buffer.hpp"

#include <stdexcept>
#include <sstream>

using namespace dc;

Buffer::Buffer(size_t chunk_size): position_(0), size_(0), reserved_(0), cs_(chunk_size), data_(0) {
    if(cs_==0) cs_ = 1;
    //if(cs_>8192) cs_ = 8192;
}

Buffer::Buffer(Buffer && other): position_(other.position_), size_(other.size_), reserved_(other.reserved_),cs_(other.cs_), data_(other.data_){
    other.position_ = other.size_ = other.reserved_ = 0;
    other.data_ = 0;
}

Buffer::~Buffer(){
    free(data_); // also ok if data_ is NULL
}

void Buffer::reallocate(size_t new_min_reserved){
    // round up to nearest chunk size:
    size_t new_reserved = (new_min_reserved / cs_) * cs_;
    if(new_min_reserved % cs_ > 0){
        new_reserved += cs_;
    }
    // note: realloc also works if current data_ is NULL
    char * new_data = reinterpret_cast<char*>(realloc(reinterpret_cast<void*>(data_), new_reserved));
    if(new_data==0){
        throw std::bad_alloc();
    }
    else {
        reserved_ = new_reserved;
        data_ = new_data;
    }
}

void Buffer::fail_check_for_read(size_t s) const{
    std::stringstream ss;
    ss << "Buffer: boundary check for reading failed: at position " << position_ << ": tried to read " << s << " bytes, but total size is only " << size_;
    throw std::out_of_range(ss.str());
}

void Buffer::fail_seek(size_t s) const{
    std::stringstream ss;
    ss << "Buffer: boundary check for seeking failed: asked to seek to position " << s << " but total size is only " << size_;
    throw std::out_of_range(ss.str());
}

void Buffer::fail_seek_resize(size_t s) const{
    std::stringstream ss;
    ss << "Buffer: boundary check for seek_resize failed: asked to seek to position " << s << " but total reserved size is only " << reserved_;
    throw std::out_of_range(ss.str());
}


// free functions:

Buffer & dc::operator<<(Buffer & out, const std::string & s){
    out.reserve_for_write(sizeof(uint32_t) + s.size());
    out << static_cast<uint32_t>(s.size());
    out.write(s.data(), s.size());
    return out;
}

Buffer & dc::operator>>(Buffer & in, std::string & s){
    uint32_t size;
    in >> size;
    in.check_for_read(size);
    size_t pos0 = in.position();
    s.assign(in.data() + pos0, size);
    // update position:
    in.seek(pos0 + size);
    return in;
}
