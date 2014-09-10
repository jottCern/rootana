#ifndef RA_CONTECT_BACKEND_HPP
#define RA_CONTECT_BACKEND_HPP

#include "base/include/registry.hpp"
#include "config.hpp"
#include "context.hpp"

namespace ra {

/** \brief Base class for implementations of InputManager
 *
 * This 'intermediate' abstract class is introduced to give the
 * AnalysisModule only access to the interface of InputManager, while this
 * class here defines the interface required by the AnalysisController / the event
 * loop.
 */
class InputManagerBackend: public InputManager {
public:
    // return the number of events in the file.
    virtual size_t setup_input_file(const std::string & treename, const std::string & filename) = 0;
    
    // populate the event container with the data from event number ievent
    virtual void read_event(size_t ievent) = 0;
    
    // get the number of bytes read since the last time this function was called. Note that
    // this is not returned by read_event to allow for lazy reads.
    virtual size_t nbytes_read() = 0;
    
    virtual ~InputManagerBackend();
    
protected:
    explicit InputManagerBackend(Event & event): InputManager(event){}
};

typedef Registry<ra::InputManagerBackend, std::string, Event &, const ptree &> InputManagerBackendRegistry;
#define REGISTER_INPUT_MANAGER_BACKEND(T, name) namespace { int dummy##T = ::ra::InputManagerBackendRegistry::register_<T>(name); }

}

#endif
