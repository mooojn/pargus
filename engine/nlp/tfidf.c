#include "nlp/tfidf.h"

#include "common/string_utils.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int term_id;
    int count;
} TermCount;

typedef struct {
    TermCount *items;
    int count;
    int capacity;
} TermCountList;

void sparse_vector_init(SparseVector *vector)
{
    vector->entries = NULL;
    vector->count = 0;
    vector->capacity = 0;
    vector->norm = 0.0;
}

void sparse_vector_free(SparseVector *vector)
{
    if (!vector) {
        return;
    }
    free(vector->entries);
    sparse_vector_init(vector);
}

void vocabulary_init(Vocabulary *vocabulary)
{
    vocabulary->terms = NULL;
    vocabulary->count = 0;
    vocabulary->capacity = 0;
}

void vocabulary_free(Vocabulary *vocabulary)
{
    if (!vocabulary) {
        return;
    }
    for (int i = 0; i < vocabulary->count; i++) {
        free(vocabulary->terms[i].term);
    }
    free(vocabulary->terms);
    vocabulary_init(vocabulary);
}

int vocabulary_find(const Vocabulary *vocabulary, const char *term)
{
    for (int i = 0; i < vocabulary->count; i++) {
        if (strcmp(vocabulary->terms[i].term, term) == 0) {
            return i;
        }
    }
    return -1;
}

static int vocabulary_add(Vocabulary *vocabulary, const char *term)
{
    VocabularyTerm *grown;
    int new_capacity;

    if (vocabulary->count == vocabulary->capacity) {
        new_capacity = vocabulary->capacity == 0 ? 128 : vocabulary->capacity * 2;
        grown = (VocabularyTerm *)realloc(vocabulary->terms, (size_t)new_capacity * sizeof(VocabularyTerm));
        if (!grown) {
            return -1;
        }
        vocabulary->terms = grown;
        vocabulary->capacity = new_capacity;
    }

    vocabulary->terms[vocabulary->count].term = pargus_strdup(term);
    if (!vocabulary->terms[vocabulary->count].term) {
        return -1;
    }
    vocabulary->terms[vocabulary->count].document_frequency = 0;
    return vocabulary->count++;
}

static int vocabulary_get_or_add(Vocabulary *vocabulary, const char *term)
{
    int found = vocabulary_find(vocabulary, term);
    if (found >= 0) {
        return found;
    }
    return vocabulary_add(vocabulary, term);
}

