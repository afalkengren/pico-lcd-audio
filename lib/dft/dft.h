#ifndef __INCLUDE_DFT_H__
#define __INCLUDE_DFT_H__

double* dft(const double *data, unsigned n_samples, unsigned n_bins);
double* dft_short(const short *data, unsigned n_samples, unsigned n_bins);

#endif