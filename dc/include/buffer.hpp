#ifndef DC_BUFFER_HPP
#define DC_BUFFER_HPP

#include <string>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

namespace dc{

/** \brief A memory area for reading/writing raw bytes, with boundary-checking
 * 
 * A buffer has a current position, a size, and a reserved size. The reserved size is always at least the size;
 * having a reserved size larger than the size is an optimization to avoid re-allocation on writes.
 * Reading and writing is done starting at the current position and advances the current position by the number
 * of bytes read.
 * 
 * Access to the memory is possible through two levels of interfaces:
 *  - read, write, seek, truncate: these methods do boundary-checking and (in case of write) automatic re-allocation.
 *  - data can also be accessed on a raw level through the data() pointer for optimization. In this case, any boudary checking
 *    is the caller's responsibility; the method 'check_for_read' and 'reserve_for_write' are exposed to ensure
 *    enough bytes can be read / written at the current position.
 * 
 * To avoid frequent re-allocation, allocation is done integer multiples of a chunk size which is specified at construction time.
 */
class Buffer final {
public:
    
    // disallow copying:
    Buffer(const Buffer &) = delete;
    Buffer & operator=(Buffer &&) = delete;
    Buffer & operator=(const Buffer &) = delete;
    
    // support moving:
    Buffer(Buffer &&);
    
    explicit Buffer(size_t chunk_size = 128);
    
    ~Buffer();
    
    const char * data() const{
        return data_;
    }
    
    char * data(){
        return data_;
    }
    
    size_t position() const {
        return position_;
    }
    
    size_t size() const {
        return size_;
    }
    
    size_t reserved() const{
        return reserved_;
    }
    
    /** \brief Set the current position.
     * 
     * throws an out_of_range exception in case of an attempted seek
     * beyond the current size.
     */
    void seek(size_t pos){
        if(pos > size_) fail_seek(pos);
        position_ = pos;
    }
    
    /** \brief Set current position and size
     * 
     * Seek to a new position possibly outside the current size (but within current reserved).
     * This is needed after writing directly to the underlying buffer. Both position
     * and size will be updated to new_size.
     * 
     * If new_size is not in the current reserve, it will fail with an exception.
     */
    void seek_resize(size_t new_size){
        if(new_size > reserved_) fail_seek_resize(new_size);
        position_ = size_ = new_size;
    }
    
    
    // routines for writing:
    
    /** \brief Ensure there is enough space to write the given number of bytes at the current position
     *
     * Checks whether the current reserved size is large enough; if not, re-allocates. Does not modify
     * the current position and size, but does update reserved in case a re-allocation was performed.
     * 
     * Throws bad_alloc in case memory is exhausted; in this case, the Buffer is unmodified.
     */
    void reserve_for_write(size_t write_size){
        if(position_ + write_size > reserved_){
            reallocate(position_ + write_size);
        }
    }
    
    /** \brief Write a number of bytes into the buffer
     * 
     * Moves position forward by s. Re-allocates if necessary.
     * The size will be the new position.
     */
    void write(const char * data, size_t s){
        reserve_for_write(s);
        memcpy(data_ + position_, data, s);
        position_ += s;
        size_ = position_;
    }
    
    template<typename T>
    void write_bitwise(const T & t){
        write(reinterpret_cast<const char*>(&t), sizeof(T));
    }
    
    /** \brief Check that the size is large enough to read a given number of bytes from current position
     * 
     * Checks whether position + read_size <= size.
     *
     * If this is not fulfiled, a std::out_of_range exception is thrown.
     */
    void check_for_read(size_t read_size) const{
        if(position_ + read_size > size_) fail_check_for_read(read_size);
    }
    
    /** \brief Read some data from the current position
     * 
     * Copies n bytes into data and update current position.
     * 
     * reads beyond the end of the buffer (i.e. if position() + n > size()) result
     * in an invalid_argument exception.
     */
    void read(char * data, size_t s){
        check_for_read(s);
        memcpy(data, data_ + position_, s);
        position_ += s;
    }
    
    template<typename T>
    void read_bitwise(T & t){
        read(reinterpret_cast<char*>(&t), sizeof(T));
    }
    
private:
    size_t position_, size_, reserved_, cs_;
    char * data_;
    
    // re-allocate to at least new_min_reserved bytes (actual allocation size rounds up to nearest multiple of chuck size)
    void reallocate(size_t new_min_reserved);
    void fail_check_for_read(size_t) const;
    void fail_seek(size_t) const;
    void fail_seek_resize(size_t) const;
};


// want to have operator<< and operator>> for buffer and some primitive types.
// Can't do that as template, as using write_bitwise might be semantically wrong ...
#define IMPLEMENT_BUFFER_POD(T) inline Buffer & operator<<(Buffer & out, const T & t){ out.write_bitwise(t);  return out; } \
   inline Buffer & operator>>(Buffer & in, T & t){ in.read_bitwise(t);  return in; }
   
IMPLEMENT_BUFFER_POD(float)
IMPLEMENT_BUFFER_POD(double)
IMPLEMENT_BUFFER_POD(uint64_t)
IMPLEMENT_BUFFER_POD(uint32_t)
IMPLEMENT_BUFFER_POD(uint16_t)
IMPLEMENT_BUFFER_POD(uint8_t)

IMPLEMENT_BUFFER_POD(int64_t)
IMPLEMENT_BUFFER_POD(int32_t)
IMPLEMENT_BUFFER_POD(int16_t)
IMPLEMENT_BUFFER_POD(int8_t)
   
#undef IMPLEMENT_BUFFER_POD


Buffer & operator<<(Buffer & out, const std::string & s);
Buffer & operator>>(Buffer & in, std::string & s);


}


#endif
