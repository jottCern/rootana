#include "poly.hpp"
#include "unfold.hpp"
#include <stdexcept>
#include <cassert>
#include <sstream>
#include <set>

using namespace std;

int variable::next_id = 0;
std::map<std::string, int> variable::name_to_id;
std::map<int, std::string> variable::id_to_name;

variable::variable(const string & name){
    auto it = name_to_id.find(name);
    if(it==name_to_id.end()){
        id_ = next_id++;
        name_to_id[name] = id_;
        id_to_name[id_] = name;
    }
    else{
        id_ = it->second;
    }
}

std::string variable::name() const{
    if(id_==-1) return "<invalid id>";
    auto it = id_to_name.find(id_);
    if(it==id_to_name.end()) throw invalid_argument("name: no such id");
    return it->second;
}

poly::poly(const variable & v){
    prod p;
    p.exponents[v] = 1;
    terms[p] = 1.0;
}

poly::poly(double c){
    if(c != 0.0){
        prod p;
        terms[p] = c;
    }
}

bool poly::prod::operator<(const poly::prod & rhs) const{
    auto it_left = exponents.begin();
    auto it_right = rhs.exponents.begin();
    auto left_end = exponents.end();
    auto right_end = rhs.exponents.end();
    while(it_left != left_end && it_right != right_end){
        if(it_left->first < it_right->first) return true;
        if(it_right->first < it_left->first) return false;
        // now we have the same variable, so compare the exponent:
        assert(it_left->first == it_right->first);
        if(it_left->second < it_right->second) return true;
        if(it_right->second < it_left->second) return false;
        // now the exponent is also the same, continue with the next:
        ++it_left;
        ++it_right;
    }
    // either the left or the right iterator is now at the end. left is smaller than right
    // iff left is at the end but right is not:
    return it_left == left_end && it_right != right_end;
}

poly poly::operator*(const poly & rhs)const{
    poly result;
    for(const auto & tleft : terms){
        for(const auto & tright : rhs.terms){
            // build the product term:
            prod p(tleft.first);
            for(const auto & pr : tright.first.exponents){
                p.exponents[pr.first] += pr.second;
            }
            double coeff = tleft.second * tright.second;
            // check if this term already exists and only add coeff if it does:
            result.terms[p] += coeff;
        }
    }
    result.cleanup();
    return move(result);
}

void poly::add(double coeff, const poly & rhs){
    auto it_left = terms.begin();
    auto it_right = rhs.terms.begin();
    auto left_end = terms.end();
    auto right_end = rhs.terms.end();
    typedef std::map<prod, double>::value_type vt;
    while(it_left != left_end && it_right != right_end){
        if(it_left->first < it_right->first){
            // nothing to do: this term does not exist in rhs
            ++it_left;
        }
        else if(it_right->first < it_left->first){
            // it_right is only in rhs, not here, so insert it and proceed to next in rhs. 
            // The new element will *precede* the current it_left, so this
            // * should be optimized with C++11
            // * leave the iteration over the left sequence it_left Ok
            terms.insert(it_left, vt(it_right->first, coeff * it_right->second));
            ++it_right;
        }
        else{ // the same: just add coefficient and move to next:
            it_left->second += coeff * it_right->second;
            ++it_left;
            ++it_right;
        }
    }
    // now we have reached the end with either it_left or it_right, so add everything that's left of rhs (might be an empty range):
    while(it_right != right_end){
        terms.insert(terms.end(), vt(it_right->first, coeff * it_right->second));
        ++it_right;
    }
}

poly::operator variable() const{
    if(terms.size() != 1){
        throw runtime_error("cannot convert poly to variable: terms.size() != 1");
    }
    auto term0 = terms.begin();
    if(term0->first.exponents.size() != 1){
        throw runtime_error("cannot convert poly to variable: exponents.size() != 1");
    }
    auto exponent0 = term0->first.exponents.begin();
    if(exponent0->second != 1){
        throw runtime_error("cannot convert poly to variable: exponent != 1");
    }
    return exponent0->first;
}


void poly::print(ostream & out) const{
    bool first = true;
    for(const auto & it : terms){
        bool cterm = it.first.exponents.empty();
        bool do_print_coeff = (cterm && it.second != 0.0) || (!cterm && it.second != 1.0);
        if(cterm and !do_print_coeff) continue;
        if(!first) out << " + ";
        if(do_print_coeff){
            out << it.second;
            first = false;
        }
        for(const auto & e : it.first.exponents){
            if(!first) out << " ";
            out << e.first.name();
            if(e.second > 1){
                out << "^" << e.second;
            }
            first = false;
        }
    }
    // if nothing has been printed, make sure to print "0":
    if(first) out << "0";
}


std::map<variable, double> poly::argmin() const{
    // to convert into "standard" matrices and vectors, make a mapping variable->index.
    // For that, first make a set of all variables:
    std::set<variable> variables;
    for(auto & t : terms){
        int total_exponent = 0;
        for(auto & e : t.first.exponents){
            total_exponent += e.second;
            variables.insert(e.first);
        }
        if(total_exponent > 2) throw range_error("argmin called for polynomial of order > 2");
    }
    std::map<variable, size_t> v2i;
    size_t index = 0;
    for(auto & v : variables){
        v2i[v] = index++;
    }
    
    // now express the problem as matrix inversion problem: minimizing the polynomial
    //   f(x) = x^T A x + b^T x + c
    // with constants in A, b, c has the minimum at
    //   x = -(A + A^T)^-1  b
    //
    // Note that A can be taken as lower triangular without loss of generality.
    //
    // fill A + A^T and b (as column vector):
    const size_t n = variables.size();
    Matrix ApA(n,n), b(n, 1);
    for(auto & t : terms){
        int index0 = -1, index1 = -1;
        for(auto & e : t.first.exponents){
            if(e.second==1){
                if(index0 < 0) index0 = v2i[e.first];
                else index1 = v2i[e.first];
            }
            else{
                assert(e.second==2);
                index0 = index1 = v2i[e.first];
            }
        }
        if(index1 < 0){
            if(index0 < 0){ // the constant term
                continue;
            }
            b(index0,0) = t.second;
        }
        else{
            // A + A^T:
            ApA(index0, index1) += t.second;
            ApA(index1, index0) += t.second;
        }
    }
    ApA.invert_cholesky();
    Matrix sol = ApA * b;
    map<variable, double> result;
    for(auto & it : v2i){
        result[it.first] = -sol(it.second, 0);
    }
    return result;
}



