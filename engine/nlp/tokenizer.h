#ifndef PARGUS_TOKENIZER_H
#define PARGUS_TOKENIZER_H

#include "nlp/stopwords.h"

typedef struct {
    char **tokens;
    int count;
    int capacity;
} TokenList;

void token_list_init(TokenList *list);
void token_list_free(TokenList *list);
int token_list_add(TokenList *list, const char *token);
int tokenize_text(const char *text, const StopwordSet *stopwords, TokenList *tokens);
void normalize_token(char *token);

#endif

