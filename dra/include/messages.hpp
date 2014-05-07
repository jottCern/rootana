#ifndef DRA_MESSAGES_HPP
#define DRA_MESSAGES_HPP

#include "dc/include/message.hpp"

// This file defines the Messages required for the problem; see stategraph of how they relate to the overall structure

namespace dra {

// Worker should read the configuration file and set up everything
// to start working (as far as possible).
class Configure: public dc::Message {
public:
    std::string cfgfile;
    int iworker;
    
    virtual void write_data(dc::Buffer & out) const{
        out << cfgfile << iworker;
    }
    
    virtual void read_data(dc::Buffer & in){
        in >> cfgfile >> iworker;
    }
    
};

// Process a certain event region of the currently configured config file.
class Process: public dc::Message {
public:
    unsigned int idataset;
    unsigned int ifile; // index into dataset.filenames
    size_t files_hash;
    size_t first, last; // event range: [first, last) in the file are processed (or fewer, if the file contains fewer events)
    
    virtual void write_data(dc::Buffer & out) const{
        out << idataset << ifile << files_hash << first << last;
    }
    
    virtual void read_data(dc::Buffer & in){
        in >> idataset >> ifile >> files_hash >> first >> last;
    }
    
};

// as response to Process, send how many events the file had. For statistics,
// also say how many bytes have been read and how much (real and cpu) time have been
// consumed to process that part.
class ProcessResponse: public dc::Message {
public:
    size_t file_nevents;
    size_t nbytes;
    float realtime, cputime;
    
    virtual void write_data(dc::Buffer & out) const{
        out << file_nevents << nbytes << realtime << cputime;
    }
    
    virtual void read_data(dc::Buffer & in){
        in >> file_nevents >> nbytes >> realtime >> cputime;
    }
};

// Tell the Worker to close the output file of the current dataset.
class Close: public dc::Message {
public:
    size_t idataset;
    size_t files_hash;
    
    virtual void write_data(dc::Buffer & out) const{
        out << idataset << files_hash;
    }
    
    virtual void read_data(dc::Buffer & in){
        in >> idataset >> files_hash;
    }
};

// Tell the Worker to merge the results with another worker
class Merge: public dc::Message {
public:
    size_t idataset;
    int iworker1;
    int iworker2;
    
    Merge() = default;
    Merge(size_t id, int iw1, int iw2): idataset(id), iworker1(iw1), iworker2(iw2){}
    
    virtual void write_data(dc::Buffer & out) const{
        out << idataset << iworker1 << iworker2;
    }
    
    virtual void read_data(dc::Buffer & in){
        in >> idataset >> iworker1 >> iworker2;
    }
};

class Stop: public dc::Message {
public:
    virtual void write_data(dc::Buffer & out) const{}
    virtual void read_data(dc::Buffer & in){}
};

}

#endif
