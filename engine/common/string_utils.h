#ifndef PARGUS_STRING_UTILS_H
#define PARGUS_STRING_UTILS_H

#include <stddef.h>

char *pargus_strdup(const char *text);
int pargus_ends_with_ci(const char *text, const char *suffix);
const char *pargus_basename(const char *path);
int pargus_join_path(char *out, size_t out_size, const char *dir, const char *name);

#endif

