#ifndef PARGUS_LSH_H
#define PARGUS_LSH_H

#include "config/config.h"
#include "nlp/minhash.h"

typedef struct {
    int a;
    int b;
} DocPair;

typedef struct {
    DocPair *pairs;
    int count;
    int capacity;
} PairList;

void pair_list_init(PairList *list);
void pair_list_free(PairList *list);
int pair_list_contains(const PairList *list, int a, int b);
int pair_list_add_unique(PairList *list, int a, int b);
int build_lsh_candidates(const MinHashCorpus *minhash, const EngineConfig *config, PairList *pairs);

#endif

