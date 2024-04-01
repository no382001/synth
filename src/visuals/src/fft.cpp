#include "fft.h"

void cooley_tukey::fft(std::valarray<std::complex<double>>& values) {
	const auto N = values.size();
	
    if (N <= 1) return;

	std::valarray<std::complex<double>> evens = values[std::slice(0, N / 2, 2)];
	std::valarray<std::complex<double>> odds  = values[std::slice(1, N / 2, 2)];

	cooley_tukey::fft(evens);
	cooley_tukey::fft(odds);

	for(auto i = 0; i < N / 2; i++) {
		auto index = std::polar(1.0, -2 * M_PI * i / N) * odds[i];
		values[i] = evens[i] + index;
		values[i + N / 2] = evens[i] - index;
	}
}

void cooley_tukey::ifft(std::valarray<std::complex<double>>& values) {
	values = values.apply(std::conj);
	cooley_tukey::fft(values);
	values = values.apply(std::conj);
	values /= values.size();
}

void cooley_tukey::run_tests(){
    { // Impulse
        const auto N = 8; 
        std::valarray<std::complex<double>> impulse_input(N);
        impulse_input[0] = 1;

        cooley_tukey::fft(impulse_input);

        for (auto& x : impulse_input) {
            assert(std::abs(x.real() - 1) < 1e-10);
            assert(std::abs(x.imag()) < 1e-10);
        }
    }

    { // Round trip
        const auto N = 8;
        std::valarray<std::complex<double>> original_signal(N);
        for (auto i = 0; i < N; ++i) {
            original_signal[i] = std::complex<double>(rand() / (double)RAND_MAX, rand() / (double)RAND_MAX);
        }

        std::valarray<std::complex<double>> transformed_signal = original_signal;
        cooley_tukey::fft(transformed_signal);
        cooley_tukey::ifft(transformed_signal);

        for (auto i = 0; i < N; ++i) {
            assert(std::abs(transformed_signal[i].real() - original_signal[i].real()) < 1e-10);
            assert(std::abs(transformed_signal[i].imag() - original_signal[i].imag()) < 1e-10);
        }
    }

    { // Inverse of constant
        const auto N = 8;
        std::valarray<std::complex<double>> constant_signal(N);
        for (auto i = 0; i < N; ++i) {
            constant_signal[i] = 1;
        }

        std::valarray<std::complex<double>> transformed_signal = constant_signal;
        cooley_tukey::fft(transformed_signal);
        cooley_tukey::ifft(transformed_signal);

        for (auto i = 0; i < N; ++i) {
            assert(std::abs(transformed_signal[i].real() - 1) < 1e-10);
            assert(std::abs(transformed_signal[i].imag()) < 1e-10);
        }
    }

    { // Sinusoidal inverted
        const size_t N = 256;
        const double frequency = 5.0;
        const double samplingRate = 256.0;
        std::valarray<std::complex<double>> sinusoidal_signal(N);
        
        for (size_t i = 0; i < N; ++i) {
            double t = i / samplingRate;
            sinusoidal_signal[i] = std::complex<double>(std::sin(2 * M_PI * frequency * t), 0);
        }

        std::valarray<std::complex<double>> transformed_signal = sinusoidal_signal;
        cooley_tukey::fft(transformed_signal);

        cooley_tukey::ifft(transformed_signal);

        for (size_t i = 0; i < N; ++i) {
            assert(std::abs(transformed_signal[i].real() - sinusoidal_signal[i].real()) < 1e-10);
            // The imaginary part should remain close to zero, but let's check it too
            assert(std::abs(transformed_signal[i].imag() - sinusoidal_signal[i].imag()) < 1e-10);
        }
    }
}