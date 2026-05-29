#ifndef PARGUS_CONFIG_H
#define PARGUS_CONFIG_H

typedef struct {
    char input_dir[512];
    char corpus_path[512];
    char out_dir[512];
    int threads;
    double sim_threshold;
    double ai_threshold;
    int bands;
    int rows;
    int ngram;
    int benchmark;
    int verbose;
    int serial_mode;
} EngineConfig;

void config_init_defaults(EngineConfig *config);

#endif

