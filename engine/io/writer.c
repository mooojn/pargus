#include "io/writer.h"

#include "common/string_utils.h"
#include "common/timing.h"

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
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            return 1;
        }
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

static void write_csv_field(FILE *file, const char *text)
{
    int needs_quotes = 0;

    if (!text) {
        return;
    }

    for (size_t i = 0; text[i] != '\0'; i++) {
        if (text[i] == '"' || text[i] == ',' || text[i] == '\n' || text[i] == '\r') {
            needs_quotes = 1;
            break;
        }
    }

    if (!needs_quotes) {
        fputs(text, file);
        return;
    }

    fputc('"', file);
    for (size_t i = 0; text[i] != '\0'; i++) {
        if (text[i] == '"') {
            fputc('"', file);
        }
        fputc(text[i], file);
    }
    fputc('"', file);
}

static void write_json_string(FILE *file, const char *text)
{
    fputc('"', file);
    if (text) {
        for (size_t i = 0; text[i] != '\0'; i++) {
            unsigned char ch = (unsigned char)text[i];

            switch (ch) {
            case '"':
                fputs("\\\"", file);
                break;
            case '\\':
                fputs("\\\\", file);
                break;
            case '\b':
                fputs("\\b", file);
                break;
            case '\f':
                fputs("\\f", file);
                break;
            case '\n':
                fputs("\\n", file);
                break;
            case '\r':
                fputs("\\r", file);
                break;
            case '\t':
                fputs("\\t", file);
                break;
            default:
                if (ch < 0x20) {
                    fprintf(file, "\\u%04x", ch);
                } else {
                    fputc(ch, file);
                }
                break;
            }
        }
    }
    fputc('"', file);
}

static int write_similarity_matrix(const EngineConfig *config, const DocumentList *docs, const SimilarityResults *similarity)
{
    FILE *file = open_output_file(config->out_dir, "similarity_matrix.csv");
    if (!file) {
        fprintf(stderr, "Failed to write similarity_matrix.csv\n");
        return 0;
    }

    fprintf(file, "document");
    for (int i = 0; i < docs->count; i++) {
        fputc(',', file);
        write_csv_field(file, docs->items[i].filename);
    }
    fprintf(file, "\n");

    for (int row = 0; row < docs->count; row++) {
        write_csv_field(file, docs->items[row].filename);
        for (int col = 0; col < docs->count; col++) {
            fprintf(file, ",%.3f", similarity_matrix_get(similarity, row, col));
        }
        fprintf(file, "\n");
    }

    fclose(file);
    return 1;
}

static int write_ai_scores(const EngineConfig *config, const AiScoreList *ai_scores)
{
    FILE *file = open_output_file(config->out_dir, "ai_scores.csv");
    if (!file) {
        fprintf(stderr, "Failed to write ai_scores.csv\n");
        return 0;
    }

    fprintf(file, "filename,mean_perplexity,ppl_variance,ai_score,flagged\n");
    for (int i = 0; i < ai_scores->count; i++) {
        write_csv_field(file, ai_scores->items[i].filename);
        fprintf(file, ",%.3f,%.3f,%.3f,%s\n",
            ai_scores->items[i].mean_perplexity,
            ai_scores->items[i].perplexity_variance,
            ai_scores->items[i].ai_score,
            ai_scores->items[i].flagged ? "true" : "false");
    }

    fclose(file);
    return 1;
}

