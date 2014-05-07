#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <map>
#include <stdint.h>
#include <memory>
#include <typeinfo>
#include <stdexcept>

#include <boost/ptr_container/ptr_map.hpp>
#include "buffer.hpp"
#include "base/include/registry.hpp"

namespace dc{

/** Base class for all messages sent over Channels.
 * 
 * For implementing your own Messages:
 *  * derive from Message; the Derived class must be default-constructible
 *  * implement the read_data and write_data methods. If you have more than one level of inheritance,
 *    you would usually call read_data/write_data of the base class [it is not necessary to call Message::read_data/write_data as there is nothing to read/write].
 *  * in the cpp file, call DEFINE_MESSAGE(Der, "Der") at global scope. The second argument is some unique identifier up to 10 alpha-numeric characters
 * 
 * Note that the message serialization system is meant to be used for simple and small (=fitting easily into RAM)
 * messages consisting of some (non-pointer) data elements and no polymorphism apart from other Messages.
 * It does *not* handle the complex cases of circular references and more support for polymorphism beyond
 * writing other Messages with Message::write.
 * If you need to serialize such structures, use Boost.Serialization to fill the buffer in your derived class.
 * 
 * A polymorphic Message m sent over the network:
 *  1. on sender side, the code calls out << m.
 *     - this asks the MessageTypeRegistry for the 64bit type code of the dynamic type of m. This is
 *       put as a header as the first 8 bytes to the out buffer.
 *     - after the type header, write_data is called, whcih takes care of writing all data to the buffer
 *  2. This buffer is transmitted on the network. Note that the wire format for the Buffer is (64 bits size) + (buffer contents), and the buffer
 *      itselfs starts with the (64 bit) typecode.
 *  3. On the receiving side, the full buffer is read. Using the type code, the MessageRegistry is asked to construct an empty instance
 *     of the right type, based on that type code. (At this point, we need the derived class to be default-contructible).
 *  4. On the constructed message, m.read_data is called.
 * 
 * As user of Messages and Channels, you usually do not have to interact with the MessageRegistry other than through the macro
 * DEFINE_MESSAGE which takes care of registering your class at the startup of the program.
 * 
 * For a Message & m and Buffer out, Please not the subtle but important difference between
 * 
 * \code
 * m.write_data(out);
 * \endcode
 * and
 * \code
 * out << m;
 * \endcode
 * 
 * The first one does not write type information, it just writes 'data', as the method name suggests. The second line of code first
 * writes the type code of the actual type of m, followed by m.write_data(out).
 * 
 * To read a Message from a Buffer in where you know the concrete type of the Message and which was written with write_data, use
 * \code
 * m.read_data(in);
 * \endcode
 * 
 * If, however, a message has been written with type infomation, use unique_ &lt; Message &gt; m to read from the buffer:
 * \code
 *  in >> m;
 * \endcode
 */
class Message{
public:
    
    /** write data member of this class to the out Buffer
     * 
     * Note that this only writes the data of this instance, no type information.
     */
    virtual void write_data(Buffer & out) const = 0;
    
    /// read data members of this class from the in Buffer; has to agree with write_data.
    virtual void read_data(Buffer & in) = 0;
    
    virtual ~Message();
};


// convert a string name into a uint64_t. Works only for names consisting of
//  up to 10 alphanumeric characters and underscores and colons; so only the 64 characters
//  A-Z, a-z, 0-9, '_', and ':' are allowed. This enables to encode a single character in 6 bits
// and a 10-character string in 60 bits; the 4 most significant bits are reserved for special typecodes.
uint64_t string_to_uint64(const char * str);

//extern uint64_t null_typecode;

// write / read messages with type information
Buffer & operator<<(Buffer & out, const Message & m);
Buffer & operator<<(Buffer & out, const std::unique_ptr<Message> & m);
Buffer & operator>>(Buffer & in, std::unique_ptr<Message> & m);

typedef Registry<Message, uint64_t> MessageTypeRegistry;

/// Register a class T deriving from Message in the MessageRegistry using the provided name
# define concat1(a, b) a##b
# define concat2(a, b) concat1(a, b)
#define REGISTER_MESSAGE(T, name) namespace { int concat2(dummy, __LINE__) = ::dc::MessageTypeRegistry::register_< T >( ::dc::string_to_uint64(name)); }

}

#endif
