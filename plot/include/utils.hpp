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

struct Histogram {
    std::shared_ptr<TH1> histo;// the actual root histogram
    std::string process;   // process name
    std::string selection; // selection (path in input rootfile)
    std::string hname;     // histogram name as in the input rootfile (without path)
    std::string legend;       // legend to use in root plotting
    std::string latex_legend; // legend to use for latex
    
    std::map<std::string, std::string> options; //user-defined additional tags for controlling the plotting. Commonly used tags are:
       // "xmin", "xmax": minimum and maximum to draw
       // "xtext", "ytext": alternative labels for the x and y axes
       // (... see draw_histos in utils.cpp for a complete list)
       
       
    Histogram(){}
       
    Histogram copy_shallow() const{
        return Histogram(*this);
    }
    
    Histogram copy_deep() const{
        Histogram result(*this);
        result.histo.reset((TH1*)histo->Clone());
        return result;
    }
    
    Histogram(Histogram &&) = default; // moving is allowed
    Histogram & operator=(Histogram &&) = default;
    
    // use copy_* above instead of copy-assign:
    Histogram & operator=(const Histogram &) = delete;
private:
    Histogram(const Histogram &) = default; // corresponds to shallow copy
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
    virtual std::string get_process() = 0; // unique name for this process; data should be called 'data' or 'DATA'
    virtual std::vector<std::string> get_selections() = 0; // the directories (corresponding to selections)
    virtual std::vector<std::string> get_hnames(const std::string & selection) = 0; // the histogram names for this selection
    virtual Histogram get_histogram(const std::string & selection, const std::string & hname) = 0; // get a copy of the histogram named
    
    virtual ~ProcessHistograms();
};


// read histograms from a root file. Directories correspond to the selections.
// It is possible to specify multiple filenames, glob expressions (or even multiple glob expressions) to define
// which root files to use. The histograms of all files will be added.
class ProcessHistogramsTFile: public ProcessHistograms {
public:
    ProcessHistogramsTFile(const std::string & filename, const std::string & process);
    ProcessHistogramsTFile(const std::initializer_list<std::string> & filenames, const std::string & process);
    
    virtual std::vector<std::string> get_selections();
    virtual std::vector<std::string> get_hnames(const std::string & selection);
    virtual Histogram get_histogram(const std::string & selection, const std::string & hname);
    virtual std::string get_process(){
        return process_;
    }
    
    virtual ~ProcessHistogramsTFile();
    
private:
    void init_files(const std::initializer_list<std::string> & filenames);
    
    std::string process_;
    std::vector<TFile*> files;
};

// 'virtual' histogram input which adds histograms from different selections to create one virtual 'selection'
// For example, suppose you already have root files and created ProcessHistograms ttbar, zjets with the
// selections 'ee' and 'mm' (say dielectron and dimuon). In this case, AddSelections can be used to create the
// virtual selection corresponding to ee or mm, called 'mpluse', via
//  AddSelection ttbar_mpluse(ttbar, "mpluse", {"ee", "mm"});
//  AddSelection zjets_mpluse(zjets, "mpluse", {"ee", "mm"});
//
// In some cases (in particular data), you might want to specify which ProcessHistogram object to use for which selection
// when adding. In the example above, given data_ee and data_mm, and to use the histograms in selection 'ee' always from data_ee
// and for 'mm' from data_mm, use
//   AddSelection data_mpluse({data_ee, data_mm}, "mpluse", {"ee", "mm"});
// make sure to use consistent order for the first argument and third argument.
class AddSelections: public ProcessHistograms {
public:
    AddSelections(const std::shared_ptr<ProcessHistograms> & ph_, const std::string & added_selection_name, const std::vector<std::string> & sels);
    AddSelections(const std::vector<std::shared_ptr<ProcessHistograms>> & phs_, const std::string & added_selection_name, const std::vector<std::string> & sels);
    
    virtual std::string get_process() override;
    
    virtual std::vector<std::string> get_selections() override;
    
    virtual std::vector<std::string> get_hnames(const std::string & selection) override;
    
    virtual Histogram get_histogram(const std::string & selection, const std::string & hname) override;
    
    virtual ~AddSelections();
    
private:
    std::vector<std::shared_ptr<ProcessHistograms>> phs;
    std::string sname;
    std::vector<std::string> selections;
};


// make a 'virtual' input file by adding histograms from other, underlying ProcessHistograms
class AddProcesses: public ProcessHistograms {
public:
    AddProcesses(const std::vector<std::shared_ptr<ProcessHistograms>> & ph_, const std::string & added_process_name): phs{ph_},
       pname(added_process_name) {
    }
    
    virtual std::string get_process(){
        return pname;
    }
    
    virtual std::vector<std::string> get_selections(){
        return phs[0]->get_selections();
    }
    
