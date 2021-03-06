#include "config.hpp"
#include "base/include/log.hpp"
#include "base/include/utils.hpp"

#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/utility/in_place_factory.hpp>

#include <set>
#include <iostream>

using boost::property_tree::ptree;
using namespace std;
using namespace ra;


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

s_options::s_options(const ptree & options_cfg): blocksize(5000), maxevents_hint(-1), output_dir("."), keep_unmerged(false), mergemode(mm_master) {
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
            if(!output_dir.empty() && output_dir[output_dir.size()-1]=='/'){
                output_dir = output_dir.substr(0, output_dir.size()-1);
            }
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
        else if(cfg.first == "default_treename"){
            default_treename = cfg.second.data();
        }
        else if(cfg.first == "keep_unmerged"){
            keep_unmerged = try_cast<bool>("options.keep_unmerged", cfg.second.data());
        }
        else if(cfg.first == "mergemode"){
            if(cfg.second.data() == "workers"){
                mergemode = mm_workers;
            }
            else if(cfg.second.data() == "master"){
                mergemode = mm_master;
            }
            else if(cfg.second.data() == "nomerge"){
                mergemode = mm_nomerge;
            }
            else{
                LOG_THROW("unknown mergemode='" << cfg.second.data() << "' (allowed values: 'master', 'workers', 'nomerge')");
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
    treename = ptree_get<string>(cfg, "treename", "");
    for(const auto & it : cfg){
        if(it.first == "file"){
            if(it.second.size()){ // full group
                files.emplace_back(it.second);
            }
            else{ // only pathname string
                files.emplace_back(it.second.data());
            }
        }
        else if(it.first == "file-pattern"){
            vector<string> filenames = glob(it.second.data());
            LOG_INFO("glob pattern '" << it.second.data() << "' matched " << filenames.size() << " files");
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

namespace {

struct set_reset_cwd {
    
    set_reset_cwd(const string & path){
        old_dir = get_current_dir_name();
        int res = chdir(path.c_str());
        if(res < 0){
            int error = errno;
            stringstream ss;
            ss << "Could not chdir to path '" << path << "': " << strerror(error) << endl;
            free(old_dir);
            throw runtime_error(ss.str());
        }
    }
    
    ~set_reset_cwd(){
        int res = chdir(old_dir);
        if(res < 0){
            int error = errno;
            cerr << "Could not change to '" << old_dir << "': " << strerror(error) << endl;
        }
        free(old_dir);
    }
    
    char * old_dir;
};


// current_path is used for error reporting only
void substitute(ptree & cfg, const std::map<std::string, std::string> & variables, const string & current_path = ""){
    for(auto & it : cfg){
        if(it.second.size() == 0){
            string current_value = it.second.data();
            size_t p = 0;
            while((p = current_value.find("${", p))!= string::npos){
                size_t endpos = current_value.find('}', p);
                if(endpos == string::npos){
                    throw runtime_error("Could not substitute variables in malformed " + current_path + "." + it.first + "=" + current_value);
                }
                string varname = current_value.substr(p + 2, endpos - p - 2);
                auto v_it = variables.find(varname);
                if(v_it == variables.end()){
                    throw runtime_error("Did not find variable '" + varname + "' required for " + current_path + "." + it.first + "=" + current_value) ;
                }
                current_value.replace(p, endpos - p + 1, v_it->second);
                // to search for next "${", start searching at the position following the string we just replaced, i.e.
                // at endpos + 1, but corrected for the string length differences of the replaces string:
                p = endpos + 1 + v_it->second.size() - (endpos - p + 1);
            }
            it.second = ptree(current_value);
        }
        else{
            substitute(it.second, variables, current_path + (current_path.empty() ? "" : ".") + it.first);
        }
    }
}

// get all datasets of the top-level configuration cfg
std::vector<s_dataset> get_datasets(const ptree & cfg, const string & default_treename){
    std::vector<s_dataset> result;
    std::set<std::string> dataset_names;
    for(const auto & it : cfg){
        if(it.first != "dataset") continue;
        result.emplace_back(it.second);
        auto res = dataset_names.insert(result.back().name);
        if(!res.second){
            throw runtime_error("duplicate dataset name '" + result.back().name + "'");
        }
        if(result.back().treename.empty()){
            if(default_treename.empty()){
                throw runtime_error("no treename given in dataset '" + result.back().name + "' and no default treename given in options");
            }   
            result.back().treename = default_treename;
        }
    }
    return move(result);
}


// read configuration with boost::property_tree::read_info, but also do the 
// variable substitution.
// Note that changing the directory into the one of the cfg file (for include resolution and
// datasets_from_output resolution) has to be done outside of this method.
void read_config(const string & filename, ptree & cfg){
    //LOG_DEBUG("read_config filename='" << filename << "', cwd='" << get_current_dir_name() << "'");
    boost::property_tree::read_info(filename.c_str(), cfg);
    if(cfg.count("variables") > 0){
        // there might be more than one variables, section, read them all:
        std::map<string, string> variables;
        auto range = cfg.equal_range("variables");
        for(auto it = range.first; it != range.second; ++it){
            for(const auto & var : it->second){
                string varname = var.first;
                string varvalue = var.second.data();
                // do not overwrite: first variable definition wins!
                if(variables.find(varname)==variables.end()){
                    variables[varname] = varvalue;
                }
            }
        }
        substitute(cfg, variables);
    }
}

    
}

s_config::s_config(const std::string & filename) {
    ptree cfg;
    set_reset_cwd setter(dir_name(filename));
    read_config(base_name(filename), cfg);
    ptree empty_ptree;
    auto logger_cfg = cfg.get_child_optional("logger");
    logger = s_logger(logger_cfg ? *logger_cfg : empty_ptree);
    auto options_cfg = cfg.get_child_optional("options");
    options = s_options(options_cfg ? *options_cfg : empty_ptree);
    modules_cfg = cfg.get_child("modules");
    if(cfg.count("input") > 0){
        input_cfg = cfg.get_child("input");
    }
    else{
        input_cfg.add_child("type", ptree("root"));
    }
    if(cfg.count("output") > 0){
        output_cfg = cfg.get_child("output");
    }
    else{
        output_cfg.add_child("type", ptree("root"));
    }
    // read in all 'dataset' and 'dataset_output_from' statements:
    datasets = get_datasets(cfg, options.default_treename);
    if(datasets.empty()){
        throw runtime_error("config: no datasets defined");
    }
}

