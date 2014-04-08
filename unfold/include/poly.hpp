#ifndef UNFOLD_POLY_HPP
#define UNFOLD_POLY_HPP

#include <vector>
#include <map>
#include <string>
#include <ostream>
#include <cassert>

#include "matrix.hpp"
#include "fwd.hpp"

// variable id. Note that no 'current value' is associated with a variable, only identify.
class variable{
public:
    
    variable(): id_(-1){}
    
    explicit variable(const std::string & name);
    
    int id() const{
        return id_;
    }
    
    bool operator==(const variable & other) const{
        assert(id_!=-1 && other.id_!=-1);
        return id_==other.id_;
    }
    
    bool operator<(const variable & other) const{
        assert(id_!=-1 && other.id_!=-1);
        return id_ < other.id_;
    }
    
    std::string name() const;
    
private:
    static int next_id;
    static std::map<std::string, int> name_to_id;
    static std::map<int, std::string> id_to_name;
    int id_;
};


// a polynomial in many variables
class poly {
public:
    poly(const variable & v);
    poly(double c = 0.0);
    
    poly operator*(const poly & rhs) const;
    void operator+=(const poly & rhs){
        add(1.0, rhs);
    }
    
    void operator-=(const poly & rhs){
        add(-1.0, rhs);
    }
    
    void operator *=(const poly & rhs){
        *this = *this * rhs;
    }

    void operator*=(double c){
        if(c==0){
            terms.clear();
        }
        else{
            for(auto & t : terms){
               t.second *= c;
            }
        }
    }

    poly operator*(double c) const{
        poly result(*this);
        result *= c;
        return result;
    }
    
    // convert to variable. Only allowed if the polynomial is a monomial without constant term in one variable ...
    operator variable() const;
    
    // calculate   this += coeff * other
    void add(double coeff, const poly & other);
    
    // note: this only works for positive-definite polynomials with order 2!
    std::map<variable, double> argmin() const;
    
    void print(std::ostream & out) const;
private:
    
    // remove terms with coefficient of 0.0
    void cleanup(){
        // split to find and delete to avoid handling a lot of special cases
        // of invalidating iterators while they are in use.
        std::vector<std::map<prod, double>::iterator > to_erase;
        for(auto it = terms.begin(); it != terms.end(); ++it){
            if(it->second==0.0) to_erase.push_back(it);
        }
        for(auto it : to_erase){
            terms.erase(it);
        }
    }
    
    // a polynomial is a sum of product terms  c * prod_i x_i^n_i  where x_i is a variable and n_i the exponent. To allow
    // log(N) addition of polynomials, we need to order them somehow based on the variables and their exponents. Do that
    // by lexicographical order of (variable, exponent) pairs
    struct prod {
        std::map<variable, unsigned int> exponents; // map from the variable to the exponent; note that 0 exponent is not allowed (element should be erased instead)
        bool operator<(const prod & rhs) const;
    };
    
    std::map<prod, double> terms; // maps to coefficient
};

inline poly operator*(double a, const poly & p){
    poly result(p);
    result *= a;
    return result;
}

class poly_matrix {
public:
    // 0-order polynomials from matrix
    poly_matrix(const Matrix & m);
    
    // 0-order polynomial (=constant values), arranged either as column matrix or as row matrix
    poly_matrix(const std::vector<double> & values, bool column_matrix = true);
    
    // zero matrix of given dimenstion
    poly_matrix(size_t rows_=0, size_t cols_=0): rows(rows_), cols(cols_), elems(rows * cols){}
    
    // matrix of variables; the variables are named
    // basename_i_j
    // where i and j and the row index and column index, resp.
    poly_matrix(const std::string & basename, size_t rows, size_t cols);
    
    size_t get_n_rows() const{
        return rows;
    }
    
    size_t get_n_cols() const{
        return cols;
    }
    
    void print(std::ostream & out) const;
    
    poly & operator()(size_t i, size_t j){
        return elems[i*cols + j];
    }
    
    const poly & operator()(size_t i, size_t j) const{
        return elems[i*cols + j];
    }
    
    void operator*=(const poly_matrix & rhs);
    void operator+=(const poly_matrix & rhs);
    void operator-=(const poly_matrix & rhs);
    
    poly_matrix transpose() const;
    
    void swap(poly_matrix & rhs){
        std::swap(rows, rhs.rows);
        std::swap(cols, rhs.cols);
        std::swap(elems, rhs.elems);
    }
    
private:
    size_t rows, cols;
    std::vector<poly> elems;
};


// multiply each element with the coefficient a
poly_matrix operator*(double a, const poly_matrix & A);
poly_matrix operator*(const poly_matrix & A, const poly_matrix & B);
poly_matrix operator*(const Matrix & A, const poly_matrix & B);
poly_matrix operator*(const poly_matrix & A, const Matrix & B);


poly_matrix operator+(const poly_matrix & A, const poly_matrix & B);
poly_matrix operator-(const poly_matrix & A, const poly_matrix & B);

namespace std {

template<>
inline void swap(poly_matrix & l, poly_matrix & r){
    l.swap(r);
}

}

#endif
