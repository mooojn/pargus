#include "parallel/work_queue.h"

#include <stdlib.h>
#include <string.h>

int work_queue_init(WorkQueue *queue, int capacity)
{
    memset(queue, 0, sizeof(*queue));
    queue->capacity = capacity > 0 ? capacity : 1;
    queue->items = (DocPair *)calloc((size_t)queue->capacity, sizeof(DocPair));
    if (!queue->items) {
        return 0;
    }
#ifndef _WIN32
    if (pthread_mutex_init(&queue->mutex, NULL) != 0 ||
        pthread_cond_init(&queue->not_empty, NULL) != 0 ||
        pthread_cond_init(&queue->not_full, NULL) != 0) {
        free(queue->items);
        memset(queue, 0, sizeof(*queue));
        return 0;
    }
#endif
    return 1;
}

void work_queue_close(WorkQueue *queue)
{
#ifndef _WIN32
    pthread_mutex_lock(&queue->mutex);
#endif
    queue->closed = 1;
#ifndef _WIN32
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
#endif
}

void work_queue_free(WorkQueue *queue)
{
    if (!queue) {
        return;
    }
    free(queue->items);
#ifndef _WIN32
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
#endif
    memset(queue, 0, sizeof(*queue));
}

int work_queue_push(WorkQueue *queue, DocPair pair)
{
#ifdef _WIN32
    (void)queue;
    (void)pair;
    return 0;
#else
    pthread_mutex_lock(&queue->mutex);
    while (!queue->closed && queue->count == queue->capacity) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    if (queue->closed) {
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }
    queue->items[queue->tail] = pair;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    return 1;
#endif
}

int work_queue_pop(WorkQueue *queue, DocPair *pair)
{
#ifdef _WIN32
    (void)queue;
    (void)pair;
    return 0;
#else
    pthread_mutex_lock(&queue->mutex);
    while (!queue->closed && queue->count == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    if (queue->count == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }
    *pair = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    return 1;
#endif
}
