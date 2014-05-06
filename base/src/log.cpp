#include "log.hpp"

#include <iostream>
#include <sstream>
#include <map>
#include <memory>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iomanip>
#include <sys/mman.h>
#include <limits.h>
#include <string.h>
#include <set>

#include <boost/lexical_cast.hpp>

using namespace std;

namespace{
    string red(const string & msg){
        return "\033[0;31m" + msg + "\033[0m";
    }
}

void print(ostream & out, loglevel s, bool with_color){
    switch(s){
        case loglevel::debug:   out << "[DEBUG]  "; break;
        case loglevel::info:    out << "[INFO]   "; break;
        case loglevel::warning: out << "[WARNING]"; break;
        case loglevel::error:
            if(with_color)
                out << red("[ERROR]  ");
            else
                out << "[ERROR]  ";
            break;
    }
}

namespace {
    
void split(const std::string & s, char delim, std::vector<std::string> & elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

}

std::shared_ptr<LogSink> LogSink::get_sink(const std::string & outfile){
    // note that sinks are usually owned by the Loggers they use them;
    // we do not need to keep around sinks that are not used by any logger.
    // Therefore, we keep weak_ptrs here, but return them as shared_ptr
    static std::map<std::string, std::weak_ptr<LogSink> > filename_to_sink; // uses full path names as key (!) and "" for the stdout sink.
    
    if(outfile.empty() || outfile == "-"){
        auto it = filename_to_sink.find("");
        if(it != filename_to_sink.end() && !it->second.expired()){
            return it->second.lock();
        }
        else{
            std::shared_ptr<LogSink> result(new LogSinkOstream(cout));
            filename_to_sink[""] = result;
            return result;
        }
    }
    // create a file: ...
    std::vector<string> tokens;
    split(outfile, ';', tokens);
    // in case the filename is relative to the current directory, make sure to tell that realpath:
    if(tokens[0].find('/') == string::npos){
        tokens[0] = "./" + tokens[0];
    }
    unique_ptr<char[]> path(new char[PATH_MAX]);
    char * res = realpath(tokens[0].c_str(), path.get());
    if(res == 0){
        int errorcode = errno;
        cerr << "Could not create LogSink for outfile '" << outfile << "': error during path resolution: " << strerror(errorcode) << endl;
    }
    auto it = filename_to_sink.find(path.get());
    if(it != filename_to_sink.end() && !it->second.expired()){
        return it->second.lock();
    }
    else{
        int maxsize = 5 * (1 << 20);
        int circular = 1;
        if(tokens.size() > 1){
            maxsize = boost::lexical_cast<int>(tokens[1]);
        }
        if(tokens.size() > 2){
            circular = boost::lexical_cast<int>(tokens[2]);
        }
        std::shared_ptr<LogSink> result(new LogFile(path.get(), maxsize, circular));
        filename_to_sink[path.get()] = result;
        return result;
    }
}


void LogSinkOstream::append(const LogMessage & m){
    if(out.good()){
        out << m.message << flush;
    }
}

LogSinkOstream::LogSinkOstream(std::ostream & out_): out(out_){
    term = false;
    if(&out == &cout){
        term = isatty(1);
    }
}

LogFile::LogFile(const char * fname, size_t maxsize, int circular_): filename(fname), msize(maxsize), circular(circular_), current_file(-1), maddr(0), dropped(0) {
    setup_new_file();
}

void LogFile::handle_fork(int new_pid){
    //cout << "handle fork, new pid = " << new_pid << endl;
    stringstream fname;
    fname << filename << ".p" << new_pid;
    filename = fname.str();
    // unmap current region:
    if(maddr){
        int res = munmap(maddr, msize);
        maddr = 0;
        if(res < 0){
            cerr << "LogFile: munmap failed: " << strerror(errno) << "; ignoring that." << endl;
        }
    }
    // start new file with different name containing ".p<pid>".
    current_file = -1;
    setup_new_file();
}

std::string LogFile::get_filename(int ifile) const{
    stringstream ss;
    ss << filename;
    if(ifile > 0){
        ss << "." << ifile;
    }
    return ss.str();
}

void LogFile::close_file(const string & filename){
    int res = truncate(filename.c_str(), mpos);
    if(res < 0){
        cerr << "LogFile: could not truncate log file '" << filename << "'; ignoring that." << endl;
    }
    res = munmap(maddr, msize);
    if(res < 0){
        cerr << "LogFile: munmap failed: " << strerror(errno) << "; ignoring that." << endl;
    }
}


LogFile::~LogFile(){
    if(current_file >= 0){
        close_file(get_filename(current_file));
    }
    if(dropped){
        cerr << "~LogFile('" << filename << "') warning: dropped " << dropped << " log messages due to previous errors" << endl;
    }
}
 
void LogFile::setup_new_file(){
    string old_filename;
    if(current_file < 0){
        //initial setup:
        current_file = 0;
    }
    else{
        // we already opened some file ....
        if(circular < 0){
            old_filename = get_filename(current_file);
            current_file++;
        }
        else if(circular == 0){
            mpos = 0;
            return;
        }
        else{
            old_filename = get_filename(current_file);
            if(circular == current_file){
                current_file = 0;
            }
            else{
                ++current_file;
            }
        }
    }
    // now execute the actions determined:
    if(!old_filename.empty()){
        close_file(old_filename);
    }
    // open new file, mmap it to maddr and reset mpos:
    mpos = 0;
    string new_filename = get_filename(current_file);
    int fd = open(new_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
    if(fd < 0){
        cerr << "LogFile: could not open logfile '" << new_filename << "': " << strerror(errno) << "; LogFile enters null state." << endl;
        maddr = 0;
        return;
    }
    int res = ftruncate(fd, msize);
    if(res < 0){
        cerr << "LogFile: could not ftruncate logfile '" << new_filename << "' to desired length: " << strerror(errno) << "; LogFile enters null state." << endl;
        maddr = 0;
        return;
    }
    maddr = reinterpret_cast<char*>(mmap(NULL, msize, PROT_WRITE, MAP_SHARED, fd, 0));
    if(maddr == reinterpret_cast<char*>(MAP_FAILED)){
        cerr << "LogFile: mmap failed for logfile '" << new_filename << "': " << strerror(errno) << "; LogFile enters null state." << endl;
        close(fd);
        maddr = 0;
        return;
    }
    res = close(fd);
    if(res < 0){
        cerr << "LogFile: closing fd for logfile '" << new_filename << "' after mmap failed: " << strerror(errno) << "; LogFile enters null state." << endl;
        maddr = 0;
        return;
    }
}


void LogFile::append(const LogMessage & m){
    if(maddr==0 || m.message.size() > msize){
        ++dropped;
        return;
    }
    if(mpos + m.message.size() > msize){
        setup_new_file();
        if(maddr==0){
            ++dropped;
            return;
        }
    }    
    memcpy(maddr + mpos, m.message.data(), m.message.size());
    mpos += m.message.size();
}


// Logger
int Logger::init_pid(getpid());

std::vector<LoggerConfiguration> & Logger::configurations(){
    static std::vector<LoggerConfiguration> configs;
    return configs;
}

void Logger::configure(const std::vector<LoggerConfiguration> & conf){
    configurations() = conf;
    auto & loggers = all_loggers();
    for(auto & l : loggers){
        l.second.apply_configuration();
    }
    
}

std::map<std::string, Logger> & Logger::all_loggers(){
    static map<string, Logger> loggers;
    return loggers;
}

Logger & Logger::get(const string & name){
    auto & loggers = all_loggers();
    auto it = loggers.find(name);
    if(it!=loggers.end()) return it->second;
    else{
        auto it = loggers.insert(make_pair(name, Logger(name)));
        Logger & result = it.first->second;
        return result;
    }
}

void Logger::apply_configuration(){
    const auto & configs  = configurations();
    std::string threshold, outfile;
    for(auto & conf : configs){
        // the config applies if its logger_name_prefix is empty or is identical to our full logger name or our name starts with (prefix + ".").
        if(!conf.logger_name_prefix.empty() && conf.logger_name_prefix != logger_name && logger_name.find(conf.logger_name_prefix + ".") != 0) continue;
        if(conf.command == LoggerConfiguration::set_threshold) threshold = conf.value;
        else if(conf.command == LoggerConfiguration::set_outfile) outfile = conf.value;
    }
    // if not configured, use the default from the environment:
    if(threshold.empty()){
        const char * ll = getenv("DC_LOGLEVEL");
        if(!ll) threshold = "WARNING";
        else threshold = ll;
    }
    if(threshold == "DEBUG") l_threshold = loglevel::debug;
    else if(threshold == "INFO") l_threshold = loglevel::info;
    else if(threshold == "WARNING") l_threshold = loglevel::warning;
    else if(threshold == "ERROR") l_threshold = loglevel::error;
    else {
        cerr << "Logger: could not parse threshold '" << threshold << "', defaulting to warning level.";
        l_threshold = loglevel::warning;
    }
    
    if(outfile.empty()){
        const char * of = getenv("DC_LOGFILE");
        if(!of) outfile = "-";
        else {
            outfile = of;
        }
    }
    //cout << logger_name << " outfile: " << outfile << "; threshold: " << (int)l_threshold << endl;
    sink = LogSink::get_sink(outfile); // note: can be the same as before ...
}

void Logger::log(loglevel l, const LogMessage & m){
    if(l < l_threshold || !sink) return;
    int pid = getpid();
    if(pid != init_pid){
        sink->handle_fork(pid);
        init_pid = pid;
    }
    int index = next_index++;
    timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    stringstream ss;
    print(ss, l, sink->is_terminal());
    ss << "@" << pid << "; " << logger_name << "[" << index << "] "
       << t.tv_sec << "." << setw(3) << setfill('0') << t.tv_nsec / 1000000 << ": " << m.message << "\n";
    sink->append(LogMessage(ss.str()));
}
