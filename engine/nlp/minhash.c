#include "nlp/minhash.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

unsigned int minhash_token_hash(const char *text, unsigned int seed)
{
    unsigned int hash = 2166136261u ^ seed;

    for (size_t i = 0; text && text[i] != '\0'; i++) {
        hash ^= (unsigned char)text[i];
        hash *= 16777619u;
    }

    hash ^= seed * 0x9e3779b9u;
    hash ^= hash >> 16;
    hash *= 0x85ebca6bu;
    hash ^= hash >> 13;
    return hash;
}

static unsigned int shingle_hash(const TokenList *tokens, int start, int width, unsigned int seed)
{
    unsigned int hash = 2166136261u ^ seed;

    for (int i = 0; i < width; i++) {
        const char *token = tokens->tokens[start + i];
        unsigned int token_hash = minhash_token_hash(token, seed + (unsigned int)i * 17u);
        hash ^= token_hash;
        hash *= 16777619u;
    }

    hash ^= hash >> 15;
    return hash == 0 ? 1u : hash;
}

void minhash_signature_free(MinHashSignature *signature)
{
    if (!signature) {
        return;
    }
    free(signature->values);
    signature->values = NULL;
    signature->length = 0;
}

void minhash_corpus_free(MinHashCorpus *corpus)
{
    if (!corpus) {
        return;
    }
    if (corpus->signatures) {
        for (int i = 0; i < corpus->count; i++) {
            minhash_signature_free(&corpus->signatures[i]);
        }
    }
    free(corpus->signatures);
    corpus->signatures = NULL;
    corpus->count = 0;
    corpus->length = 0;
}

static int build_signature_for_tokens(const TokenList *tokens, MinHashSignature *signature)
{
    int shingle_width = tokens->count >= PARGUS_SHINGLE_SIZE ? PARGUS_SHINGLE_SIZE : tokens->count;

    signature->length = PARGUS_MINHASH_K;
    signature->values = (unsigned int *)malloc((size_t)PARGUS_MINHASH_K * sizeof(unsigned int));
    if (!signature->values) {
        return 0;
    }

    for (int i = 0; i < PARGUS_MINHASH_K; i++) {
        signature->values[i] = UINT_MAX;
    }

    if (shingle_width == 0) {
        for (int i = 0; i < PARGUS_MINHASH_K; i++) {
            signature->values[i] = 0;
        }
        return 1;
    }

    for (int shingle_start = 0; shingle_start <= tokens->count - shingle_width; shingle_start++) {
        for (int hash_id = 0; hash_id < PARGUS_MINHASH_K; hash_id++) {
            unsigned int seed = 0x9e3779b9u + (unsigned int)hash_id * 0x85ebca6bu;
            unsigned int value = shingle_hash(tokens, shingle_start, shingle_width, seed);
            if (value < signature->values[hash_id]) {
                signature->values[hash_id] = value;
            }
        }
    }

    return 1;
}

int build_minhash_corpus(const TfidfCorpus *tfidf, const EngineConfig *config, MinHashCorpus *corpus)
{
    (void)config;

    memset(corpus, 0, sizeof(*corpus));
    corpus->count = tfidf->doc_count;
    corpus->length = PARGUS_MINHASH_K;
    corpus->signatures = (MinHashSignature *)calloc((size_t)tfidf->doc_count, sizeof(MinHashSignature));
    if (!corpus->signatures) {
        return 0;
    }

#if defined(PARGUS_HAS_OPENMP)
    if (!config->serial_mode) {
        int ok = 1;
        #pragma omp parallel for schedule(dynamic) num_threads(config->threads) reduction(&&:ok)
        for (int i = 0; i < tfidf->doc_count; i++) {
            if (!build_signature_for_tokens(&tfidf->tokens[i], &corpus->signatures[i])) {
                ok = 0;
            }
        }
        if (!ok) {
            minhash_corpus_free(corpus);
            return 0;
        }
    } else
#endif
    {
        for (int i = 0; i < tfidf->doc_count; i++) {
            if (!build_signature_for_tokens(&tfidf->tokens[i], &corpus->signatures[i])) {
                minhash_corpus_free(corpus);
                return 0;
            }
        }
    }

    return 1;
}
