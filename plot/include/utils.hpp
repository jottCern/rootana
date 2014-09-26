#ifndef PLOT_UTILS_HPP
#define PLOT_UTILS_HPP

#include "ra/include/identifier.hpp"
#include "TFile.h"
#include "TH1.h"

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/optional.hpp>

typedef std::string process_type;
typedef std::string selection_type;
typedef std::string hname_type;


inline std::string nameof(const std::string & s){
    return s;
}

inline std::string nameof(const ra::identifier & i){
    return i.name();
}

struct Histogram {
    std::unique_ptr<TH1> histo;// the actual root histogram
    process_type process;   // process name
    selection_type selection; // selection (path in input rootfile)
    hname_type hname;     // histogram name as in the input rootfile (without path)
    std::string legend;       // legend to use in root plotting
    std::string latex_legend; // legend to use for latex
    
    std::map<std::string, std::string> options; //user-defined additional tags for controlling the plotting. Commonly used tags are:
       // "xmin", "xmax": minimum and maximum to draw
       // "xtext", "ytext": alternative labels for the x and y axes
       // (... see draw_histos in utils.cpp for a complete list)
};

void draw_histos(const std::vector<Histogram> & histos, const std::string & filename);

void create_dir(const std::string & filename);

// use T = string
template<typename T>
void get_names_of_type(std::vector<T> & result, TDirectory * dir, const char * type, bool recursive = false, const std::string & prefix = "");

// abstract class of a source of histograms for one process (like ttbar, wjets).
// histograms are organized in two levels: a source has a list of selections, and there is a list of histograms for each selection.
class ProcessHistograms {
public:
    virtual process_type get_process() = 0; // unique name for this process; data should be called 'data' or 'DATA'
    virtual std::vector<selection_type> get_selections() = 0; // the directories (corresponding to selections)
    virtual std::vector<hname_type> get_hnames(const selection_type & selection) = 0; // the histogram names for this selection
    virtual Histogram get_histogram(const selection_type & selection, const hname_type & hname) = 0; // get a copy of the histogram named
    
    virtual ~ProcessHistograms();
};


// read histograms from a root file. Directories correspond to the selections.
// It is possible to specify multiple filenames, glob expressions (or even multiple glob expressions) to define
// which root files to use. The histograms of all files will be added.
class ProcessHistogramsTFile: public ProcessHistograms {
public:
    ProcessHistogramsTFile(const std::string & filename, const process_type & process);
    ProcessHistogramsTFile(const std::initializer_list<std::string> & filenames, const process_type & process);
    
    virtual std::vector<selection_type> get_selections();
    virtual std::vector<hname_type> get_hnames(const selection_type & selection);
    virtual Histogram get_histogram(const selection_type & selection, const hname_type & hname);
    virtual process_type get_process(){
        return process_;
    }
    
    virtual ~ProcessHistogramsTFile();
    
private:
    void init_files(const std::initializer_list<std::string> & filenames);
    
    process_type process_;
    std::vector<TFile*> files;
};


class Formatters;
class Formatters {
public:
    template<typename T>
    struct Adder{
        Formatters & f;
        
        Adder(Formatters & f_): f(f_){}
        
        template<typename... Targs>
        Adder & operator()(const std::string & histos, Targs&&... args){
            f.add(histos, T(std::forward<Targs>(args)...));
            return *this;
        }
    };
    
    
    Formatters();
    
    typedef std::function<void (Histogram &)> formatter_type;
    
    void add(const std::string & histos, const formatter_type & formatter);
    
    template<typename T, typename... Targs>
    Adder<T> add(const std::string & histos, Targs&&... args){
        add(histos, T(std::forward<Targs>(args)...));
        return Adder<T>(*this);
    }
    
    void operator()(Histogram & h) const;
    
private:
    struct fspec {
        enum e_matchmode { mm_any, mm_full, mm_prefix, mm_suffix };
        
        process_type proc;
        selection_type sel;
        
        e_matchmode hname_mm; // mm_any = match any, mm_full = use hname_full equality; mm_prefix = match hname_substring as prefix; mm_suffix = match hname_substring as suffix
        std::string hname_substr;
        hname_type hname_full;
        
        formatter_type formatter;
        
        fspec(const process_type & proc_, const selection_type & sel_, e_matchmode hname_mode_, const std::string & hname_, const formatter_type & formatter_):
            proc(proc_), sel(sel_), hname_mm(hname_mode_), hname_substr(hname_), hname_full(hname_), formatter(formatter_){}
    };
    
    process_type all_processes;
    selection_type all_selections;
    
    std::vector<fspec> formatters;
};


class SetLegends {
public:
    virtual void operator()(Histogram & h){
        h.legend = legend;
        if(latex_legend.empty()){
            h.latex_legend = nameof(h.process);
        }
        else{
            h.latex_legend = latex_legend;
        }
    }
    
    explicit SetLegends(const std::string & legend_, const std::string & latex_legend_ = ""): legend(legend_), latex_legend(latex_legend_){
    }
private:
    std::string legend, latex_legend;
};

class SetLineColor {
public:
    void operator()(Histogram & h){
        h.histo->SetLineColor(col);
    }
    
    explicit SetLineColor(int col_): col(col_){}
    
private:
    int col;
};

class SetLineWidth {
public:
    void operator()(Histogram & h){
        h.histo->SetLineWidth(w);
    }
    