static int write_report_json(const EngineConfig *config, const DocumentList *docs, const SimilarityResults *similarity, const AiScoreList *ai_scores, const StageTimings *timings)
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
    fprintf(file, "    \"mode\": \"%s\",\n",
        config->mode == PARGUS_MODE_PTHREADS ? "pthreads" :
        (config->mode == PARGUS_MODE_SERIAL ? "serial" : "openmp"));
    fprintf(file, "    \"sim_threshold\": %.6f,\n", config->sim_threshold);
    fprintf(file, "    \"ai_threshold\": %.6f\n", config->ai_threshold);
    fprintf(file, "  },\n");

    fprintf(file, "  \"documents\": [\n");
    for (int i = 0; i < docs->count; i++) {
        fprintf(file, "    {\"doc_id\": %d, \"filename\": ",
            docs->items[i].doc_id);
        write_json_string(file, docs->items[i].filename);
        fprintf(file, ", \"bytes\": %zu}%s\n",
            docs->items[i].length,
            i + 1 == docs->count ? "" : ",");
    }
    fprintf(file, "  ],\n");

    fprintf(file, "  \"similarity_matrix\": [\n");
    for (int row = 0; row < docs->count; row++) {
        fprintf(file, "    [");
        for (int col = 0; col < docs->count; col++) {
            fprintf(file, "%.3f%s", similarity_matrix_get(similarity, row, col), col + 1 == docs->count ? "" : ", ");
        }
        fprintf(file, "]%s\n", row + 1 == docs->count ? "" : ",");
    }
    fprintf(file, "  ],\n");
    fprintf(file, "  \"flagged_pairs\": [\n");
    for (int i = 0; i < similarity->flagged_count; i++) {
        const PairScore *pair = &similarity->flagged_pairs[i];
        fprintf(file, "    {\"doc_a\": %d, \"doc_b\": %d, \"filename_a\": ",
            pair->a,
            pair->b);
        write_json_string(file, docs->items[pair->a].filename);
        fprintf(file, ", \"filename_b\": ");
        write_json_string(file, docs->items[pair->b].filename);
        fprintf(file, ", \"cosine\": %.6f, \"rabin_karp\": %.6f, \"combined\": %.6f}%s\n",
            pair->cosine,
            pair->rabin_karp,
            pair->combined,
            i + 1 == similarity->flagged_count ? "" : ",");
    }
    fprintf(file, "  ],\n");
    fprintf(file, "  \"ai_scores\": [\n");
    for (int i = 0; i < ai_scores->count; i++) {
        fprintf(file, "    {\"filename\": ");
        write_json_string(file, ai_scores->items[i].filename);
        fprintf(file,
            ", \"mean_perplexity\": %.6f, \"perplexity_variance\": %.6f, \"ai_score\": %.6f, \"flagged\": %s}%s\n",
            ai_scores->items[i].mean_perplexity,
            ai_scores->items[i].perplexity_variance,
            ai_scores->items[i].ai_score,
            ai_scores->items[i].flagged ? "true" : "false",
            i + 1 == ai_scores->count ? "" : ",");
    }
    fprintf(file, "  ],\n");
    fprintf(file, "  \"benchmark\": {\n");
    fprintf(file, "    \"stage_times_ms\": {\n");
    fprintf(file, "      \"io\": %.3f,\n", timings ? timings->io_ms : 0.0);
    fprintf(file, "      \"tokenize_tfidf\": %.3f,\n", timings ? timings->tokenize_tfidf_ms : 0.0);
    fprintf(file, "      \"minhash_lsh\": %.3f,\n", timings ? timings->minhash_lsh_ms : 0.0);
    fprintf(file, "      \"similarity\": %.3f,\n", timings ? timings->similarity_ms : 0.0);
    fprintf(file, "      \"perplexity\": %.3f,\n", timings ? timings->perplexity_ms : 0.0);
    fprintf(file, "      \"write_outputs\": %.3f,\n", timings ? timings->write_ms : 0.0);
    fprintf(file, "      \"total\": %.3f\n", timings ? timings->total_ms : 0.0);
    fprintf(file, "    },\n");
    fprintf(file, "    \"candidate_generation\": {\n");
    fprintf(file, "      \"total_pairs\": %d,\n", timings ? timings->total_pairs : 0);
    fprintf(file, "      \"candidate_pairs\": %d,\n", timings ? timings->candidate_pairs : 0);
    fprintf(file, "      \"flagged_pairs\": %d\n", timings ? timings->flagged_pairs : 0);
    fprintf(file, "    }\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");

    fclose(file);
    return 1;
}

int write_engine_outputs(const EngineConfig *config, const DocumentList *docs, const SimilarityResults *similarity, const AiScoreList *ai_scores, StageTimings *timings)
{
    double start_ms = pargus_now_ms();

    if (!ensure_dir(config->out_dir)) {
        return 0;
    }

    if (!write_similarity_matrix(config, docs, similarity)) {
        return 0;
    }
    if (!write_ai_scores(config, ai_scores)) {
        return 0;
    }
    if (timings) {
        timings->write_ms = pargus_now_ms() - start_ms;
        timings->total_ms += timings->write_ms;
    }
    if (!write_report_json(config, docs, similarity, ai_scores, timings)) {
        return 0;
    }
    if (timings) {
        timings->write_ms = pargus_now_ms() - start_ms;
    }

    return 1;
}
