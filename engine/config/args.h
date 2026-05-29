#ifndef PARGUS_ARGS_H
#define PARGUS_ARGS_H

#include "config/config.h"

int parse_args(int argc, char **argv, EngineConfig *config);
void print_usage(const char *program);

#endif

