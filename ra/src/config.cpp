#include "config.hpp"
#include "base/include/log.hpp"

#include <glob.h>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

using boost::property_tree::ptree;
using namespace std;
using namespace ra;

// note: glob is sorted!
std::vector<std::string> ra::glob(const std::string& pattern){
    glob_t glob_result;
    int res = glob(pattern.c_str(), GLOB_TILDE | GLOB_BRACE | GLOB_ERR | GLOB_MARK, NULL, &glob_result);
    if(res != 0){
        string errmsg;
        if(res == GLOB_NOSPACE) errmsg = "out of memory";
        else if(res == GLOB_ABORTED) errmsg = "I/O error";
        else if(res == GLOB_NOMATCH) errmsg = "no match";
        else errmsg = "unknwon glob return value";
        throw runtime_error("glob error for pattern '" + pattern + "': " + errmsg);
    }
    vector<string> ret;
    ret.reserve(glob_result.gl_pathc);
    for(unsigned int i=0; i<glob_result.gl_pathc; ++i){
        ret.emplace_back(glob_result.gl_pathv[i]);
    }
    globfree(&glob_result);
    return ret;
}

namespace{

// 'parse' sframe xml file for all filenames. Note that this is *not* a full-fledged xml parsing;
// rather, it is assumed that each line contains exactly one statement of the form
// <In FileName="..." />
// which is parsed here.
vector<string> parse_sframe_xml(const string & path){
    auto logger = Logger::get("ra.config.dataset.sframe_xml");
    ifstream in(path);
    if(!in.good()){
        LOG_THROW("error opening xml file '" << path << "'");
    }
    string line;
    int nline = 0;
    vector<string> result;
    while(!in.eof()){
        getline(in, line);
        ++nline;
        if(in.eof() && line.empty()){
            break;
        }
        if(in.fail()){
            LOG_THROW("error reading line from sframe xml file '" << path << "'");
        }
        // ignore empty lines:
        if(line.empty()) continue;
        // consider an error lines with more than one '<'
        size_t p0 = line.find("FileName=\"");
        if(p0==string::npos){
            LOG(logger, loglevel::warning, "Ignoring line " << nline << " in sframe xml file '" << path << "' as there's no 'FileName=\"'");
            continue;
        }
        if(line.find('<', p0)!=string::npos){
            LOG_THROW("At line " << nline << " in sframe xml file '" << path << "': more than one '<'");
        }
        p0 += 10; // skip over FileName="
        size_t p1 = line.find('"', p0);
        if(p1==string::npos){
            LOG_THROW("At line " << nline << " in sframe xml file '" << path << "': no \" found");
        }
        result.emplace_back(line.substr(p0, p1 - p0));
    }
    return result;
}

}

s_options::s_options(const ptree & options_cfg): blocksize(5000), maxevents_hint(-1), output_dir("."), keep_unmerged(false), mergemode(mm_master){
    auto logger = Logger::get("ra.config.options");
    std::string verb("info");
    for(const auto & cfg : options_cfg){
        if(cfg.first == "blocksize"){
            blocksize = try_cast<int>("options.blocksize", cfg.second.data());
            if(blocksize <= 0){
                LOG_THROW("blocksize <= 0 invalid");
            }
        }
        else if(cfg.first == "output_dir"){
            output_dir = cfg.second.data();
        }
        else if(cfg.first == "maxevents_hint"){
            maxevents_hint = try_cast<int>("options.maxevents_hint", cfg.second.data());
        }
        else if(cfg.first == "verbosity"){
            verb = cfg.second.data();
        }
        else if(cfg.first == "library"){
            libraries.emplace_back(cfg.second.data());
        }
        else if(cfg.first == "searchpath"){
            searchpaths.emplace_back(cfg.second.data());
        }
        else if(cfg.first == "keep_unmerged"){
            keep_unmerged = try_cast<bool>("options.keep_unmerged", cfg.second.data());
        }
        else if(cfg.first == "mergemode"){
            if(cfg.second.data() == "workers"){
                mergemode = mm_workers;
            }
            else if(cfg.second.data() != "master"){
                LOG_THROW("unknown mergemode='" << cfg.second.data() << "' (allowed values: 'master', 'workers')");
            }
        }
        else{
            LOG_THROW("options: unknown setting '" << cfg.first << "'");
        }
    }
}

s_options::s_options(): blocksize(5000), maxevents_hint(-1), output_dir("."), keep_unmerged(false){
}
    
