#ifndef PARGUS_TYPES_H
#define PARGUS_TYPES_H

#include <stddef.h>

typedef struct {
    int doc_id;
    char *path;
    char *filename;
    char *content;
    size_t length;
} Document;

typedef struct {
    Document *items;
    int count;
    int capacity;
} DocumentList;

typedef struct {
    double io_ms;
    double tokenize_tfidf_ms;
    double minhash_lsh_ms;
    double write_ms;
    double total_ms;
    int total_pairs;
    int candidate_pairs;
} StageTimings;

void document_free(Document *doc);
void document_list_init(DocumentList *list);
void document_list_free(DocumentList *list);

#endif
