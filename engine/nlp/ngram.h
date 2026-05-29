#ifndef PARGUS_NGRAM_H
#define PARGUS_NGRAM_H

#include "common/types.h"
#include "config/config.h"
#include "nlp/stopwords.h"
#include "nlp/tokenizer.h"

typedef struct {
    char *key;
    int count;
} NGramEntry;

typedef struct {
    NGramEntry *entries;
    int count;
    int capacity;
} NGramTable;

typedef struct {
    NGramTable ngrams;
    NGramTable contexts;
    NGramTable vocabulary;
    int order;
    int total_tokens;
} NGramModel;

typedef struct {
    char *filename;
    double mean_perplexity;
    double perplexity_variance;
    double ai_score;
    int flagged;
} AiScore;

typedef struct {
    AiScore *items;
    int count;
} AiScoreList;

void ngram_model_init(NGramModel *model);
void ngram_model_free(NGramModel *model);
int train_ngram_model(const char *corpus_path, const StopwordSet *stopwords, const EngineConfig *config, NGramModel *model);
int score_ai_documents(const DocumentList *docs, const StopwordSet *stopwords, const NGramModel *model, const EngineConfig *config, AiScoreList *scores);
void ai_score_list_free(AiScoreList *scores);

#endif