s_dataset::s_file::s_file(const ptree & cfg){
    path = ptree_get<string>(cfg, "path");
    nevents = ptree_get<int>(cfg, "nevents", -1);
    skip = ptree_get<unsigned int>(cfg, "skip", 0);
}

s_dataset::s_file::s_file(string s): path(std::move(s)), nevents(-1), skip(0) {}

s_dataset::s_tags::s_tags(const boost::property_tree::ptree & tree){
    size_t n = 0;
    for(auto it : tree){
        keyval[it.first] = it.second.data();
        ++n;
    }
    if(n != keyval.size()) throw runtime_error("key defined twice in dataset tags");
}

s_dataset::s_dataset(const ptree & cfg){
    auto logger = Logger::get("ra.config.dataset");
    name = ptree_get<string>(cfg, "name");
    treename = ptree_get<string>(cfg, "treename");
    for(const auto & it : cfg){
        if(it.first == "file"){
            files.emplace_back(it.second);
        }
        else if(it.first == "file-pattern"){
            vector<string> filenames = glob(it.second.data());
            LOG_INFO("glob pattern '" << it.second.data() << "' matched " << filenames.size() << " files");
            if(filenames.empty()){
                throw runtime_error("file pattern '" + it.second.data() + "' did not match anything");
            }
            for(std::string & f : filenames){
                files.emplace_back(move(f));
            }
        }
        else if(it.first == "sframe-xml-file"){
            vector<string> filenames = parse_sframe_xml(it.second.data());
            LOG_INFO("sframe xml file '" << it.second.data() << "' had " << filenames.size() << " file names");
            for(std::string & f : filenames){
                files.emplace_back(std::move(f));
            }
        }
        else if(it.first == "tags"){
            tags = s_tags(it.second);
        }
        else if(it.first == "name" || it.first == "treename"){
            continue;
        }
        else{
            LOG_THROW("unknown setting " << it.first << " in dataset '" << name << "'");
        }
    }
    if(files.empty()){
        LOG_THROW("no files in dataset '" << name << "'");
    }
    filenames_hash = 0;
    std::hash<string> hasher;
    for(const auto & f : files){
        filenames_hash ^=  hasher(f.path) + 0x9e3779b9 + (filenames_hash << 6) + (filenames_hash >> 2);
    }
}

s_logger::s_logconfig::s_logconfig(const std::string & key, const ptree & cfg): loggername_prefix(key){
    threshold = ptree_get<string>(cfg, "threshold", "");
}

s_logger::s_logger(const ptree & cfg){
    logfile = ptree_get<string>(cfg, "logfile", "log-%T-%h-%p");
    stdout_threshold = ptree_get<string>(cfg, "stdout_threshold", "WARNING");
    for(const auto & it : cfg){
        if(it.first == "logfile" || it.first == "stdout_threshold") continue;
        configs.emplace_back(it.first, it.second);
    }
}

void s_logger::apply(const std::string & base_dir) const{
    // set output file:
    if(!logfile.empty()){
        string outfile = logfile;
        if(outfile[0] != '/'){ // if it is relative, interpret it relative to base_dir:
            outfile = base_dir + '/' + outfile;
        }
        LogController::get().set_outfile(outfile);
    }
    
    // set stdout threshold:
    LogController::get().set_stdout_threshold(parse_loglevel(stdout_threshold, loglevel::warning));
    
    // set logger thresholds:
    std::list<LoggerThresholdConfiguration> logger_configs;
    for(auto const & this_cfg : configs){
        if(!this_cfg.threshold.empty()){
            logger_configs.emplace_back(this_cfg.loggername_prefix, this_cfg.threshold);
        }
    }
    LogController::get().configure(logger_configs);
}

s_config::s_config(const std::string & filename) {
    // note: make an effort to iniliaze logging soon:
    ptree cfg;
    boost::property_tree::read_info(filename.c_str(), cfg);
    if(cfg.count("logger") > 0){
        logger = s_logger(cfg.get_child("logger"));
    }
    options = s_options(cfg.get_child("options"));
    auto logger = Logger::get("ra.config");
    modules_cfg = cfg.get_child("modules");
    for(const auto & it : cfg){
        //if(it.first == "options" or it.first == "modules" or it.first == "input_tree") continue;
        if(it.first != "dataset") continue;
        try{
            datasets.emplace_back(it.second);
        }
        catch(...){
            LOG_ERROR("exception while building dataset " << datasets.size() << "; re-throwing");
            throw;
        }
    }
    if(datasets.empty()){
        LOG_THROW("no datasets defined");
    }
}

