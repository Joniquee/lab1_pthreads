#ifndef IDK_H
#define IDK_H

#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t readers_cond;
    pthread_cond_t writers_cond;
    unsigned active_readers;
    unsigned waiting_readers;
    unsigned waiting_writers;
    int writer_active;
} custom_rwlock_t;

int custom_rwlock_init(custom_rwlock_t* lock);
int custom_rwlock_destroy(custom_rwlock_t* lock);
int custom_rwlock_rdlock(custom_rwlock_t* lock);
int custom_rwlock_wrlock(custom_rwlock_t* lock);
int custom_rwlock_unlock(custom_rwlock_t* lock);

#endif