poly_matrix::poly_matrix(const std::vector<double> & values, bool column_matrix){
    elems.resize(values.size());
    std::copy(values.begin(), values.end(), elems.begin());
    if(column_matrix){
        rows = values.size();
        cols = 1;
    }
    else{
        rows = 1;
        cols = values.size();
    }
}

poly_matrix::poly_matrix(const Matrix & m): rows(m.get_n_rows()), cols(m.get_n_cols()), elems(rows * cols){
    for(size_t i=0; i<rows; ++i){
        for(size_t j=0; j<cols; ++j){
            operator()(i,j) = m(i,j);
        }
    }
}

poly_matrix::poly_matrix(const std::string & basename, size_t rows_, size_t cols_): rows(rows_), cols(cols_), elems(rows * cols){
    for(size_t i=0; i<rows; ++i){
        for(size_t j=0; j<cols; ++j){
            stringstream varname;
            varname << basename << "_" << i << "_" << j;
            operator()(i,j) = variable(varname.str());
        }
    }
}

void poly_matrix::operator*=(const poly_matrix & rhs){
    poly_matrix result = *this * rhs;
    std::swap(result, *this);
}

void poly_matrix::operator+=(const poly_matrix & rhs){
    if(cols != rhs.cols || rows != rhs.rows) throw invalid_argument("incompatible matrix in addition!");
    for(size_t i=0; i<elems.size(); ++i){
        elems[i] += rhs.elems[i];
    }
}

void poly_matrix::operator-=(const poly_matrix & rhs){
    if(cols != rhs.cols || rows != rhs.rows) throw invalid_argument("incompatible matrix in subtraction!");
    for(size_t i=0; i<elems.size(); ++i){
        elems[i] -= rhs.elems[i];
    }
}

poly_matrix poly_matrix::transpose() const {
    poly_matrix result(cols, rows);
    for(size_t i=0; i<rows; ++i){
        for(size_t j=0; j<cols; ++j){
            result(j,i) = (*this)(i,j);
        }
    }
    return result;
}

void poly_matrix::print(std::ostream & out) const{
    for(size_t i=0; i<rows; ++i){
        for(size_t j=0; j<cols; ++j){
            operator()(i,j).print(out);
            if(j+1 < cols) out << " | ";
        }
        out << endl;
    }
}

poly_matrix operator*(const poly_matrix & A, const poly_matrix & B){
    if(A.get_n_cols() != B.get_n_rows()) throw invalid_argument("incompatible matrix in multiplication!");
    size_t result_rows = A.get_n_rows();
    size_t result_cols = B.get_n_cols();
    size_t A_cols = A.get_n_cols();
    poly_matrix result(result_rows, result_cols);
    // i,j: indices in result matrix:
    for(size_t i=0; i<result_rows; ++i){
        for(size_t j=0; j<result_cols; ++j){
            for(size_t k=0; k<A_cols; ++k){
                result(i,j) += A(i,k) * B(k,j);
            }
        }
    }
    return result;
}

poly_matrix operator*(const Matrix & A, const poly_matrix & B){
    if(A.get_n_cols() != B.get_n_rows()) throw invalid_argument("incompatible matrix in multiplication!");
    size_t result_rows = A.get_n_rows();
    size_t result_cols = B.get_n_cols();
    size_t A_cols = A.get_n_cols();
    poly_matrix result(result_rows, result_cols);
    // i,j: indices in result matrix:
    for(size_t i=0; i<result_rows; ++i){
        for(size_t j=0; j<result_cols; ++j){
            for(size_t k=0; k<A_cols; ++k){
                result(i,j) += A(i,k) * B(k,j);
            }
        }
    }
    return result;
}

poly_matrix operator*(const poly_matrix & A, const Matrix & B){
    if(A.get_n_cols() != B.get_n_rows()) throw invalid_argument("incompatible matrix in multiplication!");
    size_t result_rows = A.get_n_rows();
    size_t result_cols = B.get_n_cols();
    size_t A_cols = A.get_n_cols();
    poly_matrix result(result_rows, result_cols);
    // i,j: indices in result matrix:
    for(size_t i=0; i<result_rows; ++i){
        for(size_t j=0; j<result_cols; ++j){
            for(size_t k=0; k<A_cols; ++k){
                result(i,j) += A(i,k) * B(k,j);
            }
        }
    }
    return result;
}

poly_matrix operator*(double a, const poly_matrix & A){
    poly_matrix result(A);
    for(size_t i=0; i<A.get_n_rows(); ++i){
        for(size_t j=0; j<A.get_n_cols(); ++j){
            result(i,j) *= a;
        }
    }
    return result;
}


poly_matrix operator+(const poly_matrix & A, const poly_matrix & B){
    poly_matrix result(A);
    result += B;
    return result;
}

poly_matrix operator-(const poly_matrix & A, const poly_matrix & B){
    poly_matrix result(A);
    result -= B;
    return result;
}
