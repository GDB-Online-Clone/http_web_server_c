#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

struct job {
    void (*function)(void* arg);
    void* arg;
    struct job* next;
};

// 작업 큐를 정의
struct job_queue {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    struct job* front;
    struct job* rear;
    int job_count;
};

// 스레드 풀
struct threadpool {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t* threads;
    struct job_queue* job_queue;
    _Atomic bool stop;
    int max_threads;
    atomic_bool active_threads;
    void* data;
};

void job_enqueue(struct job_queue* queue, struct job* job);

struct job* job_dequeue(struct job_queue* queue);

// 워커 스레드 함수
void* worker_thread(void* arg);

// 스레드 풀 초기화
struct threadpool* threadpool_create(int num_threads);

// 작업 추가
void threadpool_add_job(struct threadpool* pool, void (*function)(void*), void* arg);

/**
 * @brief Cleanup struct threadpool
 * 
 * @param pool 
 */
void threadpool_destroy(struct threadpool* pool);