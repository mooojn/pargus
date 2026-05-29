#include "nlp/ngram.h"

#include "common/string_utils.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FALLBACK_CORPUS "the quick brown fox jumps over the lazy dog. clear writing uses varied words and natural sentence rhythm. human essays often mix short sentences with longer reflective passages."

static void ngram_table_init(NGramTable *table)
{
    table->entries = NULL;
    table->count = 0;
    table->capacity = 0;
}

static void ngram_table_free(NGramTable *table)
{
    if (!table) {
        return;
    }
    for (int i = 0; i < table->count; i++) {
        free(table->entries[i].key);
    }
    free(table->entries);
    ngram_table_init(table);
}

static int ngram_table_find(const NGramTable *table, const char *key)
{
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->entries[i].key, key) == 0) {
            return i;
        }
    }
    return -1;
}

static int ngram_table_increment(NGramTable *table, const char *key)
{
    NGramEntry *grown;
    int new_capacity;
    int found = ngram_table_find(table, key);

    if (found >= 0) {
        table->entries[found].count++;
        return 1;
    }

    if (table->count == table->capacity) {
        new_capacity = table->capacity == 0 ? 128 : table->capacity * 2;
        grown = (NGramEntry *)realloc(table->entries, (size_t)new_capacity * sizeof(NGramEntry));
        if (!grown) {
            return 0;
        }
        table->entries = grown;
        table->capacity = new_capacity;
    }

    table->entries[table->count].key = pargus_strdup(key);
    if (!table->entries[table->count].key) {
        return 0;
    }
    table->entries[table->count].count = 1;
    table->count++;
    return 1;
}

static int read_file_text(const char *path, char **out_text)
{
    FILE *file;
    long size;
    char *buffer;
    size_t read_count;

    *out_text = NULL;
    if (!path || path[0] == '\0') {
        return 0;
    }

    file = fopen(path, "rb");
    if (!file) {
        return 0;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }
    size = ftell(file);
    if (size < 0) {
        fclose(file);
        return 0;
    }
    rewind(file);

    buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(file);
        return 0;
    }
    read_count = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    if (read_count != (size_t)size) {
        free(buffer);
        return 0;
    }

    buffer[size] = '\0';
    *out_text = buffer;
    return 1;
}

static int make_key(const TokenList *tokens, int start, int length, char **out_key)
{
    size_t total = 1;
    char *key;

    *out_key = NULL;
    for (int i = 0; i < length; i++) {
        total += strlen(tokens->tokens[start + i]) + 1;
    }

    key = (char *)malloc(total);
    if (!key) {
        return 0;
    }
    key[0] = '\0';

    for (int i = 0; i < length; i++) {
        if (i > 0) {
            strcat(key, "\x1f");
        }
        strcat(key, tokens->tokens[start + i]);
    }

    *out_key = key;
    return 1;
}

static int train_from_tokens(NGramModel *model, const TokenList *tokens)
{
    int order = model->order;

    model->total_tokens = tokens->count;
    for (int i = 0; i < tokens->count; i++) {
        if (!ngram_table_increment(&model->vocabulary, tokens->tokens[i])) {
            return 0;
        }
    }

    if (tokens->count < order) {
        return 1;
    }

    for (int i = 0; i <= tokens->count - order; i++) {
        char *ngram_key = NULL;
        char *context_key = NULL;

        if (!make_key(tokens, i, order, &ngram_key) ||
            !make_key(tokens, i, order - 1, &context_key) ||
            !ngram_table_increment(&model->ngrams, ngram_key) ||
            !ngram_table_increment(&model->contexts, context_key)) {
            free(ngram_key);
            free(context_key);
            return 0;
        }

        free(ngram_key);
        free(context_key);
    }

    return 1;
}

static double token_perplexity(const NGramModel *model, const TokenList *tokens)
{
    double negative_log_sum = 0.0;
    int observations = 0;
    int vocab_size = model->vocabulary.count > 0 ? model->vocabulary.count : 1;

    if (!tokens || tokens->count == 0) {
        return 1000.0;
    }

    if (tokens->count < model->order) {
        return 500.0 / (double)tokens->count;
    }

    for (int i = 0; i <= tokens->count - model->order; i++) {
        char *ngram_key = NULL;
        char *context_key = NULL;
        int ngram_index;
        int context_index;
        int ngram_count;
        int context_count;
        double probability;

        if (!make_key(tokens, i, model->order, &ngram_key) ||
            !make_key(tokens, i, model->order - 1, &context_key)) {
            free(ngram_key);
            free(context_key);
            return 1000.0;
        }

        ngram_index = ngram_table_find(&model->ngrams, ngram_key);
        context_index = ngram_table_find(&model->contexts, context_key);
        ngram_count = ngram_index >= 0 ? model->ngrams.entries[ngram_index].count : 0;
        context_count = context_index >= 0 ? model->contexts.entries[context_index].count : 0;
        probability = ((double)ngram_count + 1.0) / ((double)context_count + (double)vocab_size);
        negative_log_sum += -log(probability);
        observations++;

        free(ngram_key);
        free(context_key);
    }

    if (observations == 0) {
        return 1000.0;
    }

    return exp(negative_log_sum / (double)observations);
}

