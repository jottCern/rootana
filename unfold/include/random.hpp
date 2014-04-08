#ifndef UNFOLD_RANDOM_HPP
#define UNFOLD_RANDOM_HPP

#include "fwd.hpp"


// generate a randomized spectrum based on the input spectrum.
// Generating the result is a two-step process:
// 1. if gauss is true, generate an intermediate spectrum based on the values and the covariance specified in 'in'
//    In case guass is false, the intermediate spectrum is taken identical to the input spectrum for the next step.
// 2. in case poisson is true, generate Poisson random numbers around the intermediate spectrum
//
// Note that for 2., no errors/covariance is used for generating random data.
//
// Note that the covariance matrix for the result spectrum is NOT filled.
Spectrum randomize(rnd_engine & rnd, const Spectrum & in, bool gauss = false, bool poisson = true);


// propagate a true observation in t to the reco level randomly. Values in t should be integer.
// note that covariance of result is not filled.
Spectrum propagate_random(rnd_engine & rnd, const Matrix & R, const Spectrum & t);


uint64_t generate_seed();

#endif
