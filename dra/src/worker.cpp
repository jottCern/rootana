#include "worker.hpp"
#include "ra/include/config.hpp"
#include "ra/include/analysis.hpp"
#include "ra/include/controller.hpp"
#include "stategraph.hpp"
#include "ra/include/root-utils.hpp"

#include "TFile.h"
#include "TTree.h"

using namespace dra;
using namespace ra;
using namespace dc;
using namespace std;

Worker::Worker(): wm(get_stategraph()), logger(Logger::get("dra.Worker")), iworker(-1) {}

// note: define it here and not in the header file to make sure the definition of the classes is available
Worker::~Worker(){}

std::unique_ptr<dc::Message> Worker::configure(const Configure & conf){
    LOG_INFO("configure called; this is worker " << conf.iworker << "; config file=" << conf.cfgfile);
    iworker = conf.iworker;
    controller.reset();
    config.reset();
    config.reset(new s_config(conf.cfgfile));
    config->logger.apply(config->options.output_dir);
    controller.reset(new AnalysisController(*config, true));
    return unique_ptr<Message>();
}

std::string Worker::get_outfilename(size_t idataset, int iw){
    stringstream outfilename;
    outfilename << config->options.output_dir << "/unmerged-" << config->datasets[idataset].name << "-" << iw << ".root";
    return outfilename.str();
}

bool Worker::stopped_successfully() const{
    return wm.state() == wm.get_graph().get_state("stop");
}

void Worker::check_filenames_hash(size_t master_hash) const {
    const s_dataset & dataset = controller->current_dataset();
    if(master_hash != dataset.filenames_hash){
        LOG_ERROR("hash mismatch; Master and this Worker do not agree on input files: worker hash: "
                   << dataset.filenames_hash << "; master hash: " << master_hash << ". " << dataset.files.size() << " worker files follow.");
        for(const auto & f : dataset.files){
            LOG_ERROR(f.path);
        }
        throw runtime_error("hash mismatch (see error log for details).");
    }
}

std::unique_ptr<dc::Message> Worker::process(const Process & p){
    assert(config);
    
    // init dataset:
    controller->start_dataset(p.idataset, get_outfilename(p.idataset, iworker));
    check_filenames_hash(p.files_hash);
    
    // init input file:
    controller->start_file(p.ifile);
    
    // process:
    AnalysisController::ProcessStatistics stat;
    controller->process(p.first, p.last, &stat);
    
    // prepare result:
    unique_ptr<ProcessResponse> pr(new ProcessResponse());
    pr->file_nevents = controller->get_file_size();
    pr->nbytes = stat.nbytes_read;
    pr->realtime = pr->cputime = 0.0f; // TODO: report this ...
    return move(pr);
}

std::unique_ptr<dc::Message> Worker::close(const Close &){
    LOG_INFO("closing file for current dataset");
    controller->start_dataset(-1, "");
    return unique_ptr<dc::Message>();
}

std::unique_ptr<dc::Message> Worker::merge(const Merge & m){
    LOG_INFO("iworker = " << iworker << ": merge entered with other iworker1=" << m.iworker1 << "; iworker2 = " << m.iworker2);
    assert(m.iworker1 != m.iworker2);
    string file1 = get_outfilename(m.idataset, m.iworker1);
    string file2 = get_outfilename(m.idataset, m.iworker2);
    ra::merge_rootfiles(file1, file2);
    if(!config->options.keep_unmerged){
        int res = unlink(file2.c_str());
        if(res < 0){
            LOG_ERRNO("unlinking this file after merging: '" << file2 << "'; ignoring this error");
        }
    }
    return unique_ptr<dc::Message>(new Merge(m));
}


std::unique_ptr<dc::Message> Worker::stop(const Stop &){
    LOG_INFO("stop");
    // nothing to do: files are closed in close or merge
    return unique_ptr<dc::Message>();
}

namespace ph = std::placeholders;

void Worker::setup(unique_ptr<Channel> c){
    const StateGraph & g = wm.get_graph();
    auto s_start = g.get_state("start");
    auto s_configure = g.get_state("configure");
    auto s_process = g.get_state("process");
    auto s_close = g.get_state("close");
    auto s_merge = g.get_state("merge");
    //auto s_stop = g.get_state("stop");
    
    wm.connect<Configure>(s_start, bind(&Worker::configure, this, ph::_1));
    wm.connect<Process>(s_configure, bind(&Worker::process, this, ph::_1));
    wm.connect<Process>(s_process, bind(&Worker::process, this, ph::_1));
    wm.connect<Process>(s_merge, bind(&Worker::process, this, ph::_1));
    wm.connect<Process>(s_close, bind(&Worker::process, this, ph::_1));
    wm.connect<Close>(s_process, bind(&Worker::close, this, ph::_1));
    wm.connect<Merge>(s_close, bind(&Worker::merge, this, ph::_1));
    wm.connect<Merge>(s_merge, bind(&Worker::merge, this, ph::_1));
    
    wm.connect<Stop>(s_close, bind(&Worker::stop, this, ph::_1));
    wm.connect<Stop>(s_merge, bind(&Worker::stop, this, ph::_1));
    wm.connect<Stop>(s_start, bind(&Worker::stop, this,ph:: _1));
    wm.connect<Stop>(s_configure, bind(&Worker::stop, this, ph::_1));
    
    wm.start(move(c));
}
