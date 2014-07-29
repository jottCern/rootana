#include "log.hpp"
#include "utils.hpp"

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <sstream>
#include <map>
#include <memory>
#include <set>
#include <iomanip>

#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

using namespace std;

namespace{
    string red(const string & msg){
        return "\033[0;31m" + msg + "\033[0m";
    }
}

loglevel parse_loglevel(const std::string & threshold, const loglevel & def, bool verbose){
    if(threshold == "DEBUG") return loglevel::debug;
    else if(threshold == "INFO") return loglevel::info;
    else if(threshold == "WARNING") return loglevel::warning;
    else if(threshold == "ERROR") return loglevel::error;
    else if(threshold == "QUIET") return loglevel::quiet_threshold;
    else {
        if(verbose){
            cerr << "Could not parse logger threshold '" << threshold << "'";
        }
        return def;
    }
}

void print(ostream & out, loglevel s, bool with_color){
    switch(s){
        case loglevel::debug:   out << "[DEBUG]  "; break;
        case loglevel::info:    out << "[INFO]   "; break;
        case loglevel::warning: out << "[WARNING]"; break;
        case loglevel::error:
        case loglevel::quiet_threshold:
            if(with_color)
                out << red("[ERROR]  ");
            else
                out << "[ERROR]  ";
            break;
    }
}

void LogSinkOstream::append(const string & m){
    if(out.good()){
        out << m << flush;
    }
}

LogSinkOstream::LogSinkOstream(std::ostream & out_): out(out_){
    term = false;
    if(&out == &cout){
        term = isatty(1);
    }
}

namespace {
    // to handle a fork for LogFile, keep a list of (weak) references to
    // ALL LogFiles. In the fork handler, call the handle_fork for
    // all LogFiles.
    const int pagesize = sysconf(_SC_PAGESIZE);
    
    void logfile_forkhandler(){
        LogController::get().handle_fork();
    }
    
    int install_logfile_forhandler(){
        atfork(&logfile_forkhandler, atfork_child);
        return 0;
    }
    
    int dummy2 = install_logfile_forhandler();
}

LogFile::LogFile(const char * fname, size_t maxsize, int circular_): filename(fname), msize(maxsize),
   circular(circular_), current_file(-1), maddr(0), dropped(0) {
    if(msize == 0 || msize % pagesize > 0){
        msize += pagesize - msize % pagesize;
    }
    setup_new_file();
}

