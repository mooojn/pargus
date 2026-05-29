#include "nlp/tokenizer.h"

#include "common/string_utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void token_list_init(TokenList *list)
{
    list->tokens = NULL;
    list->count = 0;
    list->capacity = 0;
}

void token_list_free(TokenList *list)
{
    if (!list) {
        return;
    }
    for (int i = 0; i < list->count; i++) {
        free(list->tokens[i]);
    }
    free(list->tokens);
    token_list_init(list);
}

int token_list_add(TokenList *list, const char *token)
{
    char **grown;
    char *copy;
    int new_capacity;

    if (!token || token[0] == '\0') {
        return 1;
    }

    if (list->count == list->capacity) {
        new_capacity = list->capacity == 0 ? 32 : list->capacity * 2;
        grown = (char **)realloc(list->tokens, (size_t)new_capacity * sizeof(char *));
        if (!grown) {
            return 0;
        }
        list->tokens = grown;
        list->capacity = new_capacity;
    }

    copy = pargus_strdup(token);
    if (!copy) {
        return 0;
    }

    list->tokens[list->count++] = copy;
    return 1;
}

static int has_suffix(const char *token, const char *suffix)
{
    size_t token_len = strlen(token);
    size_t suffix_len = strlen(suffix);

    return token_len > suffix_len && strcmp(token + token_len - suffix_len, suffix) == 0;
}

void normalize_token(char *token)
{
    size_t len;

    for (size_t i = 0; token[i] != '\0'; i++) {
        token[i] = (char)tolower((unsigned char)token[i]);
    }

    len = strlen(token);
    if (len > 5 && has_suffix(token, "ing")) {
        token[len - 3] = '\0';
    } else if (len > 4 && has_suffix(token, "ed")) {
        token[len - 2] = '\0';
    } else if (len > 4 && has_suffix(token, "es")) {
        token[len - 2] = '\0';
    } else if (len > 3 && has_suffix(token, "s")) {
        token[len - 1] = '\0';
    }
}

int tokenize_text(const char *text, const StopwordSet *stopwords, TokenList *tokens)
{
    char buffer[256];
    int length = 0;

    token_list_init(tokens);

    if (!text) {
        return 1;
    }

    for (size_t i = 0;; i++) {
        unsigned char ch = (unsigned char)text[i];
        int is_token_char = isalnum(ch);

        if (is_token_char && length < (int)sizeof(buffer) - 1) {
            buffer[length++] = (char)ch;
        } else if (length > 0) {
            buffer[length] = '\0';
            normalize_token(buffer);
            if (!stopword_set_contains(stopwords, buffer)) {
                if (!token_list_add(tokens, buffer)) {
                    token_list_free(tokens);
                    return 0;
                }
            }
            length = 0;
        }

        if (ch == '\0') {
            break;
        }
    }

    return 1;
}

