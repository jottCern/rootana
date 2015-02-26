#include "utils.hpp"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cxxabi.h>
#include <list>
#include <glob.h>

#include <system_error>
#include <cassert>
#include <memory>
#include <mutex>

using namespace std;

std::string demangle(const char * typename_){
    int status;
    char * demangled = abi::__cxa_demangle(typename_, 0, 0, &status);
    if(status != 0 or demangled == 0){
        free(demangled);
        return typename_;
    }
    std::string result(demangled);
    free(demangled);
    return result;
}

namespace {

std::list<std::pair<std::function<void ()>, int> > & fork_callbacks(){
    static std::list<std::pair<std::function<void ()>, int> > cbs;
    return cbs;
}
    
}

void atfork(std::function<void()> callback, int when){
    auto & callbacks = fork_callbacks();
    callbacks.emplace_back(move(callback), when);
}

int dofork(){
    int pid = fork();
    auto & callbacks = fork_callbacks();
    for(auto & cb : callbacks){
        if((cb.second & atfork_parent) > 0 && pid > 0){
            cb.first();
        }
        if((cb.second & atfork_child) > 0 && pid == 0){
            cb.first();
        }
    }
    return pid;
}


bool is_directory(const std::string & path){
    struct stat s;
    errno = 0;
    int res = stat(path.c_str(), &s);
    if(res < 0){
        return false;
    }
    else{
        return S_ISDIR(s.st_mode);
    }
}

bool is_regular_file(const std::string & path){
    struct stat s;
    errno = 0;
    int res = stat(path.c_str(), &s);
    if(res < 0){
        return false;
    }
    else{
        return S_ISREG(s.st_mode);
    }
}


std::string realpath(const std::string & path){
    char * result = realpath(path.c_str(), NULL);
    if(result == 0){
        throw std::system_error(errno, std::system_category(), "realpath('" + path + "')");
    }
    std::string sresult(result);
    free(result);
    return sresult;
}


std::string read_file(const std::string & path, int64_t upto_nbytes){
    int fd = open(path.c_str(), O_RDONLY);
    if(fd < 0){
        throw std::system_error(errno, system_category(), "read_file: open('" + path + "')");
    }
    return read_file(fd, upto_nbytes);
}

std::string read_file(int fd, int64_t upto_nbytes){
    if(upto_nbytes < 0){
        struct stat buf;
        int res = fstat(fd, &buf);
        if(res < 0){
            throw std::system_error(errno, system_category(), "read_file: fstat");
        }
        upto_nbytes = buf.st_size;
    }
    if(upto_nbytes == 0) return std::string();
    assert(upto_nbytes > 0);
    std::unique_ptr<char[]> data(new char[upto_nbytes]);
    size_t to_read = upto_nbytes;
    size_t already_read = 0;
    while(to_read > 0){
        auto res = read(fd, data.get() + already_read, to_read);
        if(res < 0){
            int err = errno;
            if(err == EINTR) continue;
            throw std::system_error(err, system_category(), "read_file: read");
        }
        if(res == 0){ // EOF and to_read / upto_nbytes was too large:
            break;
        }
        assert(res > 0);
        assert(static_cast<size_t>(res) <= to_read);
        to_read -= res;
        already_read += res;
    }
    return std::string(data.get(), already_read);
}


void write_file(int fd, const std::string & contents){
    size_t to_write = contents.size();
    size_t written = 0;
    while(to_write > 0){
        auto res = write(fd, contents.data() + written, to_write);
        if(res < 0){
            int err = errno;
            if(err == EINTR){
                continue;
            }
            else{
                throw std::system_error(err, system_category(), "write_file: write");
            }
        }
        else{
            assert(res >= 0);
            assert(static_cast<size_t>(res) < to_write);
            written += res;
            to_write -= res;
        }
    }
}

bool rename_soft(const std::string & old_path, const std::string & new_path){
    int res = link(old_path.c_str(), new_path.c_str());
    if(res < 0){
        int err = errno;
        if(err == EEXIST) return false;
        throw system_error(err, system_category(), "rename_soft('" + old_path + "', '" + new_path + "'): link");
    }
    res = unlink(old_path.c_str());
    if(res < 0){
        throw system_error(errno, system_category(), "rename_soft('" + old_path + "', '" + new_path + "'): unlink");
    }
    return true;
}


std::vector<std::string> glob(const std::string& pattern, bool allow_no_match){
    glob_t glob_result;
    int res = glob(pattern.c_str(), GLOB_TILDE | GLOB_BRACE | GLOB_ERR | GLOB_MARK, NULL, &glob_result);
    std::vector<std::string> ret;
    if(res != 0){
        std::string errmsg;
        if(res == GLOB_NOSPACE) errmsg = "out of memory";
        else if(res == GLOB_ABORTED) errmsg = "I/O error";
        else if(res == GLOB_NOMATCH){
            if(allow_no_match){
                return ret; // empty vector
            }else{
                errmsg = "no match";
            }
        }
        else errmsg = "unknwon glob return value";
        throw std::runtime_error("glob error for pattern '" + pattern + "': " + errmsg);
    }
    ret.reserve(glob_result.gl_pathc);
    for(unsigned int i=0; i<glob_result.gl_pathc; ++i){
        ret.emplace_back(glob_result.gl_pathv[i]);
    }
    globfree(&glob_result);
    return ret;
}

namespace {

// remove the trailing part of the directory, i.e. everything after the last '/', ignoring
// all trailing '/'.
// If no '/' are in path, returns and empty string.
void remove_last_part(std::string & path){
    auto pos = path.find_last_of('/');
    // remove all trailing / (note: not very efficient, but usually should not be a problem as should not happen):
    while(!path.empty() && pos == path.size() - 1){
        path = path.substr(0, path.size() - 1);
        pos = path.find_last_of('/');
    }
    if(pos == string::npos){
        path.clear();
    }
    else{
        path = path.substr(0, pos);
    }
}

}

void mkdir_recursive(const std::string & path_){
    std::list<std::string> paths_to_create;
    std::string path = path_;
    while(!path.empty() && !is_directory(path)){
        paths_to_create.push_front(path);
        remove_last_part(path);
    }
    for(const auto & p : paths_to_create){
        int res = mkdir(p.c_str(), 0700);
        if(res < 0){
            throw system_error(errno, system_category(), "mkdir_recursive('" + path_ + "'): mkdir for '" + p + "'");
        }
    }
}


std::string hostname(){
    // thread-safe once-time initialization:
    static once_flag of;
    static string result;
    string * presult = &result;
    call_once(of, [presult](){
        char buf[256];
        int res = gethostname(buf, 256);
        if(res < 0){
            int err = errno;
            if(err == ENAMETOOLONG){
                buf[255] = '\0';
            }
            else{
                throw system_error(err, system_category(), "hostname: gethostname");
            }
        }
        *presult = buf;
    });
    return result;
}

