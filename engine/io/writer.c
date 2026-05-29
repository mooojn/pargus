#include "io/writer.h"

#include "common/string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

static int ensure_dir(const char *path)
{
#ifdef _WIN32
    if (CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
        return 1;
    }
#else
    if (mkdir(path, 0775) == 0 || errno == EEXIST) {
        return 1;
    }
#endif
    fprintf(stderr, "Failed to create output directory: %s\n", path);
    return 0;
}

static FILE *open_output_file(const char *out_dir, const char *name)
{
    char path[1024];

    if (!pargus_join_path(path, sizeof(path), out_dir, name)) {
        fprintf(stderr, "Output path is too long: %s\n", name);
        return NULL;
    }

    return fopen(path, "wb");
}

static int write_similarity_matrix(const EngineConfig *config, const DocumentList *docs)
{
    FILE *file = open_output_file(config->out_dir, "similarity_matrix.csv");
    if (!file) {
        fprintf(stderr, "Failed to write similarity_matrix.csv\n");
        return 0;
    }

    fprintf(file, "document");
    for (int i = 0; i < docs->count; i++) {
        fprintf(file, ",%s", docs->items[i].filename);
    }
    fprintf(file, "\n");

    for (int row = 0; row < docs->count; row++) {
        fprintf(file, "%s", docs->items[row].filename);
        for (int col = 0; col < docs->count; col++) {
            fprintf(file, ",%.3f", row == col ? 1.0 : 0.0);
        }
        fprintf(file, "\n");
    }

    fclose(file);
    return 1;
}

static int write_ai_scores(const EngineConfig *config, const DocumentList *docs)
{
    FILE *file = open_output_file(config->out_dir, "ai_scores.csv");
    if (!file) {
        fprintf(stderr, "Failed to write ai_scores.csv\n");
        return 0;
    }

    fprintf(file, "filename,mean_perplexity,ppl_variance,ai_score,flagged\n");
    for (int i = 0; i < docs->count; i++) {
        fprintf(file, "%s,0.000,0.000,0.000,false\n", docs->items[i].filename);
    }

    fclose(file);
    return 1;
}

static int write_report_json(const EngineConfig *config, const DocumentList *docs, const StageTimings *timings)
{
    FILE *file = open_output_file(config->out_dir, "report.json");
    if (!file) {
        fprintf(stderr, "Failed to write report.json\n");
        return 0;
    }

    fprintf(file, "{\n");
    fprintf(file, "  \"meta\": {\n");
    fprintf(file, "    \"num_docs\": %d,\n", docs->count);
    fprintf(file, "    \"threads_used\": %d,\n", config->threads);
    fprintf(file, "    \"mode\": \"%s\",\n", config->serial_mode ? "serial" : "openmp");
    fprintf(file, "    \"sim_threshold\": %.6f,\n", config->sim_threshold);
    fprintf(file, "    \"ai_threshold\": %.6f\n", config->ai_threshold);
    fprintf(file, "  },\n");

    fprintf(file, "  \"documents\": [\n");
    for (int i = 0; i < docs->count; i++) {
        fprintf(file, "    {\"doc_id\": %d, \"filename\": \"%s\", \"bytes\": %zu}%s\n",
            docs->items[i].doc_id,
            docs->items[i].filename,
            docs->items[i].length,
            i + 1 == docs->count ? "" : ",");
    }
    fprintf(file, "  ],\n");

    fprintf(file, "  \"similarity_matrix\": [\n");
    for (int row = 0; row < docs->count; row++) {
        fprintf(file, "    [");
        for (int col = 0; col < docs->count; col++) {
            fprintf(file, "%.3f%s", row == col ? 1.0 : 0.0, col + 1 == docs->count ? "" : ", ");
        }
        fprintf(file, "]%s\n", row + 1 == docs->count ? "" : ",");
    }
    fprintf(file, "  ],\n");
    fprintf(file, "  \"flagged_pairs\": [],\n");
    fprintf(file, "  \"ai_scores\": [\n");
    for (int i = 0; i < docs->count; i++) {
        fprintf(file,
            "    {\"filename\": \"%s\", \"mean_perplexity\": 0.0, \"perplexity_variance\": 0.0, \"ai_score\": 0.0, \"flagged\": false}%s\n",
            docs->items[i].filename,
            i + 1 == docs->count ? "" : ",");
    }
    fprintf(file, "  ],\n");
    fprintf(file, "  \"benchmark\": {\n");
    fprintf(file, "    \"stage_times_ms\": {\n");
    fprintf(file, "      \"io\": %.3f,\n", timings ? timings->io_ms : 0.0);
    fprintf(file, "      \"tokenize_tfidf\": %.3f,\n", timings ? timings->tokenize_tfidf_ms : 0.0);
    fprintf(file, "      \"minhash_lsh\": %.3f,\n", timings ? timings->minhash_lsh_ms : 0.0);
    fprintf(file, "      \"write_outputs\": %.3f,\n", timings ? timings->write_ms : 0.0);
    fprintf(file, "      \"total\": %.3f\n", timings ? timings->total_ms : 0.0);
    fprintf(file, "    },\n");
    fprintf(file, "    \"candidate_generation\": {\n");
    fprintf(file, "      \"total_pairs\": %d,\n", timings ? timings->total_pairs : 0);
    fprintf(file, "      \"candidate_pairs\": %d\n", timings ? timings->candidate_pairs : 0);
    fprintf(file, "    }\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");

    fclose(file);
    return 1;
}

int write_placeholder_outputs(const EngineConfig *config, const DocumentList *docs, const StageTimings *timings)
{
    if (!ensure_dir(config->out_dir)) {
        return 0;
    }

    if (!write_similarity_matrix(config, docs)) {
        return 0;
    }
    if (!write_ai_scores(config, docs)) {
        return 0;
    }
    if (!write_report_json(config, docs, timings)) {
        return 0;
    }

    return 1;
}
