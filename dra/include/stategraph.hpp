#ifndef DRA_STATEGRAPH_HPP
#define DRA_STATEGRAPH_HPP

#include "dc/include/state_graph.hpp"

namespace dra {

/** \brief Get the StateGraph describing the distributed root analysis
 * 
 * States (beyond the default start, stop, failed):
 *  configure, process, close, merge
 * 
 * State transitions:
 * start   ->[Configure]  configure   ->[Process]   process   ->[Close]  close    ->[Merge]  merge   ->[Stop] stop
 * configure  ->[Stop] stop
 * close ->[Stop] stop
 * start ->[Stop] stop
 * process ->[Process] process
 * merge ->[Merge]  merge
 * merge ->[Process] process
 * close ->[Process] process
 * 
 * 
 * TODO: later can add     close ->[Configure] configure to allow processing a different job config then in the same job ...
 * 
 * Restrictions:
 *   - "noprocess": contains all transitions with the Process message; active when there are not events left to process.
 *   - "nomerge": contains all transitions with the Merge message; active when there are no unmerged files left.
 * 
 * Responses (not formally part of the stategraph):
 *   - 'process' returns a ProcessResponse
 *   - 'merge' returns a Merge as response
 */
const dc::StateGraph & get_stategraph();

}

#endif
