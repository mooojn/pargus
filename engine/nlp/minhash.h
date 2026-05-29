#ifndef PARGUS_MINHASH_H
#define PARGUS_MINHASH_H

#include "config/config.h"
#include "nlp/tfidf.h"

#define PARGUS_MINHASH_K 100
#define PARGUS_SHINGLE_SIZE 3

typedef struct {
    unsigned int *values;
    int length;
} MinHashSignature;

typedef struct {
    MinHashSignature *signatures;
    int count;
    int length;
} MinHashCorpus;

unsigned int minhash_token_hash(const char *text, unsigned int seed);
void minhash_signature_free(MinHashSignature *signature);
void minhash_corpus_free(MinHashCorpus *corpus);
int build_minhash_corpus(const TfidfCorpus *tfidf, const EngineConfig *config, MinHashCorpus *corpus);

#endif

