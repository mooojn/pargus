#ifndef PARGUS_DOC_QUEUE_H
#define PARGUS_DOC_QUEUE_H

#include "common/types.h"

#ifndef _WIN32
#include <pthread.h>
#endif

typedef struct {
    Document *items;
    int capacity;
    int head;
    int tail;
    int count;
    int closed;
#ifndef _WIN32
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
#endif
} DocQueue;

int doc_queue_init(DocQueue *queue, int capacity);
void doc_queue_close(DocQueue *queue);
void doc_queue_free(DocQueue *queue);
int doc_queue_push(DocQueue *queue, Document doc);
int doc_queue_pop(DocQueue *queue, Document *doc);

#endif