    explicit SetLineWidth(float w_): w(w_){}
    
private:
    float w;
};

class SetLineStyle {
public:
    void operator()(Histogram & h){
        h.histo->SetLineStyle(s);
    }
    
    explicit SetLineStyle(int s_): s(s_){}
    
private:
    int s;
};

class Scale {
public:
    void operator()(Histogram & h){
        if(h.process != data){
            h.histo->Scale(factor);
        }
    }
    
    explicit Scale(double factor_): factor(factor_), data("data"){}
    
private:
    double factor;
    process_type data;
};

class SetFillColor {
public:
    void operator()(Histogram & h){
        h.histo->SetFillColor(col);
    }
    
    explicit SetFillColor(int col_): col(col_){}
    
private:
    int col;
};

class RebinRange {
public:
    void operator()(Histogram & h);
    RebinRange(int nbins_, double xmin_, double xmax_): nbins(nbins_), xmin(xmin_), xmax(xmax_){}
    
private:
    int nbins;
    double xmin, xmax;
};

class RebinFactor {
public:
    void operator()(Histogram & h){
        h.histo->Rebin(factor);
    }
    
    explicit RebinFactor(int factor_): factor(factor_){}
    
private:
    int factor;
};

class RebinVariable {
public:
    void operator()(Histogram & h);
    
    explicit RebinVariable(int nbins_, const double* bin_arr_): nbins(nbins_), bin_arr(bin_arr_) {}
    
private:
    int nbins;
    const double* bin_arr;
    std::string new_name;
};

class SetOption{
public:
    SetOption(const std::string & key_, const std::string & value_): key(key_), value(value_){}
    void operator()(Histogram & h){
        h.options[key] = value;
    }
private:
    std::string key, value;
};


class Plotter {
public:
    Plotter(const std::string & outdir, const std::vector<std::shared_ptr<ProcessHistograms> > & histograms, const Formatters & formatters);
    
    // make stackplots overlaying data with background stack. The processes to stack are given in
    // processes_to_stack (in the order to stack them)
    // filename_suffix is a string to append to the filename, in case you want to call stackplots multiple times with different options
    // 
    // relevant options:
    // * "add_nonstacked_to_stack": if set, the 'stackplots' method will add the non-stacked histograms to the stacked histogram,
    //    instead of the default which does not do that.
    void stackplots(const std::initializer_list<process_type> & processes_to_stack, const std::string & filenamesuffix = "");
    
    // draw normalized plots of different processes; useful to compare the shape of a variable
    // each histogram in the input will lead to one output plot
    void shapeplots(const std::initializer_list<process_type> & processes_to_compare, const std::string & filenamesuffix = "");
    
    // make selection comparison plots by plotting different selections in the same histogram
    void selcomp_plots(const std::initializer_list<selection_type> & selections_to_compare, const std::initializer_list<hname_type> & plots_to_compare,
                       const std::string & outputname);
    
    // set global options (per-histogram options are set by the filters).
    // For valid options, see the *plots methods
    void set_option(const std::string & name, const std::string & value){
        options[name] = value;
    }
    
    // set the current histogram filter to f. For each histogram in the input, f(process, selection, hname) will be called and the return value decides
    // whether the histogram is actually used
    typedef std::function<bool (const process_type &, const selection_type &, const hname_type &)> filter_function_type;
    void set_histogram_filter(const filter_function_type & f){
        filter = f;
    }
    
    // print the integral (including underflow and overflow) of the given histogram name of all processes into the file of name filename (relative to outdir
    // given in the constructor).
    // if append is true, this will be appended to the file, otherwise the file is overwritten.
    void print_integrals(const selection_type & selection, const hname_type & hname, const std::string & filename, const std::string & title, bool append = true);
    
    // make a latex cutflow table:
    void cutflow(const hname_type & cutflow_hname, const std::string & outname);
private:
    std::string outdir;
    std::vector<std::shared_ptr<ProcessHistograms> > histos;
    std::map<std::string, std::string> options;
    boost::optional<filter_function_type> filter;
    const Formatters & formatters;
    
    // get the formatted histograms for all processes from hsources; identified by seleciton and hname
    std::vector<Histogram> get_formatted_histograms(const std::vector<std::shared_ptr<ProcessHistograms> > & hsources, const selection_type & selection,
                                                    const hname_type & hname);
    
    std::vector<Histogram> get_selection_histogram(const std::shared_ptr<ProcessHistograms> & hsource,
                                                   const std::initializer_list<selection_type> & selections_to_compare, const hname_type & hname);
};



// convenience classes to be used for filters
class AndFilter {
public:
    AndFilter(const Plotter::filter_function_type & f1_, const Plotter::filter_function_type & f2_): f1(f1_), f2(f2_){
    }
    
    bool operator()(const process_type & p, const selection_type & s, const hname_type & h) const{
        if(!f1(p, s, h)) return false;
        return f2(p, s, h);
    }
    
private:   
    Plotter::filter_function_type f1, f2;
};


class RegexFilter {
public:
    explicit RegexFilter(const std::string & p_regex, const std::string & s_regex, const std::string & h_regex);
    
    bool operator()(const process_type & p, const selection_type & s, const hname_type & h) const;
    
private:
    boost::regex p_r, s_r, h_r;
};

#endif
