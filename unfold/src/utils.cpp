#include "utils.hpp"
#include "Math/GSLMinimizer1D.h"

#include <cmath>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <vector>
#include <map>
#include <iostream>

using namespace std;

// minimize f.
// a < b < c   and
// f(a) > f(b) < f(c)
// must hold.
double find_xmin(const std::function<double (double)> & f, double a, double b, double c){
    ROOT::Math::GSLMinimizer1D mb;
    mb.SetFunction(f, b, a, c);
    // reltol of 1e-7 should be enough:
    bool res = mb.Minimize(1000, 0.0, 1e-7);
    if(!res) throw runtime_error("find_xmin: did not find minimum");
    return mb.XMinimum();
}


// find the global minimum by searching (on an exponential scale) on xmin, xmax and then call find_xmin with the
// best found minimum. If the minimum is at a border, a runtime_error is thrown.
double find_xmin(const std::function<double (double)> & f, double xmin, double xmax, int initial_steps){
    vector<pair<double, double> > values;
    assert(xmin > 0.0 && xmax > xmin);
    double factor = pow(xmax / xmin, 1.0 / initial_steps);
    double fmin = numeric_limits<double>::infinity();
    int imin = -1;
    double x = xmin;
    for(int i=0; i<initial_steps; ++i, x*=factor){
        double fx = f(x);
        if(!isfinite(fx)){
            cout << "find_xmin: function valus non-finite!" << endl;
            continue;
        }
        values.emplace_back(x, fx);
        if(fx < fmin){
            fmin = fx;
            imin = i;
        }
        if(imin > 0 && i - imin >= 1) break;
    }
    if(imin<=0 || imin+1 >= static_cast<int>(values.size())) throw runtime_error("find_xmin: minimum is at border");
    return find_xmin(f, values[imin-1].first, values[imin].first, values[imin+1].first);
}