    virtual std::vector<std::string> get_hnames(const std::string & selection){
        return phs[0]->get_hnames(selection);
    }
    
    virtual Histogram get_histogram(const std::string & selection, const std::string & hname){
        Histogram result = phs[0]->get_histogram(selection, hname);
        for(size_t i=1; i<phs.size(); ++i){
            Histogram tmp = phs[i]->get_histogram(selection, hname);
            result.histo->Add(tmp.histo.get());
        }
        result.process = pname;
        return std::move(result);
    }
    
    virtual ~AddProcesses(){}
    
private:
    std::vector<std::shared_ptr<ProcessHistograms>> phs;
    std::string pname;
};




void makestack(const std::vector<TH1*> & histos);


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
        
        std::string proc;
        std::string sel;
        
        e_matchmode hname_mm; // mm_any = match any, mm_full = use hname_full equality; mm_prefix = match hname_substring as prefix; mm_suffix = match hname_substring as suffix
        std::string hname_substr;
        std::string hname_full;
        
        formatter_type formatter;
        
        fspec(const std::string & proc_, const std::string & sel_, e_matchmode hname_mode_, const std::string & hname_, const formatter_type & formatter_):
            proc(proc_), sel(sel_), hname_mm(hname_mode_), hname_substr(hname_), hname_full(hname_), formatter(formatter_){}
    };
    
    std::string all_processes;
    std::string all_selections;
    
    std::vector<fspec> formatters;
};

// a histogram functor applying a list of other functors
class ApplyAll {
public:
    void operator()(Histogram & h){
        for(auto & f : functors){
            f(h);
        }
    }
    
    template<typename... T>
    explicit ApplyAll(T... input): functors{input...}{}
private:
    std::vector<std::function<void (Histogram &)>> functors;
};


class SetLegends {
public:
    void operator()(Histogram & h){
        h.legend = legend;
        if(latex_legend.empty()){
            h.latex_legend = h.process;
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
    std::string data;
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
    void stackplots(const std::initializer_list<std::string> & processes_to_stack, const std::string & filenamesuffix = "");
    
    // draw normalized plots of different processes; useful to compare the shape of a variable
    // each histogram in the input will lead to one output plot
    void shapeplots(const std::initializer_list<std::string> & processes_to_compare, const std::string & filenamesuffix = "");
    
    // make selection comparison plots by plotting different selections in the same histogram
    void selcomp_plots(const std::initializer_list<std::string> & selections_to_compare, const std::initializer_list<std::string> & plots_to_compare,
                       const std::string & outputname);
    
    // set global options (per-histogram options are set by the filters).
    // For valid options, see the *plots methods
    void set_option(const std::string & name, const std::string & value){
        options[name] = value;
    }
    
    // set the current histogram filter to f. For each histogram in the input, f(process, selection, hname) will be called and the return value decides
    // whether the histogram is actually used
    typedef std::function<bool (const std::string &, const std::string &, const std::string &)> filter_function_type;
    void set_histogram_filter(const filter_function_type & f){
        filter = f;
    }
    
    // print the integral (including underflow and overflow) of the given histogram name of all processes into the file of name filename (relative to outdir
    // given in the constructor).
    // if append is true, this will be appended to the file, otherwise the file is overwritten.
    void print_integrals(const std::string & selection, const std::string & hname, const std::string & filename, const std::string & title, bool append = true);
    
    // make a latex cutflow table:
    void cutflow(const std::string & cutflow_hname, const std::string & outname);
private:
    std::string outdir;
    std::vector<std::shared_ptr<ProcessHistograms> > histos;
    std::map<std::string, std::string> options;
    boost::optional<filter_function_type> filter;
    const Formatters & formatters;
    
    // get the formatted histograms for all processes from hsources; identified by seleciton and hname
    std::vector<Histogram> get_formatted_histograms(const std::vector<std::shared_ptr<ProcessHistograms> > & hsources, const std::string & selection,
                                                    const std::string & hname);
    
    std::vector<Histogram> get_selection_histogram(const std::shared_ptr<ProcessHistograms> & hsource,
                                                   const std::initializer_list<std::string> & selections_to_compare, const std::string & hname);
};



// convenience classes to be used for filters
class AndFilter {
public:
    AndFilter(const Plotter::filter_function_type & f1_, const Plotter::filter_function_type & f2_): f1(f1_), f2(f2_){
    }
    
    bool operator()(const std::string & p, const std::string & s, const std::string & h) const{
        if(!f1(p, s, h)) return false;
        return f2(p, s, h);
    }
    
private:   
    Plotter::filter_function_type f1, f2;
};


class RegexFilter {
public:
    explicit RegexFilter(const std::string & p_regex, const std::string & s_regex, const std::string & h_regex);
    
    bool operator()(const std::string & p, const std::string & s, const std::string & h) const;
    
private:
    boost::regex p_r, s_r, h_r;
};

#endif