static double sentence_variance(const char *content, const StopwordSet *stopwords, const NGramModel *model, double mean)
{
    double sum_sq = 0.0;
    int count = 0;
    size_t start = 0;
    size_t len = content ? strlen(content) : 0;

    for (size_t i = 0; i <= len; i++) {
        unsigned char ch = (unsigned char)content[i];
        if (ch == '\0' || ch == '.' || ch == '!' || ch == '?') {
            size_t sentence_len = i - start;
            if (sentence_len > 0) {
                char *sentence = (char *)malloc(sentence_len + 1);
                if (sentence) {
                    TokenList tokens;
                    double ppl;
                    memcpy(sentence, content + start, sentence_len);
                    sentence[sentence_len] = '\0';
                    if (tokenize_text(sentence, stopwords, &tokens)) {
                        if (tokens.count > 0) {
                            ppl = token_perplexity(model, &tokens);
                            sum_sq += (ppl - mean) * (ppl - mean);
                            count++;
                        }
                        token_list_free(&tokens);
                    }
                    free(sentence);
                }
            }
            start = i + 1;
        }
    }

    return count > 1 ? sum_sq / (double)count : 0.0;
}

static double clamp_score(double value)
{
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 100.0) {
        return 100.0;
    }
    return value;
}

void ngram_model_init(NGramModel *model)
{
    ngram_table_init(&model->ngrams);
    ngram_table_init(&model->contexts);
    ngram_table_init(&model->vocabulary);
    model->order = 3;
    model->total_tokens = 0;
}

void ngram_model_free(NGramModel *model)
{
    if (!model) {
        return;
    }
    ngram_table_free(&model->ngrams);
    ngram_table_free(&model->contexts);
    ngram_table_free(&model->vocabulary);
    model->order = 0;
    model->total_tokens = 0;
}

int train_ngram_model(const char *corpus_path, const StopwordSet *stopwords, const EngineConfig *config, NGramModel *model)
{
    char *text = NULL;
    TokenList tokens;
    int ok;

    ngram_model_init(model);
    model->order = config->ngram < 2 ? 2 : config->ngram;

    if (!read_file_text(corpus_path, &text)) {
        text = pargus_strdup(FALLBACK_CORPUS);
    }
    if (!text) {
        ngram_model_free(model);
        return 0;
    }

    if (!tokenize_text(text, stopwords, &tokens)) {
        free(text);
        ngram_model_free(model);
        return 0;
    }

    ok = train_from_tokens(model, &tokens);
    token_list_free(&tokens);
    free(text);

    if (!ok || model->vocabulary.count == 0) {
        ngram_model_free(model);
        return 0;
    }

    return 1;
}

int score_ai_documents(const DocumentList *docs, const StopwordSet *stopwords, const NGramModel *model, const EngineConfig *config, AiScoreList *scores)
{
    memset(scores, 0, sizeof(*scores));
    scores->count = docs->count;
    scores->items = (AiScore *)calloc((size_t)docs->count, sizeof(AiScore));
    if (!scores->items && docs->count > 0) {
        return 0;
    }

#if defined(PARGUS_HAS_OPENMP)
    if (!config->serial_mode) {
        int ok = 1;
        #pragma omp parallel for schedule(dynamic) num_threads(config->threads) reduction(&&:ok)
        for (int i = 0; i < docs->count; i++) {
            TokenList tokens;
            double ppl;
            double variance;
            double ai_score;

            scores->items[i].filename = pargus_strdup(docs->items[i].filename);
            if (!scores->items[i].filename || !tokenize_text(docs->items[i].content, stopwords, &tokens)) {
                ok = 0;
                continue;
            }
            ppl = token_perplexity(model, &tokens);
            variance = sentence_variance(docs->items[i].content, stopwords, model, ppl);
            ai_score = 100.0 / (1.0 + (ppl / 75.0));
            ai_score *= 1.0 / (1.0 + (sqrt(variance) / 100.0));
            scores->items[i].mean_perplexity = ppl;
            scores->items[i].perplexity_variance = variance;
            scores->items[i].ai_score = clamp_score(ai_score);
            scores->items[i].flagged = scores->items[i].ai_score >= config->ai_threshold;
            token_list_free(&tokens);
        }
        if (!ok) {
            ai_score_list_free(scores);
            return 0;
        }
    } else
#endif
    {
        for (int i = 0; i < docs->count; i++) {
            TokenList tokens;
            double ppl;
            double variance;
            double ai_score;

            scores->items[i].filename = pargus_strdup(docs->items[i].filename);
            if (!scores->items[i].filename || !tokenize_text(docs->items[i].content, stopwords, &tokens)) {
                ai_score_list_free(scores);
                return 0;
            }
            ppl = token_perplexity(model, &tokens);
            variance = sentence_variance(docs->items[i].content, stopwords, model, ppl);
            ai_score = 100.0 / (1.0 + (ppl / 75.0));
            ai_score *= 1.0 / (1.0 + (sqrt(variance) / 100.0));
            scores->items[i].mean_perplexity = ppl;
            scores->items[i].perplexity_variance = variance;
            scores->items[i].ai_score = clamp_score(ai_score);
            scores->items[i].flagged = scores->items[i].ai_score >= config->ai_threshold;
            token_list_free(&tokens);
        }
    }

    return 1;
}

void ai_score_list_free(AiScoreList *scores)
{
    if (!scores) {
        return;
    }
    if (scores->items) {
        for (int i = 0; i < scores->count; i++) {
            free(scores->items[i].filename);
        }
    }
    free(scores->items);
    memset(scores, 0, sizeof(*scores));
}
