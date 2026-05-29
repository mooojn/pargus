#include "common/types.h"
#include "config/config.h"
#include "io/writer.h"
#include "nlp/ngram.h"
#include "nlp/similarity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define rmdir _rmdir
#else
#include <unistd.h>
#endif

static char *copy_text(const char *text)
{
    size_t len = strlen(text);
    char *copy = (char *)malloc(len + 1);
    if (copy) {
        memcpy(copy, text, len + 1);
    }
    return copy;
}

static int read_file(const char *path, char *buffer, size_t buffer_size)
{
    FILE *file = fopen(path, "rb");
    size_t count;

    if (!file) {
        return 0;
    }
    count = fread(buffer, 1, buffer_size - 1, file);
    fclose(file);
    buffer[count] = '\0';
    return 1;
}

static int expect_contains(const char *text, const char *needle, const char *message)
{
    if (!strstr(text, needle)) {
        fprintf(stderr, "FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    const char *out_dir = "writer_escape_output";
    char report[4096];
    char csv[1024];
    EngineConfig config;
    Document doc;
    DocumentList docs;
    SimilarityResults similarity;
    AiScore score;
    AiScoreList ai_scores;
    StageTimings timings = {0};
    int ok = 1;

    config_init_defaults(&config);
    strcpy(config.out_dir, out_dir);

    memset(&doc, 0, sizeof(doc));
    doc.doc_id = 0;
    doc.filename = copy_text("essay,\"one\".txt");
    doc.path = copy_text("essay,\"one\".txt");
    doc.content = copy_text("alpha beta gamma");
    doc.length = strlen(doc.content);

    docs.items = &doc;
    docs.count = 1;
    docs.capacity = 1;

    memset(&similarity, 0, sizeof(similarity));
    similarity.doc_count = 1;
    similarity.matrix = (double *)calloc(1, sizeof(double));
    similarity.matrix[0] = 1.0;

    memset(&score, 0, sizeof(score));
    score.filename = copy_text(doc.filename);
    score.mean_perplexity = 42.0;
    score.perplexity_variance = 3.0;
    score.ai_score = 75.0;
    score.flagged = 1;
    ai_scores.items = &score;
    ai_scores.count = 1;

    ok = ok && write_engine_outputs(&config, &docs, &similarity, &ai_scores, &timings);
    ok = ok && read_file("writer_escape_output/report.json", report, sizeof(report));
    ok = ok && read_file("writer_escape_output/similarity_matrix.csv", csv, sizeof(csv));
    ok = ok && expect_contains(report, "essay,\\\"one\\\".txt", "JSON filename is escaped");
    ok = ok && expect_contains(csv, "\"essay,\"\"one\"\".txt\"", "CSV filename is escaped");

    remove("writer_escape_output/report.json");
    remove("writer_escape_output/similarity_matrix.csv");
    remove("writer_escape_output/ai_scores.csv");
    rmdir("writer_escape_output");
    free(similarity.matrix);
    free(score.filename);
    free(doc.filename);
    free(doc.path);
    free(doc.content);
    return ok ? 0 : 1;
}