static void term_count_list_init(TermCountList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void term_count_list_free(TermCountList *list)
{
    free(list->items);
    term_count_list_init(list);
}

static int term_count_list_add_token(TermCountList *list, int term_id, int *is_new)
{
    TermCount *grown;
    int new_capacity;

    for (int i = 0; i < list->count; i++) {
        if (list->items[i].term_id == term_id) {
            list->items[i].count++;
            *is_new = 0;
            return 1;
        }
    }

    if (list->count == list->capacity) {
        new_capacity = list->capacity == 0 ? 32 : list->capacity * 2;
        grown = (TermCount *)realloc(list->items, (size_t)new_capacity * sizeof(TermCount));
        if (!grown) {
            return 0;
        }
        list->items = grown;
        list->capacity = new_capacity;
    }

    list->items[list->count].term_id = term_id;
    list->items[list->count].count = 1;
    list->count++;
    *is_new = 1;
    return 1;
}

static int sparse_vector_reserve(SparseVector *vector, int count)
{
    vector->entries = (SparseEntry *)calloc((size_t)count, sizeof(SparseEntry));
    if (!vector->entries && count > 0) {
        return 0;
    }
    vector->capacity = count;
    return 1;
}

static int build_vectors_from_tokens(const DocumentList *docs, TfidfCorpus *corpus)
{
    TermCountList *doc_counts;
    int ok = 1;

    doc_counts = (TermCountList *)calloc((size_t)docs->count, sizeof(TermCountList));
    if (!doc_counts) {
        return 0;
    }

    for (int doc_id = 0; doc_id < docs->count; doc_id++) {
        term_count_list_init(&doc_counts[doc_id]);
        for (int token_id = 0; token_id < corpus->tokens[doc_id].count; token_id++) {
            int is_new = 0;
            int term_id = vocabulary_get_or_add(&corpus->vocabulary, corpus->tokens[doc_id].tokens[token_id]);
            if (term_id < 0 || !term_count_list_add_token(&doc_counts[doc_id], term_id, &is_new)) {
                ok = 0;
                break;
            }
            if (is_new) {
                corpus->vocabulary.terms[term_id].document_frequency++;
            }
        }
        if (!ok) {
            break;
        }
    }

    if (ok) {
        for (int doc_id = 0; doc_id < docs->count; doc_id++) {
            SparseVector *vector = &corpus->vectors[doc_id];
            int token_count = corpus->tokens[doc_id].count;
            double norm_sq = 0.0;

            sparse_vector_init(vector);
            if (!sparse_vector_reserve(vector, doc_counts[doc_id].count)) {
                ok = 0;
                break;
            }

            for (int i = 0; i < doc_counts[doc_id].count; i++) {
                int term_id = doc_counts[doc_id].items[i].term_id;
                int df = corpus->vocabulary.terms[term_id].document_frequency;
                double tf = token_count > 0 ? (double)doc_counts[doc_id].items[i].count / (double)token_count : 0.0;
                double idf = log((1.0 + (double)docs->count) / (1.0 + (double)df)) + 1.0;
                double value = tf * idf;

                vector->entries[vector->count].term_id = term_id;
                vector->entries[vector->count].value = value;
                vector->count++;
                norm_sq += value * value;
            }
            vector->norm = sqrt(norm_sq);
        }
    }

    for (int doc_id = 0; doc_id < docs->count; doc_id++) {
        term_count_list_free(&doc_counts[doc_id]);
    }
    free(doc_counts);
    return ok;
}

int build_tfidf_corpus(const DocumentList *docs, const StopwordSet *stopwords, const EngineConfig *config, TfidfCorpus *corpus)
{
    memset(corpus, 0, sizeof(*corpus));
    corpus->doc_count = docs->count;
    vocabulary_init(&corpus->vocabulary);

    corpus->tokens = (TokenList *)calloc((size_t)docs->count, sizeof(TokenList));
    corpus->vectors = (SparseVector *)calloc((size_t)docs->count, sizeof(SparseVector));
    if (!corpus->tokens || !corpus->vectors) {
        tfidf_corpus_free(corpus);
        return 0;
    }

#if defined(PARGUS_HAS_OPENMP)
    if (!config->serial_mode) {
        int ok = 1;
        #pragma omp parallel for schedule(dynamic) num_threads(config->threads) reduction(&&:ok)
        for (int i = 0; i < docs->count; i++) {
            if (!tokenize_text(docs->items[i].content, stopwords, &corpus->tokens[i])) {
                ok = 0;
            }
        }
        if (!ok) {
            tfidf_corpus_free(corpus);
            return 0;
        }
    } else
#endif
    {
        for (int i = 0; i < docs->count; i++) {
            if (!tokenize_text(docs->items[i].content, stopwords, &corpus->tokens[i])) {
                tfidf_corpus_free(corpus);
                return 0;
            }
        }
    }

    for (int i = 0; i < docs->count; i++) {
        corpus->total_tokens += corpus->tokens[i].count;
    }

    if (!build_vectors_from_tokens(docs, corpus)) {
        tfidf_corpus_free(corpus);
        return 0;
    }

    (void)config;
    return 1;
}

void tfidf_corpus_free(TfidfCorpus *corpus)
{
    if (!corpus) {
        return;
    }

    if (corpus->tokens) {
        for (int i = 0; i < corpus->doc_count; i++) {
            token_list_free(&corpus->tokens[i]);
        }
    }
    if (corpus->vectors) {
        for (int i = 0; i < corpus->doc_count; i++) {
            sparse_vector_free(&corpus->vectors[i]);
        }
    }

    free(corpus->tokens);
    free(corpus->vectors);
    vocabulary_free(&corpus->vocabulary);
    memset(corpus, 0, sizeof(*corpus));
}

double tfidf_value_for_term(const TfidfCorpus *corpus, int doc_id, const char *term)
{
    int term_id;

    if (!corpus || doc_id < 0 || doc_id >= corpus->doc_count) {
        return 0.0;
    }

    term_id = vocabulary_find(&corpus->vocabulary, term);
    if (term_id < 0) {
        return 0.0;
    }

    for (int i = 0; i < corpus->vectors[doc_id].count; i++) {
        if (corpus->vectors[doc_id].entries[i].term_id == term_id) {
            return corpus->vectors[doc_id].entries[i].value;
        }
    }

    return 0.0;
}
