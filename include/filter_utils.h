#ifndef __FILTER_UTILS__
#define __FILTER_UTILS__

#include "types.h"

void copy_unescaped(char *dest, const char *start, size_t raw_len);

int parse_condition(const char *input, char *field, char *op, char *value);

int compare_numeric(long long left, long long right, const char *op);

int compare_string(const char *left, const char *right, const char *op);

time_t parse_time(const char *value);

int match_condition(REPORT_DATA *r, const char *field, const char *op,
                    const char *value);
#endif
