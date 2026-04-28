#ifndef UTILS_H

#define UTILS_H

// header / library for different utilitarian functions that help other functions achieve their main goals

#include "commons.h"

void print_usage();
void permission_bits_to_symbolic(mode_t mode, char* out);

#endif // UTILS_H