#ifndef DRA_WORKER_HPP
#define DRA_WORKER_HPP

#include "messages.hpp"
#include "dc/include/channel.hpp"
#include "dc/include/worker_manager.hpp"
#include "ra/include/fwd.hpp"
#include "ra/include/controller.hpp"
#include "base/include/log.hpp"

#include <memory>

namespace dra {
    
class Worker{
public:
    Worker();
    ~Worker();
    std::unique_ptr<dc::Message> configure(const Configure & conf);
    std::unique_ptr<dc::Message> process(const Process & p);
    std::unique_ptr<dc::Message> close(const Close &);
    std::unique_ptr<dc::Message> merge(const Merge & m);
    std::unique_ptr<dc::Message> stop(const Stop &);
    
    void setup(std::unique_ptr<dc::Channel> c);
    
private:
    std::string get_outfilename(size_t idataset, int iworker);
    void check_filenames_hash(size_t master_hash) const;
    
    dc::WorkerManager wm;
    
    std::shared_ptr<Logger> logger;
    int iworker;
    
    std::unique_ptr<ra::s_config> config;
    std::unique_ptr<ra::AnalysisController> controller;
};

}

#endif
