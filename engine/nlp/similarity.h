#ifndef PARGUS_SIMILARITY_H
#define PARGUS_SIMILARITY_H

#include "config/config.h"
#include "nlp/lsh.h"
#include "nlp/tfidf.h"

typedef struct {
    int a;
    int b;
    double cosine;
    double rabin_karp;
    double combined;
} PairScore;

typedef struct {
    double *matrix;
    PairScore *flagged_pairs;
    int flagged_count;
    int flagged_capacity;
    int doc_count;
} SimilarityResults;

double cosine_similarity(const SparseVector *a, const SparseVector *b);
double rabin_karp_similarity(const TokenList *a, const TokenList *b, int shingle_size);
int compute_similarity_results(const TfidfCorpus *tfidf, const PairList *pairs, const EngineConfig *config, SimilarityResults *results);
double similarity_matrix_get(const SimilarityResults *results, int row, int col);
void similarity_results_free(SimilarityResults *results);

#endif
