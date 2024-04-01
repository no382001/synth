#pragma once

// https://stackoverflow.com/questions/28009590/understanding-the-radix-2-fft-recursive-algorithm
// https://www.youtube.com/watch?v=mPVtovydY1k

// implements Cooley-Tukey algo for FT

#include <complex>
#include <valarray>
#include <math.h>
#include <cassert>
#include <iostream>

struct cooley_tukey {
	static void fft(std::valarray<std::complex<double>> &values);
    static void ifft(std::valarray<std::complex<double>> &fft);
    void run_tests();
};