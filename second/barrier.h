#ifndef MY_BARRIER_H
#define MY_BARRIER_H

#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int count;
    int total;
    int generation;
} my_barrier_t;

static inline void barrier_init(my_barrier_t* b, int total) {
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->count = 0;
    b->total = total;
    b->generation = 0;
}

static inline void barrier_wait(my_barrier_t* b) {
    pthread_mutex_lock(&b->mutex);
    int gen = b->generation;
    b->count++;
    if (b->count == b->total) {
        b->generation++;
        b->count = 0;
        pthread_cond_broadcast(&b->cond);
    } else {
        while (gen == b->generation)
            pthread_cond_wait(&b->cond, &b->mutex);
    }
    pthread_mutex_unlock(&b->mutex);
}

static inline void barrier_destroy(my_barrier_t* b) {
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->cond);
}

#endif