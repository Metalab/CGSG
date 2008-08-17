#ifndef METAFFT_H_
#define METAFFT_H_

bool metafft_init();
bool metafft_ready();
void metafft_get_spectrum(float *&spectrum, size_t &length);
void compute();
#endif
