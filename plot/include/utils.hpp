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

struct Histogram {
    std::unique_ptr<TH1> histo;// the actual root histogram
    ra::identifier process;   // process name
    ra::identifier selection; // selection (path in input rootfile)
    ra::identifier hname;     // histogram name as in the input rootfile (without path)
    std::string legend;       // legend to use in root plotting
    std::string latex_legend; // legend to use for latex
    
    bool ignore; // set to true to ignore this histogram
    
    std::map<std::string, std::string> options; //user-defined additional tags for controlling the plotting. Commonly used tags are:
       // "xmin", "xmax": minimum and maximum to draw
       // "xtext", "ytext": alternative labels for the x and y axes
       // (... see draw_histos in utils.cpp)
       
       
    Histogram(): ignore(false){}
};

void draw_histos(const std::vector<Histogram> & histos, const std::string & filename);

void create_dir(const std::string & filename);

void get_names_of_type(std::vector<std::string> & result, TDirectory * dir, const char * type, const std::string & prefix = "");

class ProcessHistograms {
public:
    virtual std::vector<std::string> get_histogram_names() = 0; // all histograms names available
//     virtual std::vector<std::string> get_histogram_names(const std::string &) = 0;
    virtual Histogram get_histogram(const std::string & name) = 0; // get a copy of the histogram named
    virtual ra::identifier id() = 0; // unique name for this process; data should be called data or DATA
    virtual ~ProcessHistograms();
    
    virtual std::set<ra::identifier> get_plot_types() = 0;
};


class ProcessHistogramsTFile: public ProcessHistograms {
public:
    ProcessHistogramsTFile(const std::string & filename, const ra::identifier & id);
    ProcessHistogramsTFile(const std::initializer_list<std::string> & filenames, const ra::identifier & id);
    
    virtual std::vector<std::string> get_histogram_names(); // including subdirectories ...
//     virtual std::vector<std::string> get_histogram_names(const std::string &);
    virtual Histogram get_histogram(const std::string & name);
    virtual ra::identifier id(){
        return id_;
    }
    virtual ~ProcessHistogramsTFile();
    
    virtual std::set<ra::identifier> get_plot_types();
    
private:
    void init_files(const std::initializer_list<std::string> & filenames);
    
    ra::identifier id_;
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
    const ra::identifier all;
    
    struct fspec {
        ra::identifier proc;
        ra::identifier sel;
        
        int hname_mode; // 0 = use hname equality (including if hname is wildcard); 1 = match hname_substring as prefix; 2 = match hname_substring as suffix
        std::string hname_substr;
        ra::identifier hname;
        
        formatter_type formatter;
        
        fspec(const ra::identifier & proc_, const ra::identifier & sel_, int hname_mode_, const std::string & hname_, const formatter_type & formatter_):
            proc(proc_), sel(sel_), hname_mode(hname_mode_), hname_substr(hname_), hname(hname_mode == 0 ? hname_ : ""), formatter(formatter_){}
    };
    
    std::vector<fspec> formatters;
};


class SetLegends {
public:
    virtual void operator()(Histogram & h){
        h.legend = legend;
        if(latex_legend.empty()){
            h.latex_legend = h.process.name();
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
        h.histo->Scale(factor);
    }
    
    explicit Scale(double factor_): factor(factor_){}
    
private:
    double factor;
};

class Ignore{
public:
    Ignore(bool ignore_ = true): ignore(ignore_){}
    
    void operator()(Histogram & h){
        h.ignore = ignore;
    }
    
private:
    bool ignore;
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
    
    void stackplots(const std::initializer_list<ra::identifier> & processes_to_stack);
    
    void shapeplots(const std::initializer_list<ra::identifier> & processes_to_compare, const std::string & filenamesuffix = "");
    
    void selcomp_plots(const std::initializer_list<ra::identifier> & selections_to_compare, const std::initializer_list<ra::identifier> & plots_to_compare, const std::string & outputname);
    
    // print the integral (including underflow and overflow) of the given histogram name of all processes into the file of name filename.
    // if append is true, this will be appended to the file, otherwise the file is overwritten.
    void print_integrals(const std::string & hname, const std::string & filename, const std::string & title, bool append = true);
    
    // make a latex cutflow table:
    void cutflow(const std::string & cutflow_hname, const std::string & outname);
private:
    std::string outdir;
    std::vector<std::shared_ptr<ProcessHistograms> > histos;
    const Formatters & formatters;
    
    std::vector<Histogram> get_formatted_histograms(const std::vector<std::shared_ptr<ProcessHistograms> > & hsources, const std::string & hname);
    
    std::vector<Histogram> get_selection_histogram(const std::shared_ptr<ProcessHistograms> & hsource, const std::initializer_list<ra::identifier> & selections_to_compare, const ra::identifier & plot_type);
};

#endif
