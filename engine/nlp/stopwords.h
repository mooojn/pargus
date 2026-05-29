#ifndef PARGUS_STOPWORDS_H
#define PARGUS_STOPWORDS_H

typedef struct {
    char **words;
    int count;
    int capacity;
} StopwordSet;

void stopword_set_init(StopwordSet *set);
void stopword_set_free(StopwordSet *set);
int stopword_set_add(StopwordSet *set, const char *word);
int stopword_set_contains(const StopwordSet *set, const char *word);
int stopword_set_load(StopwordSet *set, const char *path);

#endif

