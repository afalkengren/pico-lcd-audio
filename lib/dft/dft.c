// DFT is defined as per Euler's formula as follows:
// X[k] = SUM(x[n]*cos(k*n*c)) - jSUM(x[n]*sin(k*n*c))
// where c is a constant 2*pi/N and N is number of frequency bins
// bin width = sample_frequency / N

#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

#include "dft.h"

#define PI 3.14159265358979323846
#define SHORT_MAX_D 32767.0

void hann_window(double *window, unsigned size) {
    const double c = PI/size;
    for (unsigned n = 0; n < size; ++n) {
        window[n] = sin(n*c) * sin(n*c);
    }
}

double* dft(const double *data, unsigned n_samples, unsigned n_bins) {
    const double c = 2.0 * PI / n_bins;
    double *result = malloc(n_bins * sizeof(double));
    double sum_sin, sum_cos;
    
    double *window = malloc(n_samples * sizeof(double));
    hann_window(window, n_samples);

    for (unsigned k = 0; k < n_bins; ++k) {
        sum_cos = 0.0; sum_sin = 0.0;
        const double kc = k*c;
        for (unsigned n = 0; n < n_samples; ++n) {
            double smoothed_data = data[n] * window[n];
            sum_cos += smoothed_data * cos(n*kc);
            sum_sin += smoothed_data * sin(n*kc);
        }
        result[k] = sqrt(sum_cos*sum_cos + sum_sin*sum_sin) / n_samples;
    }
    return result;
}

double* dft_radix2(const double *data, unsigned n_samples, unsigned n_bins) {
    return (double *)0;
}

double* dft_u8(const uint8_t *data, unsigned n_samples, unsigned n_bins) {
    double *buf = malloc(n_samples*sizeof(double));

    for (int i = 0; i < n_samples; ++i) {
        buf[i] = data[i];
    }
    double* result = dft(buf, n_samples, n_bins);
    free(buf);
    return result;
}