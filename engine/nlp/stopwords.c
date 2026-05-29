#include "nlp/stopwords.h"

#include "common/string_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void stopword_set_init(StopwordSet *set)
{
    set->words = NULL;
    set->count = 0;
    set->capacity = 0;
}

void stopword_set_free(StopwordSet *set)
{
    if (!set) {
        return;
    }
    for (int i = 0; i < set->count; i++) {
        free(set->words[i]);
    }
    free(set->words);
    stopword_set_init(set);
}

static void normalize_line(char *line)
{
    size_t len = strlen(line);

    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || isspace((unsigned char)line[len - 1]))) {
        line[--len] = '\0';
    }

    for (size_t i = 0; line[i] != '\0'; i++) {
        line[i] = (char)tolower((unsigned char)line[i]);
    }
}

int stopword_set_add(StopwordSet *set, const char *word)
{
    char **grown;
    char *copy;
    int new_capacity;

    if (!word || word[0] == '\0' || stopword_set_contains(set, word)) {
        return 1;
    }

    if (set->count == set->capacity) {
        new_capacity = set->capacity == 0 ? 64 : set->capacity * 2;
        grown = (char **)realloc(set->words, (size_t)new_capacity * sizeof(char *));
        if (!grown) {
            return 0;
        }
        set->words = grown;
        set->capacity = new_capacity;
    }

    copy = pargus_strdup(word);
    if (!copy) {
        return 0;
    }

    set->words[set->count++] = copy;
    return 1;
}

int stopword_set_contains(const StopwordSet *set, const char *word)
{
    if (!set || !word) {
        return 0;
    }

    for (int i = 0; i < set->count; i++) {
        if (strcmp(set->words[i], word) == 0) {
            return 1;
        }
    }

    return 0;
}

int stopword_set_load(StopwordSet *set, const char *path)
{
    FILE *file;
    char line[256];

    file = fopen(path, "rb");
    if (!file) {
        return 0;
    }

    while (fgets(line, sizeof(line), file)) {
        normalize_line(line);
        if (line[0] != '\0' && line[0] != '#') {
            if (!stopword_set_add(set, line)) {
                fclose(file);
                return 0;
            }
        }
    }

    fclose(file);
    return 1;
}

