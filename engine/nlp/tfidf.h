#ifndef PARGUS_TFIDF_H
#define PARGUS_TFIDF_H

#include "common/types.h"
#include "config/config.h"
#include "nlp/stopwords.h"
#include "nlp/tokenizer.h"

typedef struct {
    int term_id;
    double value;
} SparseEntry;

typedef struct {
    SparseEntry *entries;
    int count;
    int capacity;
    double norm;
} SparseVector;

typedef struct {
    char *term;
    int document_frequency;
} VocabularyTerm;

typedef struct {
    VocabularyTerm *terms;
    int count;
    int capacity;
} Vocabulary;

typedef struct {
    TokenList *tokens;
    SparseVector *vectors;
    Vocabulary vocabulary;
    int doc_count;
    int total_tokens;
} TfidfCorpus;

void sparse_vector_init(SparseVector *vector);
void sparse_vector_free(SparseVector *vector);
void vocabulary_init(Vocabulary *vocabulary);
void vocabulary_free(Vocabulary *vocabulary);
int vocabulary_find(const Vocabulary *vocabulary, const char *term);
int build_tfidf_corpus(const DocumentList *docs, const StopwordSet *stopwords, const EngineConfig *config, TfidfCorpus *corpus);
void tfidf_corpus_free(TfidfCorpus *corpus);
double tfidf_value_for_term(const TfidfCorpus *corpus, int doc_id, const char *term);

#endif

