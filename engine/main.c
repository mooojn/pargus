#include "common/errors.h"
#include "common/timing.h"
#include "common/types.h"
#include "config/args.h"
#include "io/reader.h"
#include "io/writer.h"
#include "nlp/lsh.h"
#include "nlp/minhash.h"
#include "nlp/ngram.h"
#include "nlp/similarity.h"
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
    MinHashCorpus minhash;
    PairList candidate_pairs;
    SimilarityResults similarity;
    NGramModel ngram_model;
    AiScoreList ai_scores;
    StageTimings timings = {0};
    double total_start;
    double io_start;
    double tfidf_start;
    double minhash_lsh_start;
    double similarity_start;
    double perplexity_start;
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

    minhash_lsh_start = pargus_now_ms();
    if (!build_minhash_corpus(&tfidf, &config, &minhash)) {
        tfidf_corpus_free(&tfidf);
        stopword_set_free(&stopwords);
        document_list_free(&docs);
        return PARGUS_ERR_MEMORY;
    }
    if (!build_lsh_candidates(&minhash, &config, &candidate_pairs)) {
        minhash_corpus_free(&minhash);
        tfidf_corpus_free(&tfidf);
        stopword_set_free(&stopwords);
        document_list_free(&docs);
        return PARGUS_ERR_MEMORY;
    }
    timings.minhash_lsh_ms = pargus_now_ms() - minhash_lsh_start;
    timings.total_pairs = docs.count * (docs.count - 1) / 2;
    timings.candidate_pairs = candidate_pairs.count;

    printf("MinHash/LSH: signatures=%d signature_length=%d total_pairs=%d candidate_pairs=%d build_ms=%.3f\n",
        minhash.count,
        minhash.length,
        timings.total_pairs,
        timings.candidate_pairs,
        timings.minhash_lsh_ms);

    similarity_start = pargus_now_ms();
    if (!compute_similarity_results(&tfidf, &candidate_pairs, &config, &similarity)) {
        pair_list_free(&candidate_pairs);
        minhash_corpus_free(&minhash);
        tfidf_corpus_free(&tfidf);
        stopword_set_free(&stopwords);
        document_list_free(&docs);
        return PARGUS_ERR_MEMORY;
    }
    timings.similarity_ms = pargus_now_ms() - similarity_start;
    timings.flagged_pairs = similarity.flagged_count;

    printf("Similarity: candidate_pairs=%d flagged_pairs=%d threshold=%.3f build_ms=%.3f\n",
        candidate_pairs.count,
        similarity.flagged_count,
        config.sim_threshold,
        timings.similarity_ms);

    perplexity_start = pargus_now_ms();
    if (!train_ngram_model(config.corpus_path, &stopwords, &config, &ngram_model) ||
        !score_ai_documents(&docs, &stopwords, &ngram_model, &config, &ai_scores)) {
        ngram_model_free(&ngram_model);
        similarity_results_free(&similarity);
        pair_list_free(&candidate_pairs);
        minhash_corpus_free(&minhash);
        tfidf_corpus_free(&tfidf);
        stopword_set_free(&stopwords);
        document_list_free(&docs);
        return PARGUS_ERR_MEMORY;
    }
    timings.perplexity_ms = pargus_now_ms() - perplexity_start;

    printf("AI scoring: documents=%d ngram=%d vocabulary=%d build_ms=%.3f\n",
        ai_scores.count,
        ngram_model.order,
        ngram_model.vocabulary.count,
        timings.perplexity_ms);

    write_start = pargus_now_ms();
    timings.total_ms = pargus_now_ms() - total_start;
    if (!write_engine_outputs(&config, &docs, &similarity, &ai_scores, &timings)) {
        exit_code = PARGUS_ERR_IO;
    }
    timings.write_ms = pargus_now_ms() - write_start;
    timings.total_ms = pargus_now_ms() - total_start;

    if (config.benchmark) {
        printf("{\"stage_times_ms\":{\"io\":%.3f,\"tokenize_tfidf\":%.3f,\"minhash_lsh\":%.3f,\"similarity\":%.3f,\"perplexity\":%.3f,\"write_outputs\":%.3f,\"total\":%.3f},\"candidate_pairs\":%d,\"flagged_pairs\":%d,\"total_pairs\":%d}\n",
            timings.io_ms,
            timings.tokenize_tfidf_ms,
            timings.minhash_lsh_ms,
            timings.similarity_ms,
            timings.perplexity_ms,
            timings.write_ms,
            timings.total_ms,
            timings.candidate_pairs,
            timings.flagged_pairs,
            timings.total_pairs);
    }

    if (config.verbose && exit_code == PARGUS_OK) {
        printf("Wrote engine outputs to %s\n", config.out_dir);
    }

    ai_score_list_free(&ai_scores);
    ngram_model_free(&ngram_model);
    similarity_results_free(&similarity);
    pair_list_free(&candidate_pairs);
    minhash_corpus_free(&minhash);
    tfidf_corpus_free(&tfidf);
    stopword_set_free(&stopwords);
    document_list_free(&docs);
    return exit_code;
}
