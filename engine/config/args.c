#include "config/args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int copy_arg(char *dest, size_t dest_size, const char *value, const char *name)
{
    if (!value || strlen(value) >= dest_size) {
        fprintf(stderr, "Invalid value for %s: too long\n", name);
        return 0;
    }
    strcpy(dest, value);
    return 1;
}

static int parse_int_arg(const char *value, const char *name, int min_value, int *out)
{
    char *end = NULL;
    long parsed;

    if (!value || value[0] == '\0') {
        fprintf(stderr, "Missing value for %s\n", name);
        return 0;
    }

    parsed = strtol(value, &end, 10);
    if (*end != '\0' || parsed < min_value || parsed > 1000000L) {
        fprintf(stderr, "Invalid integer for %s: %s\n", name, value);
        return 0;
    }

    *out = (int)parsed;
    return 1;
}

static int parse_double_arg(const char *value, const char *name, double min_value, double max_value, double *out)
{
    char *end = NULL;
    double parsed;

    if (!value || value[0] == '\0') {
        fprintf(stderr, "Missing value for %s\n", name);
        return 0;
    }

    parsed = strtod(value, &end);
    if (*end != '\0' || parsed < min_value || parsed > max_value) {
        fprintf(stderr, "Invalid number for %s: %s\n", name, value);
        return 0;
    }

    *out = parsed;
    return 1;
}

void config_init_defaults(EngineConfig *config)
{
    memset(config, 0, sizeof(*config));
    strcpy(config->out_dir, "./output");
    config->threads = 4;
    config->sim_threshold = 0.75;
    config->ai_threshold = 50.0;
    config->bands = 20;
    config->rows = 5;
    config->ngram = 3;
    config->mode = PARGUS_MODE_OPENMP;
}

void print_usage(const char *program)
{
    fprintf(stderr,
        "Usage: %s --input DIR [options]\n"
        "\n"
        "Required:\n"
        "  --input DIR              Directory containing .txt documents\n"
        "\n"
        "Options:\n"
        "  --corpus FILE            Corpus path for later n-gram training\n"
        "  --out-dir DIR            Output directory, default ./output\n"
        "  --threads N              Thread count, default 4\n"
        "  --sim-threshold VALUE    Similarity threshold 0.0 to 1.0, default 0.75\n"
        "  --ai-threshold VALUE     AI threshold, default 50.0\n"
        "  --bands N                LSH bands, default 20\n"
        "  --rows N                 LSH rows, default 5\n"
        "  --ngram N                N-gram order, default 3\n"
        "  --mode MODE              openmp, pthreads, or serial, default openmp\n"
        "  --benchmark              Print timing information\n"
        "  --verbose                Print loaded documents\n"
        "  --help                   Show this help\n",
        program);
}

int parse_args(int argc, char **argv, EngineConfig *config)
{
    config_init_defaults(config);

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        const char *next = (i + 1 < argc) ? argv[i + 1] : NULL;

        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            print_usage(argv[0]);
            return 1;
        } else if (strcmp(arg, "--input") == 0) {
            if (!copy_arg(config->input_dir, sizeof(config->input_dir), next, arg)) return -1;
            i++;
        } else if (strcmp(arg, "--corpus") == 0) {
            if (!copy_arg(config->corpus_path, sizeof(config->corpus_path), next, arg)) return -1;
            i++;
        } else if (strcmp(arg, "--out-dir") == 0) {
            if (!copy_arg(config->out_dir, sizeof(config->out_dir), next, arg)) return -1;
            i++;
        } else if (strcmp(arg, "--threads") == 0) {
            if (!parse_int_arg(next, arg, 1, &config->threads)) return -1;
            i++;
        } else if (strcmp(arg, "--sim-threshold") == 0) {
            if (!parse_double_arg(next, arg, 0.0, 1.0, &config->sim_threshold)) return -1;
            i++;
        } else if (strcmp(arg, "--ai-threshold") == 0) {
            if (!parse_double_arg(next, arg, 0.0, 1000000.0, &config->ai_threshold)) return -1;
            i++;
        } else if (strcmp(arg, "--bands") == 0) {
            if (!parse_int_arg(next, arg, 1, &config->bands)) return -1;
            i++;
        } else if (strcmp(arg, "--rows") == 0) {
            if (!parse_int_arg(next, arg, 1, &config->rows)) return -1;
            i++;
        } else if (strcmp(arg, "--ngram") == 0) {
            if (!parse_int_arg(next, arg, 1, &config->ngram)) return -1;
            i++;
        } else if (strcmp(arg, "--mode") == 0) {
            if (!next) {
                fprintf(stderr, "Missing value for --mode\n");
                return -1;
            }
            if (strcmp(next, "serial") == 0) {
                config->serial_mode = 1;
                config->mode = PARGUS_MODE_SERIAL;
            } else if (strcmp(next, "openmp") == 0) {
                config->serial_mode = 0;
                config->mode = PARGUS_MODE_OPENMP;
            } else if (strcmp(next, "pthreads") == 0) {
                config->serial_mode = 1;
                config->mode = PARGUS_MODE_PTHREADS;
            } else {
                fprintf(stderr, "Invalid mode: %s. Expected openmp, pthreads, or serial.\n", next);
                return -1;
            }
            i++;
        } else if (strcmp(arg, "--benchmark") == 0) {
            config->benchmark = 1;
        } else if (strcmp(arg, "--verbose") == 0) {
            config->verbose = 1;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", arg);
            return -1;
        }
    }

    if (config->input_dir[0] == '\0') {
        fprintf(stderr, "Missing required argument: --input DIR\n");
        return -1;
    }

    if (config->mode == PARGUS_MODE_SERIAL) {
        config->threads = 1;
    }

    return 0;
}
