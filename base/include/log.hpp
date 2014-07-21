#ifndef BASE_LOG_HPP
#define BASE_LOG_HPP

#include <memory>
#include <string>
#include <map>
#include <sstream>
#include <list>

enum class loglevel {
    debug, info, warning, error,
    quiet_threshold // do not use to issue messages, only for setting the threshold to discard all messages
};

loglevel parse_loglevel(const std::string & level, const loglevel & def, bool verbose = true);



// Logging overview:
// Log messages are issued by classes using the Logger class; the Logger class is the only class clients
// of this library need to know.
//
// The Logger class sends the message (if the level exceeds the logger threshold) to the LogController singleton which in turn sends the
// messages to one or more LogSinks (again according to currently set thresholds).
// As Logging deals with global stuff, there is one singleton class, the LogController, which handles all these global things, such as Logger instances
// and LogSinks (i.e. output of the logging such as files).


/** \brief Base class for the 'sink' of log messages
 * 
 * All a LogSink does is to accept messages which are written to their destination (which depends on the
 * derived class).
 * 
 * LogSinks are usually not used directly, but only indirectly via Loggers.
 */
class LogSink {
public:
    virtual void append(const std::string & m) = 0;
    virtual bool is_terminal() { return false; }
    virtual ~LogSink(){}
};

/** \brief LogSink writing all messages to a std::ostream
 */
class LogSinkOstream: public LogSink{
public:
    // note: out has to live longer than this.
    explicit LogSinkOstream(std::ostream & out);
    virtual void append(const std::string & m);
    virtual bool is_terminal(){ return term; }
    
private:
    std::ostream & out;
    bool term;
};


/** \brief A LogSink writing all log messages line-by-line to a file
 * 
 * To limit the size of the log files, log files only grow up to a maximum size of maxsize specified
 * in the constructor.
 *
 * Once reaching maxsize, the \c circular value governs what happens next:
 *  - \c circular < 0: open new log files fname + ".1", fname + ".2", etc. without end
 *  - \c circular = 0: overwrite the file fname from the beginning if reaching its end
 *  - \c circular > 0: open new log files fname + ".1", etc. up to fname + "." circular and then start at the beginning (=fname) again
 * 
 * The default \c circular = 1 writes to fname, then to fname + ".1", then to fname, etc. This 
 * is probably appropriate for many use cases: While it discards old messages at some point, it limits
 * the size required by log messages to twice the \c maxsize.
 * 
 * Note that \c maxsize will be rounded to the nearest page size.
 * 
 * To make LogFile work across forks, you have to call \c reopen_after_fork with a new file name in the child process.
 */
class LogFile: public LogSink {
public:
    
    static const size_t KB = (1 << 10);
    static const size_t MB = (1 << 20);
    static const size_t GB = (1 << 30);
    
    explicit LogFile(const char * fname, size_t maxsize = 20 * MB, int circular = 1);
    
    void reopen_after_fork(const char * new_fname);
    
    virtual ~LogFile();
    
    virtual void append(const std::string & m);
    
private:
    
    // close old file (if applicable) and setup
    // new file according to current values of filename, current_file
    void setup_new_file();
    
    std::string get_filename(int ifile) const;
    void close_current_file();
    
    // constructor arguments:
    std::string filename;
    size_t msize;
    const int circular;
    
    int current_file; // -1 = no file opened yet, 0 = no suffix, 1 = .1, etc.
    
    //current mmap region:
    char * maddr;
    size_t mpos;
    
    int dropped; // number of dropped messages
};


struct LoggerThresholdConfiguration {
    std::string logger_name_prefix; // to which loggers to apply this configuration: prefix = "a" matches loggers names exactly "a" or starting with "a."
    std::string threshold; // "DEBUG", "INFO", "WARNING", "ERROR", "QUIET"
    
    LoggerThresholdConfiguration(std::string prefix, std::string thresh): logger_name_prefix(move(prefix)), threshold(move(thresh)){}
};


class Logger;
/** \brief Singleton class managing all Loggers and global configuration
 *
 * Loggers call LogController::log if the logging threshold exceeds the configured one. The LogController
 * then writes the LogMessage to the ouput file and to stdout if the message exceeds the stdout_threshold.
 */
class LogController {
public:
    friend class Logger;
    
    LogController(const LogController &) = delete;
    
    static LogController & get();
    
    /** \brief Set the output file for all logging messages
     * 
     * \c outfile_pattern is used to construct the file name. There are special string sequences:
     * "%p" is replaced by the pid, "%T" by the current date and time in the format, "%h" is the hostname.
     * For logging in distributed applications, it is recommended to use all of these to make log filename collissions
     * unlikely.
     * 
     * For empty \c outfile_pattern, the current file is closed and no new file is created.
     *
     * The parameters \c maxsize and \c circular are passed to \c LogFile, see description there.
     */
    void set_outfile(const std::string & outfile_pattern, size_t maxsize = 20 * LogFile::MB, int circular = 1);
    
    void handle_fork();
    
    void set_stdout_threshold(const loglevel & l);
    
    void configure(const std::list<LoggerThresholdConfiguration> & conf);
    
private:
    LogController();
    
