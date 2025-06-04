#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../include/threadpool.h"

ThreadPool *threadpool_create(int thread_count, int queue_capacity)
{
    if (thread_count <= 0 || queue_capacity <= 0)
    {
        fprintf(stderr, "Invalid thread pool parameters\n");
        return NULL;
    }

    ThreadPool *pool = malloc(sizeof(ThreadPool));
    if (!pool)
    {
        fprintf(stderr, "Failed to allocate memory for thread pool\n");
        return NULL;
    }

    // Initialize pool structure
    memset(pool, 0, sizeof(ThreadPool));
    pool->thread_count = thread_count;
    pool->queue_capacity = queue_capacity;
    pool->queue_size = 0;
    pool->queue_front = 0;
    pool->queue_rear = 0;
    pool->shutdown = false;
    pool->started = false;

    pool->queue = malloc(sizeof(ThreadPoolTask) * queue_capacity);
    if (!pool->queue)
    {
        fprintf(stderr, "Failed to allocate memory for task queue\n");
        free(pool);
        return NULL;
    }

    // Initialize task queue
    memset(pool->queue, 0, sizeof(ThreadPoolTask) * queue_capacity);

    // Allocate memory for thread array
    pool->threads = malloc(sizeof(pthread_t) * thread_count);
    if (!pool->threads)
    {
        fprintf(stderr, "Failed to allocate memory for threads\n");
        free(pool->queue);
        free(pool);
        return NULL;
    }

    // Initialize mutex and condition variables
    if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0)
    {
        fprintf(stderr, "Failed to initialize queue mutex\n");
        free(pool->threads);
        free(pool->queue);
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->queue_not_empty, NULL) != 0)
    {
        fprintf(stderr, "Failed to initialize queue_not_empty condition\n");
        pthread_mutex_destroy(&pool->queue_mutex);
        free(pool->threads);
        free(pool->queue);
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->queue_not_full, NULL) != 0)
    {
        fprintf(stderr, "Failed to initialize queue_not_full condition\n");
        pthread_cond_destroy(&pool->queue_not_empty);
        pthread_mutex_destroy(&pool->queue_mutex);
        free(pool->threads);
        free(pool->queue);
        free(pool);
        return NULL;
    }

    // Create worker threads
    for (int i = 0; i < thread_count; i++)
    {
        if (pthread_create(&pool->threads[i], NULL, threadpool_worker, pool) != 0)
        {
            fprintf(stderr, "Failed to create worker thread %d\n", i);
            // Set shutdown flag and wait for already created threads
            pool->shutdown = true;
            pool->started = true; // Mark as started so cleanup works
            for (int j = 0; j < i; j++)
            {
                pthread_join(pool->threads[j], NULL);
            }
            threadpool_destroy(pool);
            return NULL;
        }
    }

    pool->started = true;
    printf("Thread pool created with %d threads and queue capacity of %d\n",
           thread_count, queue_capacity);

    return pool;
}

int threadpool_add_task(ThreadPool *pool, void (*function)(void *), void *arg)
{
    if (!pool || !function)
    {
        return -1;
    }

    pthread_mutex_lock(&pool->queue_mutex);

    // Check if pool is shutting down
    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->queue_mutex);
        return -1;
    }

    // Wait for space in queue if it's full
    while (pool->queue_size == pool->queue_capacity && !pool->shutdown)
    {
        pthread_cond_wait(&pool->queue_not_full, &pool->queue_mutex);
    }

    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->queue_mutex);
        return -1;
    }

    // Add task to queue
    pool->queue[pool->queue_rear].function = function;
    pool->queue[pool->queue_rear].arg = arg;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_capacity;
    pool->queue_size++;

    // Signal that queue is not empty
    pthread_cond_signal(&pool->queue_not_empty);
    pthread_mutex_unlock(&pool->queue_mutex);

    return 0;
}

void *threadpool_worker(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    ThreadPoolTask task;

    while (1)
    {
        // Initialize task
        task.function = NULL;
        task.arg = NULL;

        pthread_mutex_lock(&pool->queue_mutex);

        // Wait for tasks or shutdown signal
        while (pool->queue_size == 0 && !pool->shutdown)
        {
            pthread_cond_wait(&pool->queue_not_empty, &pool->queue_mutex);
        }

        // Check if we should exit
        if (pool->shutdown && pool->queue_size == 0)
        {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }

        // Get task from queue
        if (pool->queue_size > 0)
        {
            task = pool->queue[pool->queue_front];
            // Clear the queue slot to avoid stale references
            pool->queue[pool->queue_front].function = NULL;
            pool->queue[pool->queue_front].arg = NULL;

            pool->queue_front = (pool->queue_front + 1) % pool->queue_capacity;
            pool->queue_size--;

            // Signal that queue is not full
            pthread_cond_signal(&pool->queue_not_full);
        }

        pthread_mutex_unlock(&pool->queue_mutex);

        if (task.function)
        {
            task.function(task.arg);
        }
    }

    return NULL;
}

int threadpool_destroy(ThreadPool *pool)
{
    if (!pool)
    {
        return -1;
    }

    pthread_mutex_lock(&pool->queue_mutex);

    // Only set shutdown if not already set
    if (!pool->shutdown)
    {
        pool->shutdown = true;
    }

    // Wake up all waiting threads
    pthread_cond_broadcast(&pool->queue_not_empty);
    pthread_cond_broadcast(&pool->queue_not_full);

    pthread_mutex_unlock(&pool->queue_mutex);

    // Wait for all threads to finish
    if (pool->started && pool->threads)
    {
        for (int i = 0; i < pool->thread_count; i++)
        {
            if (pthread_join(pool->threads[i], NULL) != 0)
            {
                fprintf(stderr, "Warning: Failed to join thread %d\n", i);
            }
        }
    }

    // Cleanup resources
    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_not_empty);
    pthread_cond_destroy(&pool->queue_not_full);

    if (pool->threads)
    {
        free(pool->threads);
    }

    if (pool->queue)
    {
        free(pool->queue);
    }

    free(pool);

    printf("Thread pool destroyed\n");
    return 0;
}