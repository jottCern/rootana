#ifndef LOG_HPP
#define LOG_HPP

#include <memory>
#include <string>
#include <map>
#include <sstream>

enum class loglevel {
    debug, info, warning, error
};

// basically a non-copyable string:
class LogMessage {
public:
    std::string message;

    LogMessage(const LogMessage &) = delete;
    LogMessage(LogMessage &&) = default;
    LogMessage(std::string msg_): message(move(msg_)){}
};

class LogSink{
public:
    static std::shared_ptr<LogSink> get_default_sink();
    virtual void append(const LogMessage & m) = 0;
    virtual ~LogSink(){}
};

class LogSinkOstream: public LogSink{
public:
    // note: out has to live longer than this.
    LogSinkOstream(std::ostream & out);
    virtual void append(const LogMessage & m);
    
private:
    std::ostream & out;
};

class LogFile: public LogSink {
public:
    
    static const size_t KB = (1 << 10);
    static const size_t MB = (1 << 20);
    static const size_t GB = (1 << 30);
    
    // log messages will be directed to fname (but see circular below), which wil grow up to maxsize.
    //
    // circular governs what happens after maxsize has been reached:
    // * circular < 0: open new log files fname + ".1", fname + ".2", etc.
    // * circular = 0: overwrite the file fname from the beginning if reaching its end
    // * circular > 0: write up to fname + ".<circular>" and then start at the beginning (=fname) again
    //
    // the default circular = 1 writes to fname, then to fname + ".1", then to fname, etc. and
    // is probably appropriate for many use cases, but discards old messages at some point.
    explicit LogFile(const char * fname, size_t maxsize = 20 * MB, int circular = 1);
    
    ~LogFile();
    
    virtual void append(const LogMessage & m);

    
private:
    // close old file (if applicable) and setup
    // new file according to current values of filename, current_file
    void setup_new_file();
    
    std::string get_filename(int ifile) const;
    void close_file(const std::string & filename);
    
    // constructor arguments:
    const std::string filename;
    size_t msize;
    const int circular;
    
    int current_file; // -1 = no file opened yet, 0 = no suffix, 1 = .1, etc.
    
    //mmap region:
    char * maddr;
    size_t mpos;
    
    int dropped; // number of dropped messages
};



class Logger {
public:
    Logger(const Logger &) = delete;
    Logger(Logger &&) = default;

    static Logger & get(const std::string & name);

    bool enabled(loglevel l) const{
        return l_threshold <= l; 
    }

    void set_threshold(loglevel l){
        l_threshold = l;
    }
    
    void set_sink(const std::shared_ptr<LogSink> & sink);
    
    // write the message m to the underlying LogSink; the current
    // hard-wired formatting is:
    // <severity> (p<pid> t<time> logger_name[index]): <message>
    void log(loglevel s, const LogMessage & m);

    // write the message s directly to the log sink; should usually not be needed.
    //void log_raw(const LogMessage & m);

private:
    explicit Logger(const std::string & name, const std::shared_ptr<LogSink> & ls): logger_name(name), l_threshold(loglevel::debug), sink(ls), next_index(0){}
    
    std::string logger_name;
    loglevel l_threshold;
    std::shared_ptr<LogSink> sink;
    int next_index;
};

#define LOG(li, sev, expr) \
  {if(li.enabled(sev)){ \
       ::std::stringstream ss; \
       ss << expr;    \
       li.log(sev, ss.str()); }}


//log error and throw a runtime error with the same message; note: throws even if logger had lower log level
#define LOG_THROW(expr) {::std::stringstream ss; ss << expr << " (throwing runtime_error)"; logger.log(::loglevel::error, ss.str()); throw ::std::runtime_error(ss.str()); }

#define LOG_ERROR(expr)   LOG(static_cast< ::Logger&>(logger), ::loglevel::error, expr)
#define LOG_INFO(expr)    LOG(static_cast< ::Logger&>(logger), ::loglevel::info, expr)
#define LOG_WARNING(expr) LOG(static_cast< ::Logger&>(logger), ::loglevel::warning, expr)
#define LOG_DEBUG(expr)   LOG(static_cast< ::Logger&>(logger), ::loglevel::debug, expr)

#define LOG_ERRNO(expr)  LOG(static_cast< ::Logger&>(logger), ::loglevel::error, "errno=" << errno << " (" << strerror(errno) << ") " << expr)
#define LOG_ERRNO_LEVEL(level, expr)  LOG(logger, level, "errno=" << errno << " (" << strerror(errno) << ") " << expr)

#endif
