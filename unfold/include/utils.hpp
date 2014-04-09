#ifndef UNFOLD_UTILS_HPP
#define UNFOLD_UTILS_HPP

#include <functional>
#include <map>

// function wrapper for functions double -> double which saves the values for later retrieval.
class save_values{
public:
    save_values(const save_values & ) = delete;
    
    save_values(const std::function<double (double)> & f_): f(f_){}
    
    double operator()(double x) const{
        double result = f(x);
        fvalues[x] = result;
        return result;
    }
    
    const std::map<double, double> & fvals() const{
        return fvalues;
    }
    
private:
    const std::function<double (double)> f;
    mutable std::map<double, double> fvalues;
};



double find_xmin(const std::function<double (double)> & f, double a, double b, double c);

double find_xmin(const std::function<double (double)> & f, double xmin = 1e-12, double xmax = 1e12, int initial_steps = 30);


#endif
