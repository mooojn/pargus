#ifndef PARGUS_WORK_QUEUE_H
#define PARGUS_WORK_QUEUE_H

#include "nlp/lsh.h"

#ifndef _WIN32
#include <pthread.h>
#endif

typedef struct {
    DocPair *items;
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
} WorkQueue;

int work_queue_init(WorkQueue *queue, int capacity);
void work_queue_close(WorkQueue *queue);
void work_queue_free(WorkQueue *queue);
int work_queue_push(WorkQueue *queue, DocPair pair);
int work_queue_pop(WorkQueue *queue, DocPair *pair);

#endif
