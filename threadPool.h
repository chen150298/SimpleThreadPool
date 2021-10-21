// Chen Larry 209192798

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.h"
#include <pthread.h>

typedef enum {
    false, true
} bool;

typedef struct thread_pool
{
 OSQueue *tasks;                    // queue of tasks
 pthread_t *threads;                // array of threads
 int numOfThreads;
 pthread_mutex_t mutex;
 pthread_cond_t cond;               // condition variable
 bool destroyFlag;
 bool pullTasksFlag;
}ThreadPool;

typedef struct task{
    void (*func)(void *);
    void *args;
}Task;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
