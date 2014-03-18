#include "config.hpp"
#include "base/include/log.hpp"

#include <glob.h>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

using boost::property_tree::ptree;
using namespace std;
using namespace ra;

namespace{

// note: glob is sorted!
std::vector<std::string> myglob(const std::string& pattern){
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


// 'parse' sframe xml file for all filenames. Note that this is *not* a full-fledged xml parsing;
// rather, it is assumed that each line contains exactly one statement of the form
// <In FileName="..." />
// which is parsed here.
vector<string> parse_sframe_xml(const string & path){
    Logger & logger = Logger::get("config.dataset.sframe_xml");
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

s_options::s_options(const ptree & options_cfg): blocksize(5000), maxevents_hint(-1), output_dir("."), keep_unmerged(false){
    Logger & logger = Logger::get("config.options");
    std::string verb("info");
    for(const auto & cfg : options_cfg){
        if(cfg.first == "blocksize"){
            blocksize = try_cast<int>("options.blocksize", cfg.second.data());
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
        else{
            LOG_THROW("options: unknown setting '" << cfg.first << "'");
        }
    }
}

s_options::s_options(): blocksize(5000), maxevents_hint(-1), output_dir("."), keep_unmerged(false){
}
    
s_dataset::s_file::s_file(const ptree & cfg){
    path = get<string>(cfg, "path");
    nevents = get<int>(cfg, "nevents", -1);
    skip = get<unsigned int>(cfg, "skip", 0);
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
    Logger & logger = Logger::get("config.dataset");
    name = get<string>(cfg, "name");
    treename = get<string>(cfg, "treename");
    for(const auto & it : cfg){
        if(it.first == "file"){
            files.emplace_back(it.second);
        }
        else if(it.first == "file-pattern"){
            vector<string> filenames = myglob(it.second.data());
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
}


s_config::s_config(const std::string & filename) {
    Logger & logger = Logger::get("config");
    ptree cfg;
    LOG_DEBUG("reading file '" << filename << "' into ptree");
    boost::property_tree::read_info(filename.c_str(), cfg);
    LOG_DEBUG("done reading file '" << filename << "' into ptree");
    options = s_options(cfg.get_child("options"));
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

