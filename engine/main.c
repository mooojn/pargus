#include "common/errors.h"
#include "common/timing.h"
#include "common/types.h"
#include "config/args.h"
#include "io/reader.h"
#include "io/writer.h"
#include "nlp/stopwords.h"
#include "nlp/tfidf.h"

#include <stdio.h>

static void print_loaded_documents(const DocumentList *docs)
{
    printf("Loaded %d document(s):\n", docs->count);
    for (int i = 0; i < docs->count; i++) {
        printf("  [%d] %s (%zu bytes)\n",
            docs->items[i].doc_id,
            docs->items[i].filename,
            docs->items[i].length);
    }
}

int main(int argc, char **argv)
{
    EngineConfig config;
    DocumentList docs;
    StopwordSet stopwords;
    TfidfCorpus tfidf;
    StageTimings timings = {0};
    double total_start;
    double io_start;
    double tfidf_start;
    double write_start;
    int parsed;
    int exit_code = PARGUS_OK;

    total_start = pargus_now_ms();

    parsed = parse_args(argc, argv, &config);
    if (parsed > 0) {
        return PARGUS_OK;
    }
    if (parsed < 0) {
        print_usage(argv[0]);
        return PARGUS_ERR_ARGS;
    }

    if (config.verbose) {
        printf("Pargus engine starting\n");
        printf("Input: %s\n", config.input_dir);
        printf("Output: %s\n", config.out_dir);
        printf("Threads: %d\n", config.threads);
        printf("Mode: %s\n", config.serial_mode ? "serial" : "openmp");
    }

    io_start = pargus_now_ms();
    if (!read_documents_from_dir(config.input_dir, &docs)) {
        return PARGUS_ERR_IO;
    }
    timings.io_ms = pargus_now_ms() - io_start;

    if (config.verbose) {
        print_loaded_documents(&docs);
    }

    stopword_set_init(&stopwords);
    if (!stopword_set_load(&stopwords, "./data/stopwords.txt") && config.verbose) {
        printf("Stopword file not found at ./data/stopwords.txt; continuing without stopword removal\n");
    }

    tfidf_start = pargus_now_ms();
    if (!build_tfidf_corpus(&docs, &stopwords, &config, &tfidf)) {
        stopword_set_free(&stopwords);
        document_list_free(&docs);
        return PARGUS_ERR_MEMORY;
    }
    timings.tokenize_tfidf_ms = pargus_now_ms() - tfidf_start;

    printf("TF-IDF: documents=%d total_tokens=%d vocabulary=%d build_ms=%.3f\n",
        docs.count,
        tfidf.total_tokens,
        tfidf.vocabulary.count,
        timings.tokenize_tfidf_ms);

    write_start = pargus_now_ms();
    if (!write_placeholder_outputs(&config, &docs, &timings)) {
        exit_code = PARGUS_ERR_IO;
    }
    timings.write_ms = pargus_now_ms() - write_start;
    timings.total_ms = pargus_now_ms() - total_start;

    if (exit_code == PARGUS_OK) {
        write_start = pargus_now_ms();
        if (!write_placeholder_outputs(&config, &docs, &timings)) {
            exit_code = PARGUS_ERR_IO;
        }
        timings.write_ms += pargus_now_ms() - write_start;
        timings.total_ms = pargus_now_ms() - total_start;
    }

    if (config.benchmark) {
        printf("{\"stage_times_ms\":{\"io\":%.3f,\"tokenize_tfidf\":%.3f,\"write_outputs\":%.3f,\"total\":%.3f}}\n",
            timings.io_ms,
            timings.tokenize_tfidf_ms,
            timings.write_ms,
            timings.total_ms);
    }

    if (config.verbose && exit_code == PARGUS_OK) {
        printf("Wrote placeholder outputs to %s\n", config.out_dir);
    }

    tfidf_corpus_free(&tfidf);
    stopword_set_free(&stopwords);
    document_list_free(&docs);
    return exit_code;
}
