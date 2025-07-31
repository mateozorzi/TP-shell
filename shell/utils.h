#ifndef UTILS_H
#define UTILS_H

#include "defs.h"

char *split_line(char *buf, char splitter);

int block_contains(char *buf, char c);

int printf_debug(char *format, ...);
int fprintf_debug(FILE *file, char *format, ...);

void restore_default_signal_status(int signal);

#endif  // UTILS_H
