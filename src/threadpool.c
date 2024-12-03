#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "threadpool.h"


// 작업을 스레드 폴까지 전달하는 헬퍼 함수
void job_enqueue(struct job_queue* queue, struct job* job) {
    pthread_mutex_lock(&queue->lock);

    // @TODO 동적 큐 대신 고정 크기 배열을 이용하면 lock 을 짧게 가져갈 수 있을 것 같음
    if (queue->rear == NULL) {
        queue->front = job;
        queue->rear = job;
    } else {
        queue->rear->next = job;
        queue->rear = job;
    }
    queue->job_count++;
    pthread_cond_signal(&queue->cond);
    
    pthread_mutex_unlock(&queue->lock);
}

struct job* job_dequeue(struct job_queue* queue) {
    pthread_mutex_lock(&queue->lock);

    // @TODO job_count 를 통해 대기하면 lock 을 더 짧게 가져갈 수 있으며, _Atomic 도 사용 가능
    while (queue->front == NULL) {
        pthread_cond_wait(&queue->cond, &queue->lock);
    }

    struct job* job = queue->front;
    queue->front = job->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    queue->job_count--;

    pthread_mutex_unlock(&queue->lock);
    return job;
}

// 워커 스레드 함수
void* worker_thread(void* arg) {
    struct threadpool* pool = (struct threadpool*)arg;
    while (true) {
        pthread_mutex_lock(&pool->lock);
        while (pool->job_queue->front == NULL && !pool->stop) {
            pthread_cond_wait(&pool->cond, &pool->lock);
        }
        if (pool->stop) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }
        struct job* job = job_dequeue(pool->job_queue);
        pool->active_threads++;
        pthread_mutex_unlock(&pool->lock);

        if (job) {
            job->function(job->arg);
            free(job);
        }

        pthread_mutex_lock(&pool->lock);
        pool->active_threads--;
        pthread_cond_signal(&pool->cond);
        pthread_mutex_unlock(&pool->lock);
    }
    return NULL;
}

// 스레드 풀 초기화
struct threadpool* threadpool_create(int num_threads) {
    struct threadpool* pool = (struct threadpool*)malloc(sizeof(struct threadpool));
    pool->max_threads = num_threads;
    pool->active_threads = 0;
    pool->stop = false;
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);
    pool->job_queue = (struct job_queue*)malloc(sizeof(struct job_queue));
    pool->job_queue->front = pool->job_queue->rear = NULL;
    pool->job_queue->job_count = 0;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);
    pthread_mutex_init(&pool->job_queue->lock, NULL);
    pthread_cond_init(&pool->job_queue->cond, NULL);

    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&pool->threads[i], NULL, worker_thread, (void*)pool);
    }

    return pool;
}

// 작업 추가
void threadpool_add_job(struct threadpool* pool, void (*function)(void*), void* arg) {    
    struct job* job = (struct job*)malloc(sizeof(struct job));
    job->function = function;
    job->arg = arg;
    job->next = NULL;
    
    pthread_mutex_lock(&pool->lock);
    while ((pool->active_threads == pool->max_threads)) {
        pthread_cond_wait(&pool->cond, &pool->lock);
    }
    pthread_mutex_unlock(&pool->lock);

    job_enqueue(pool->job_queue, job);
    pthread_cond_signal(&pool->cond);
}

/**
 * @brief Cleanup struct threadpool
 * 
 * @param pool 
 */
void threadpool_destroy(struct threadpool* pool) {
    pthread_mutex_lock(&pool->lock);
    pool->stop = true;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < pool->max_threads; ++i) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);

    while (pool->job_queue->front != NULL) {
        struct job* job = job_dequeue(pool->job_queue);
        free(job);
    }
    
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
    pthread_mutex_destroy(&pool->job_queue->lock);
    pthread_cond_destroy(&pool->job_queue->cond);
    free(pool->job_queue);
    free(pool);
}
