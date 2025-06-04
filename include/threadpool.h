#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stdbool.h>

typedef struct
{
    void (*function)(void *arg);
    void *arg;
} ThreadPoolTask;

typedef struct
{
    ThreadPoolTask *queue;
    int queue_size;
    int queue_capacity;
    int queue_front;
    int queue_rear;

    pthread_t *threads;
    int thread_count;

    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_not_empty;
    pthread_cond_t queue_not_full;

    bool shutdown;
    bool started;
} ThreadPool;

// Function declarations
ThreadPool *threadpool_create(int thread_count, int queue_capacity);
int threadpool_add_task(ThreadPool *pool, void (*function)(void *), void *arg);
int threadpool_destroy(ThreadPool *pool);
void *threadpool_worker(void *arg);

#endif // THREADPOOL_H