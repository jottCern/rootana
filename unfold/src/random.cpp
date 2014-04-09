#include "random.hpp"
#include "unfold.hpp"

#include <set>
#include <chrono>

using namespace std;

Spectrum randomize(rnd_engine & rnd, const Spectrum & in, bool gauss, bool poisson){
    const size_t n = in.dim();
    Spectrum intermediate(in);
    
    // 1. gauss, if requested:
    if(gauss){
        normal_distribution<double> normal;
        // independent random values with mean=0, std.-dev. 1
        double x[n];
        for(size_t i=0; i<n; ++i){
            x[i] = normal(rnd);
        }
        // get the cholesky decomposition to transform x into intermediate:
        Matrix sqrt_cov = get_cholesky(in.cov());
        for(size_t i=0; i<n; i++){
            for(size_t j=0; j<=i; j++){//sqrt_cov is lower triangular
                intermediate[i] += sqrt_cov(i,j) * x[j];
            }
        }
    }
    
    // 2. Poisson (in-place for intermediate), if requested
    if(poisson){
        poisson_distribution<int> pdist;
        for(size_t i=0; i < n; ++i){
            intermediate[i] = pdist(rnd, poisson_distribution<int>::param_type(intermediate[i]));
        }
    }
    
    return intermediate;
}


Spectrum propagate_random(rnd_engine & rnd, const Matrix & R, const Spectrum & t){
    const size_t ngen = t.dim();
    const size_t nrec = R.get_n_rows();
    assert(ngen == R.get_n_cols());
    Spectrum result(nrec);
    uniform_real_distribution<double> uniform; // from 0 to 1
    for(size_t i=0; i<ngen; ++i){
        // multinomial ... TODO: anything better than this?
        size_t ngen_i = t[i] + 0.5;
        std::set<double> rnds;
        for(size_t j=0; j<ngen_i; ++j){
            rnds.insert(uniform(rnd));
        }
        // now go through response matrix r (gen index fixed to i) and fill result according to the numbers in rnds:
        double p_cumulative = 0.0;
        auto it = rnds.begin();
        auto it_end = rnds.end();
        for(size_t r=0; r<nrec; ++r){
            p_cumulative += R(r,i);
            while(it != it_end){
                if(*it > p_cumulative) break;
                result[r]++;
                ++it;
            }
        }
    }
    return result;
}


uint64_t generate_seed(){
    auto d = std::chrono::high_resolution_clock::now().time_since_epoch();
    uint64_t seed = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
    // to avoid clashes with other callers of generate_seed initialized in the same clock resolution interval, use a static counter:
    static int rnd_id = 0;
    seed = seed * 33 + rnd_id;
    ++rnd_id;
    // to avoid clashes in case of batch system usage with jobs starting in the same clock resolution
    // interval, also use the hostname for the seed.
    //Note that we should use a length of "HOST_NAME_MAX + 1" here, however, HOST_NAME_MAX is not defined on
    // all POSIX systems, so just use the value HOST_NAME_MAX=255 ...
    char hname[256];
    gethostname(hname, 256);
    // In case the hostname does not fit into hname, the name is truncated but no error is returned.
    // On the other hand, using some random bytes here for seeding is also Ok, just make sure we have a null byte
    // somewhere:
    hname[255] = '\0';
    int c;
    for(size_t i=0; (c = hname[i]); ++i){
        seed = seed * 33 + c;
    }
    // and finally, also include the process id, if theta starts on the same host at the same time:
    seed = seed * 33 + (int)getpid();
    return seed;
}