    // used by logger:
    void log(loglevel l, const std::string & m);
    std::map<std::string, std::shared_ptr<Logger> > all_loggers;
    std::list<LoggerThresholdConfiguration> configuration;
    
    loglevel stdout_threshold;
    
    std::string outfile_pattern;
    std::string get_outfile_name() const;
    
    std::unique_ptr<LogFile> outfile;
    std::unique_ptr<LogSink> stdout;
};


/** \brief A logger that accepts log messages and forwards the forwarded messages above the current threshold to a LogSink
 * 
 * A logger has a current log threshold (loglevel::warning by default) and a current LogSink to which the formatted
 * messages are sent to.
 * 
 * To get a logger instance, use
 * \code
 * logger = Logger::get("my_logger_name");
 * \endcode
 * 
 * Directly issuing messages can be done with
 * \code
 * logger->log(loglevel::warning, "A warning");
 * \endcode
 * 
 * In many cases, building the complete log message itself is expensive, so check whether the logger
 * is enabled at the level you want to log at:
 * \code
 * if(logger->enabled(loglevel::warning)){
 *    stringstream ss;
 *    // collect some (expensive) information into ss
 *    logger->log(ss.str(loglevel::warning));
 * }
 * \endcode
 * 
 * As this pattern is so common, there are macros LOG (for more control) or the simple one-argument versions LOG_ERROR, LOG_WARNING, LOG_INFO, and LOG_DEBUG which
 * do that kind of check.
 * 
 * About Logger::get: Logger::get is expensive and should only be used rarely; a common pattern is to 'get' a logger once in the constructor
 * and then use it throughout in the member methods.
 * Getting the logger of the same name twice returns the same logger; this allows using the same logger in otherwise unrelated
 * source files or by multiple instances of the same class.
 * 
 * The configuration of the Logging system can be accessed with Logger::configuration. It is a list of LoggerConfiguration rules. For each
 * given logger, all matching rules are applied in the order they appear in the list. The last value for the logger output and threshold
 * then apply. Changing the configuration affects both existing and new loggers(!). Existing loggers will have their LogSink re-set according to the
 * new configuration.
 * 
 * Logger lifetime: As logging might be required very early in the program (e.g. in initilization of static) and very late (e.g. in destruction of statics),
 * the lifetime of Loggers is kept long: a Logger is constructed as soon as it is needed and only destructed as part of the static destruction. They are also kept
 * around in case they are needed again, so doing this:
 * \code
 * auto logger = Logger::get("my_unique_logger_name");
 * ...
 * logger.reset();
 * \endcode
 * 
 * Will NOT actually destroy the logger. This is on purpose, as it allows two unrelated methods to use the same logger without triggering opening
 * the same logfile twice (which would overwrite the old messages). If you really have to delete a logger, use Logger::remove. This will
 * only work if there are no more references to that logger around.
 */
class Logger {
public:
    friend class LogController;
    Logger(const Logger &) = delete;
    Logger(Logger &&) = delete;

    static std::shared_ptr<Logger> get(const std::string & name);
    
    // the "opposite" if get: removes the logger from the global logger list.
    // remove only has an effect if there are no more references to that logger. Otherwise, the
    // request to remove a logger is silently ignored.
    // returns whether the logger was actually removed.
    static bool remove(const std::string & name);
    
    bool enabled(loglevel l) const{
        return l_threshold <= l; 
    }
    
    /** \brief Write a single log message
     * 
     * If the threshold allows, the LogMessage is forwarded to the current sink, after adding 
     * some (hard-wired) formatting to the log message that includes the logger name, a (per-logger)
     * sequence number, a process id, and a time stamp.
     */
    void log(loglevel s, const std::string & m);

private:
    
    // create a logger with the given name, respecting the current configuration.
    // If the configuration does not specify a LogSink, uses the default log sink.
    explicit Logger(const std::string & name);
    
    void apply_configuration(const std::list<LoggerThresholdConfiguration> & configuration);
    
    std::string logger_name;
    loglevel l_threshold;
    int next_index;
};

#define LOG(li, sev, expr) \
  {if(li->enabled(sev)){ \
       ::std::stringstream ss; \
       ss << expr;    \
       li->log(sev, ss.str()); }}


//log as error and throw a runtime error with the same message
#define LOG_THROW(expr) {::std::stringstream ss; ss << expr; logger->log(::loglevel::error, ss.str()); throw ::std::runtime_error(ss.str()); }

#define LOG_ERROR(expr)   LOG(logger, ::loglevel::error, expr)
#define LOG_INFO(expr)    LOG(logger, ::loglevel::info, expr)
#define LOG_WARNING(expr) LOG(logger, ::loglevel::warning, expr)
#define LOG_DEBUG(expr)   LOG(logger, ::loglevel::debug, expr)

#define LOG_ERRNO(expr)  LOG(logger, ::loglevel::error, "errno=" << errno << " (" << strerror(errno) << ") " << expr)
#define LOG_ERRNO_LEVEL(level, expr)  LOG(logger, level, "errno=" << errno << " (" << strerror(errno) << ") " << expr)

#endif
