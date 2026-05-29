#include "nlp/similarity.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define RABIN_KARP_BASE 16777619u
#define RABIN_KARP_SHINGLE_SIZE 5

static int compare_uints(const void *left, const void *right)
{
    unsigned int a = *(const unsigned int *)left;
    unsigned int b = *(const unsigned int *)right;

    if (a == b) {
        return 0;
    }
    return a < b ? -1 : 1;
}

static unsigned int token_hash(const char *token)
{
    unsigned int hash = 2166136261u;

    for (size_t i = 0; token[i] != '\0'; i++) {
        hash ^= (unsigned char)token[i];
        hash *= 16777619u;
    }

    return hash == 0 ? 1u : hash;
}

static int build_unique_fingerprints(const TokenList *tokens, int shingle_size, unsigned int **out_values, int *out_count)
{
    unsigned int *values;
    int raw_count;
    int unique_count = 0;

    *out_values = NULL;
    *out_count = 0;

    if (!tokens || shingle_size <= 0 || tokens->count < shingle_size) {
        return 1;
    }

    raw_count = tokens->count - shingle_size + 1;
    values = (unsigned int *)calloc((size_t)raw_count, sizeof(unsigned int));
    if (!values) {
        return 0;
    }

    for (int start = 0; start < raw_count; start++) {
        unsigned int hash = 0u;
        for (int i = 0; i < shingle_size; i++) {
            hash = hash * RABIN_KARP_BASE + token_hash(tokens->tokens[start + i]);
        }
        values[start] = hash == 0 ? 1u : hash;
    }

    qsort(values, (size_t)raw_count, sizeof(unsigned int), compare_uints);
    for (int i = 0; i < raw_count; i++) {
        if (i == 0 || values[i] != values[i - 1]) {
            values[unique_count++] = values[i];
        }
    }

    *out_values = values;
    *out_count = unique_count;
    return 1;
}

static int add_flagged_pair(SimilarityResults *results, const PairScore *score)
{
    PairScore *grown;
    int new_capacity;

    if (results->flagged_count == results->flagged_capacity) {
        new_capacity = results->flagged_capacity == 0 ? 16 : results->flagged_capacity * 2;
        grown = (PairScore *)realloc(results->flagged_pairs, (size_t)new_capacity * sizeof(PairScore));
        if (!grown) {
            return 0;
        }
        results->flagged_pairs = grown;
        results->flagged_capacity = new_capacity;
    }

    results->flagged_pairs[results->flagged_count++] = *score;
    return 1;
}

double cosine_similarity(const SparseVector *a, const SparseVector *b)
{
    double dot = 0.0;

    if (!a || !b || a->norm <= 0.0 || b->norm <= 0.0) {
        return 0.0;
    }

    for (int i = 0; i < a->count; i++) {
        for (int j = 0; j < b->count; j++) {
            if (a->entries[i].term_id == b->entries[j].term_id) {
                dot += a->entries[i].value * b->entries[j].value;
                break;
            }
        }
    }

    return dot / (a->norm * b->norm);
}

double rabin_karp_similarity(const TokenList *a, const TokenList *b, int shingle_size)
{
    unsigned int *left = NULL;
    unsigned int *right = NULL;
    int left_count = 0;
    int right_count = 0;
    int i = 0;
    int j = 0;
    int intersection = 0;
    int union_count;
    double score = 0.0;

    if (!build_unique_fingerprints(a, shingle_size, &left, &left_count) ||
        !build_unique_fingerprints(b, shingle_size, &right, &right_count)) {
        free(left);
        free(right);
        return 0.0;
    }

    if (left_count == 0 && right_count == 0) {
        free(left);
        free(right);
        return 0.0;
    }

    while (i < left_count && j < right_count) {
        if (left[i] == right[j]) {
            intersection++;
            i++;
            j++;
        } else if (left[i] < right[j]) {
            i++;
        } else {
            j++;
        }
    }

    union_count = left_count + right_count - intersection;
    if (union_count > 0) {
        score = (double)intersection / (double)union_count;
    }

    free(left);
    free(right);
    return score;
}

int compute_similarity_results(const TfidfCorpus *tfidf, const PairList *pairs, const EngineConfig *config, SimilarityResults *results)
{
    int matrix_size;
    PairScore *scores;

    memset(results, 0, sizeof(*results));
    results->doc_count = tfidf->doc_count;

    matrix_size = tfidf->doc_count * tfidf->doc_count;
    results->matrix = (double *)calloc((size_t)matrix_size, sizeof(double));
    if (!results->matrix && matrix_size > 0) {
        return 0;
    }

    for (int i = 0; i < tfidf->doc_count; i++) {
        results->matrix[i * tfidf->doc_count + i] = 1.0;
    }

    scores = (PairScore *)calloc((size_t)pairs->count, sizeof(PairScore));
    if (!scores && pairs->count > 0) {
        similarity_results_free(results);
        return 0;
    }

#if defined(PARGUS_HAS_OPENMP)
    if (!config->serial_mode) {
        #pragma omp parallel for schedule(dynamic) num_threads(config->threads)
        for (int i = 0; i < pairs->count; i++) {
            int a = pairs->pairs[i].a;
            int b = pairs->pairs[i].b;
            double cosine = cosine_similarity(&tfidf->vectors[a], &tfidf->vectors[b]);
            double rk = rabin_karp_similarity(&tfidf->tokens[a], &tfidf->tokens[b], RABIN_KARP_SHINGLE_SIZE);
            double combined = 0.6 * cosine + 0.4 * rk;

            scores[i].a = a;
            scores[i].b = b;
            scores[i].cosine = cosine;
            scores[i].rabin_karp = rk;
            scores[i].combined = combined;
            results->matrix[a * tfidf->doc_count + b] = combined;
            results->matrix[b * tfidf->doc_count + a] = combined;
        }
    } else
#endif
    {
        for (int i = 0; i < pairs->count; i++) {
            int a = pairs->pairs[i].a;
            int b = pairs->pairs[i].b;
            double cosine = cosine_similarity(&tfidf->vectors[a], &tfidf->vectors[b]);
            double rk = rabin_karp_similarity(&tfidf->tokens[a], &tfidf->tokens[b], RABIN_KARP_SHINGLE_SIZE);
            double combined = 0.6 * cosine + 0.4 * rk;

            scores[i].a = a;
            scores[i].b = b;
            scores[i].cosine = cosine;
            scores[i].rabin_karp = rk;
            scores[i].combined = combined;
            results->matrix[a * tfidf->doc_count + b] = combined;
            results->matrix[b * tfidf->doc_count + a] = combined;
        }
    }

    for (int i = 0; i < pairs->count; i++) {
        if (scores[i].combined >= config->sim_threshold && !add_flagged_pair(results, &scores[i])) {
            free(scores);
            similarity_results_free(results);
            return 0;
        }
    }

    free(scores);
    return 1;
}

double similarity_matrix_get(const SimilarityResults *results, int row, int col)
{
    if (!results || row < 0 || col < 0 || row >= results->doc_count || col >= results->doc_count) {
        return 0.0;
    }
    return results->matrix[row * results->doc_count + col];
}

void similarity_results_free(SimilarityResults *results)
{
    if (!results) {
        return;
    }
    free(results->matrix);
    free(results->flagged_pairs);
    memset(results, 0, sizeof(*results));
}
