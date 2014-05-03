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
#include <string.h>
#include <set>

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

std::shared_ptr<LogSink> LogSink::get_default_sink(){
    static shared_ptr<LogSink> ds;
    if(!ds){
        char * logfile = getenv("DC_LOGFILE");
        if(logfile){
            ds.reset(new LogFile(logfile));
        }
        else{
            ds.reset(new LogSinkOstream(cout));
        }
    }
    return ds;
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

LogFile::LogFile(const char * fname, size_t maxsize, int circular_): filename(fname), msize(maxsize), circular(circular_), current_file(-1), dropped(0){
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
void Logger::set_sink(const std::shared_ptr<LogSink> & sink_){
    sink = sink_;
}

Logger & Logger::get(const string & name){
    static map<string, Logger> loggers;
    auto it = loggers.find(name);
    if(it!=loggers.end()) return it->second;
    else{
        auto it = loggers.insert(make_pair(name, Logger(name, LogSink::get_default_sink())));
        Logger & result = it.first->second;
        const char * ll = getenv("DC_LOGLEVEL");
        if(!ll) ll = "WARNING";
        if(ll){
            if(string("DEBUG") == ll){
                result.set_threshold(loglevel::debug);
            }
            else if(string("INFO") == ll){
                result.set_threshold(loglevel::info);
            }
            else if(string("WARNING") == ll){
                result.set_threshold(loglevel::warning);
            }
            else if(string("ERROR") == ll){
                result.set_threshold(loglevel::error);
            }
            else{
                // the default:
                result.set_threshold(loglevel::debug);
            }
        }
        return result;
    }
}

void Logger::log(loglevel l, const LogMessage & m){
    if(l < l_threshold || !sink) return;
    int index = next_index++;
    timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    stringstream ss;
    print(ss, l, sink->is_terminal());
    ss << " (" << logger_name << "[" << index << "]) p" << getpid() << " " 
       << t.tv_sec << "." << setw(3) << setfill('0') << t.tv_nsec / 1000000 << ": " << m.message << "\n";
    sink->append(LogMessage(ss.str()));
}
