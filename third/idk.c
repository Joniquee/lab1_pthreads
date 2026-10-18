#include "idk.h"

int custom_rwlock_init(custom_rwlock_t* lock) {
    int rc = pthread_mutex_init(&lock->mutex, NULL);
    if (rc != 0)
        return rc;

    rc = pthread_cond_init(&lock->readers_cond, NULL);
    if (rc != 0) {
        pthread_mutex_destroy(&lock->mutex);
        return rc;
    }

    rc = pthread_cond_init(&lock->writers_cond, NULL);
    if (rc != 0) {
        pthread_cond_destroy(&lock->readers_cond);
        pthread_mutex_destroy(&lock->mutex);
        return rc;
    }

    lock->active_readers = 0;
    lock->waiting_readers = 0;
    lock->waiting_writers = 0;
    lock->writer_active = 0;
    return 0;
}

int custom_rwlock_destroy(custom_rwlock_t* lock) {
    int rc = pthread_cond_destroy(&lock->readers_cond);
    if (rc != 0)
        return rc;

    rc = pthread_cond_destroy(&lock->writers_cond);
    if (rc != 0)
        return rc;

    return pthread_mutex_destroy(&lock->mutex);
}

int custom_rwlock_rdlock(custom_rwlock_t* lock) {
    int rc = pthread_mutex_lock(&lock->mutex);
    if (rc != 0)
        return rc;

    ++lock->waiting_readers;
    while (lock->writer_active || lock->waiting_writers > 0) {
        rc = pthread_cond_wait(&lock->readers_cond, &lock->mutex);
        if (rc != 0) {
            --lock->waiting_readers;
            pthread_mutex_unlock(&lock->mutex);
            return rc;
        }
    }
    --lock->waiting_readers;
    ++lock->active_readers;

    return pthread_mutex_unlock(&lock->mutex);
}

int custom_rwlock_wrlock(custom_rwlock_t* lock) {
    int rc = pthread_mutex_lock(&lock->mutex);
    if (rc != 0)
        return rc;

    ++lock->waiting_writers;
    while (lock->writer_active || lock->active_readers > 0) {
        rc = pthread_cond_wait(&lock->writers_cond, &lock->mutex);
        if (rc != 0) {
            --lock->waiting_writers;
            pthread_mutex_unlock(&lock->mutex);
            return rc;
        }
    }
    --lock->waiting_writers;
    lock->writer_active = 1;

    return pthread_mutex_unlock(&lock->mutex);
}

int custom_rwlock_unlock(custom_rwlock_t* lock) {
    int rc = pthread_mutex_lock(&lock->mutex);
    if (rc != 0)
        return rc;

    if (lock->writer_active) {
        lock->writer_active = 0;
        if (lock->waiting_writers > 0)
            rc = pthread_cond_signal(&lock->writers_cond);
        else
            rc = pthread_cond_broadcast(&lock->readers_cond);
    } else if (lock->active_readers > 0) {
        --lock->active_readers;
        if (lock->active_readers == 0 && lock->waiting_writers > 0)
            rc = pthread_cond_signal(&lock->writers_cond);
    }

    {
        int unlock_rc = pthread_mutex_unlock(&lock->mutex);
        return rc != 0 ? rc : unlock_rc;
    }
}