void LogFile::reopen_after_fork(const char * new_fname){
    filename = new_fname;
    // unmap current region, but do not ftruncate (otherwise the parent would get segfault when writing ...).
    if(maddr){
        int res = munmap(maddr, msize);
        maddr = 0;
        if(res < 0){
            cerr << "LogFile: munmap failed: " << strerror(errno) << "; ignoring that." << endl;
        }
    }
    // start the new file:
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

void LogFile::close_current_file(){
    assert(current_file >= 0);
    int res = truncate(get_filename(current_file).c_str(), mpos);
    if(res < 0){
        cerr << "LogFile: could not truncate log file '" << filename << "'; ignoring that." << endl;
    }
    res = munmap(maddr, msize);
    maddr = 0;
    if(res < 0){
        cerr << "LogFile: munmap failed: " << strerror(errno) << "; ignoring that." << endl;
    }
}


LogFile::~LogFile(){
    if(current_file >= 0){
        close_current_file();
    }
    if(dropped){
        cerr << "~LogFile('" << filename << "') warning: dropped " << dropped << " log messages due to previous errors" << endl;
    }
}
 
void LogFile::setup_new_file(){
    if(current_file < 0){
        //initial setup:
        current_file = 0;
    }
    else{
        // we already opened some file.
        if(circular == 0){
            mpos = 0;
            return;
        }
        else{
            close_current_file();
            if(circular > 0){
                current_file = (current_file + 1) % (circular + 1);
            }else{
                ++current_file;
            }
        }
    }
    // open new file, mmap it to maddr and reset mpos:
    mpos = 0;
    maddr = 0;
    string new_filename = get_filename(current_file);
    int fd = open(new_filename.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
    if(fd < 0){
        cerr << "LogFile: could not open logfile '" << new_filename << "': " << strerror(errno) << "; LogFile enters null state." << endl;
        current_file = -1;
        return;
    }
    int res = ftruncate(fd, msize);
    if(res < 0){
        cerr << "LogFile: could not ftruncate logfile '" << new_filename << "' to desired length: " << strerror(errno) << "; LogFile enters null state." << endl;
        current_file = -1;
        return;
    }
    maddr = reinterpret_cast<char*>(mmap(NULL, msize, PROT_WRITE, MAP_SHARED, fd, 0));
    if(maddr == reinterpret_cast<char*>(MAP_FAILED)){
        cerr << "LogFile: mmap failed for logfile '" << new_filename << "': " << strerror(errno) << "; LogFile enters null state." << endl;
        maddr = 0;
        current_file = -1;
        close(fd);
        return;
    }
    res = close(fd);
    if(res < 0){
        cerr << "LogFile: closing fd for logfile '" << new_filename << "' after mmap failed: " << strerror(errno) << "; LogFile enters null state." << endl;
        munmap(maddr, msize);
        current_file = -1;
        maddr = 0;
        return;
    }
}


void LogFile::append(const string & m){
    if(maddr==0 || m.size() > msize){
        ++dropped;
        return;
    }
    if(mpos + m.size() > msize){
        setup_new_file();
        if(maddr==0){
            ++dropped;
            return;
        }
    }    
    memcpy(maddr + mpos, m.data(), m.size());
    mpos += m.size();
}

//LogController
LogController::LogController(): stdout_threshold(loglevel::warning){
    auto * fname = getenv("DC_LOGFILE");
    if(fname){
        set_outfile(fname);
    }
    stdout.reset(new LogSinkOstream(std::cout));
}

LogController & LogController::get(){
    static LogController lc;
    return lc;
}

void LogController::set_outfile(const std::string & outfile_pattern_, size_t maxsize, int circular){
    if(outfile_pattern == outfile_pattern_) return;
    outfile_pattern = outfile_pattern_;
    if(outfile_pattern.empty()){
        outfile.reset();
    }
    else{
        outfile.reset(new LogFile(get_outfile_name().c_str(), maxsize, circular));
    }
}

std::string LogController::get_outfile_name() const{
    string outfile = outfile_pattern;
    auto replace = [](string & s, const string & search, const std::function<string ()> & repl) -> bool {
        auto p = s.find(search);
        if(p!=string::npos){
            s.replace(p, search.size(), repl());
            return true;
        }
        return false;
    };
    
    auto gethostname_string = []() -> string {
        char buf[HOST_NAME_MAX+1];
        gethostname(buf, HOST_NAME_MAX+1); // only error can be ENAMETOOLONG ...
        buf[HOST_NAME_MAX] = '\0'; // .. in which case the hostname is truncated and we take care about null-termination ...
        return buf;
    };
    
    auto getdatetime = []() -> string {
        char buf[40];
        time_t t;
        time(&t);
        struct tm * tmt = localtime(&t);
        if(tmt==0) return "terr0";
        auto res = strftime(buf, 40, "%Y-%m-%d_%H-%M-%S", tmt); // 19 characters + null = 20.
        if(res==0) return "terr1";
        return buf;
    };
    
    replace(outfile, "%p", [](){return boost::lexical_cast<string>(getpid());});
    replace(outfile, "%h", gethostname_string);
    replace(outfile, "%T", getdatetime);
    return outfile;
}
    
void LogController::set_stdout_threshold(const loglevel & l){
    stdout_threshold = l;
}

void LogController::configure(const std::list<LoggerThresholdConfiguration> & conf){
    configuration = conf;
    for(auto & l: all_loggers){
        l.second->apply_configuration(conf);
    }
}

void LogController::log(loglevel l, const std::string & m){
    if(stdout && l >= stdout_threshold){
        stringstream ss;
        print(ss, l, stdout->is_terminal());
        ss << m;
        stdout->append(ss.str());
    }
    if(outfile){
        stringstream ss;
        print(ss, l, false);
        ss << m;
        outfile->append(ss.str());
    }
}

void LogController::handle_fork(){
    if(outfile){
        outfile->reopen_after_fork(get_outfile_name().c_str());
    }
}


// Logger
Logger::Logger(const std::string & name): logger_name(name), l_threshold(loglevel::warning), next_index(0){
    apply_configuration(LogController::get().configuration);
}

std::shared_ptr<Logger> Logger::get(const string & name){
    auto & loggers =  LogController::get().all_loggers;
    auto it = loggers.find(name);
    if(it!=loggers.end()) return it->second;
    else{
        std::shared_ptr<Logger> result(new Logger(name));
        loggers[name] = result;
        return result;
    }
}

bool Logger::remove(const std::string & name){
    auto & loggers =  LogController::get().all_loggers;
    auto it = loggers.find(name);
    if(it == loggers.end()) return true;
    if(it->second.use_count() == 1){
        loggers.erase(it);
        return true;
    }
    return false;
}

void Logger::apply_configuration(const std::list<LoggerThresholdConfiguration> & configs){
    std::string threshold;
    for(auto & conf : configs){
        // the config applies if its logger_name_prefix is empty or is identical to our full logger name or our name starts with (prefix + ".").
        if(!conf.logger_name_prefix.empty() && conf.logger_name_prefix != logger_name && logger_name.find(conf.logger_name_prefix + ".") != 0) continue;
        threshold = conf.threshold;
    }
    // if not configured, use the default from the environment:
    if(threshold.empty()){
        const char * ll = getenv("DC_LOGLEVEL");
        if(!ll) threshold = "WARNING";
        else threshold = ll;
    }
    l_threshold = parse_loglevel(threshold, loglevel::warning);
}

void Logger::log(loglevel l, const string & m){
    if(l < l_threshold) return;
    int pid = getpid();
    int index = next_index++;
    timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    stringstream ss;
    ss << "p" << pid << "; " << logger_name << "[" << index << "] "
       << t.tv_sec << "." << setw(3) << setfill('0') << t.tv_nsec / 1000000 << ": " << m << "\n";
    LogController::get().log(l, ss.str());
}
