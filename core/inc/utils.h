#ifndef UTILS_H

#define UTILS_H

// header / library for different utilitarian functions that help other functions achieve their main goals

#include "commons.h"

void print_usage();
void print_usage_hub();
void permission_bits_to_symbolic(mode_t mode, char* out);
int notify_monitor(int signal);
int log_event(char* districtID, char* message);

#endif // UTILS_H