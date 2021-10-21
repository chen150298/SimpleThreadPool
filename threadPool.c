// Chen Larry

#include "threadPool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void Error(char *line, ThreadPool *threadPool) {
    int i;
    char *message = "Error in ";
    // free
    while (!osIsQueueEmpty(threadPool->tasks)) free((Task*) osDequeue(threadPool->tasks));
    for (i = 0; i < threadPool->numOfThreads; i++) pthread_cancel(threadPool->threads[i]);
    pthread_cond_destroy(&(threadPool->cond));
    pthread_mutex_destroy(&(threadPool->mutex));
    free(threadPool->threads);
    osDestroyQueue(threadPool->tasks);
    free(threadPool);
    // error message
    strcat(message, line);
    perror(message);
    exit(-1);
}

/** the function pulls a task from tasks queue and execute it */
void *execute(void *args) {
    Task* task;
    ThreadPool *tp = (ThreadPool *) args;

    // while should wait for task
    while (tp->pullTasksFlag == true) {
        pthread_mutex_lock(&(tp->mutex));
        // the queue is empty and tp is destroyed
        if (osIsQueueEmpty(tp->tasks) && tp->destroyFlag == true) {
            pthread_mutex_unlock(&(tp->mutex));
            break;
        }
        // wait for task
        if (osIsQueueEmpty(tp->tasks)) {
            if (pthread_cond_wait(&(tp->cond), &(tp->mutex)) != 0) {
                pthread_mutex_unlock(&(tp->mutex));
                Error("system call", tp);
            }
            pthread_mutex_unlock(&(tp->mutex));
            continue;
        }
        // pull task
        task = (Task*) osDequeue(tp->tasks);
        pthread_mutex_unlock(&(tp->mutex));
        // execute task
        task->func(task->args);
        free(task);
    }
}

ThreadPool *tpCreate(int numOfThreads) {
    int i,j;

    // create thread pool
    ThreadPool *threadPool = (ThreadPool *) malloc(sizeof(ThreadPool));
    if (threadPool == NULL) Error("system call", threadPool);

    // initialize fields

    // queue of tasks
    threadPool->tasks = osCreateQueue();
    // array of pthreads
    threadPool->threads = (pthread_t *) malloc(sizeof(pthread_t) * numOfThreads);
    if (threadPool->threads == NULL) {
        osDestroyQueue(threadPool->tasks);
        free(threadPool);
        perror("Error in system call");
        exit(-1);
    }
    // number of threads
    threadPool->numOfThreads = numOfThreads;
    // initialize mutex
    if (pthread_mutex_init(&(threadPool->mutex), NULL) != 0) {
        free(threadPool->threads);
        osDestroyQueue(threadPool->tasks);
        free(threadPool);
        perror("Error in system call");
        exit(-1);
    }
    // initialize cond
    if (pthread_cond_init(&(threadPool->cond), NULL) != 0) {
        pthread_mutex_destroy(&(threadPool->mutex));
        free(threadPool->threads);
        osDestroyQueue(threadPool->tasks);
        free(threadPool);
        perror("Error in system call");
        exit(-1);
    }
    // initialize destroy flag
    threadPool->destroyFlag = false;
    // initialize should wait for tasks flag
    threadPool->pullTasksFlag = true;

    // create pthreads
    for (i = 0; i < numOfThreads; i++) {
        if (pthread_create(&(threadPool->threads[i]), NULL, execute, threadPool) != 0) {
            for (j = 0; j < i; j++) {
                pthread_cancel(threadPool->threads[j]);
            }
            pthread_cond_destroy(&(threadPool->cond));
            pthread_mutex_destroy(&(threadPool->mutex));
            free(threadPool->threads);
            osDestroyQueue(threadPool->tasks);
            free(threadPool);
            perror("Error in system call");
            exit(-1);
        }
    }

    return threadPool;
}

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    int i;
    if (threadPool == NULL) {
        perror("Error in tpDestroy");
        exit(-1);
    }

    threadPool->destroyFlag = true;
    if (shouldWaitForTasks == 0) threadPool->pullTasksFlag = false;    // don't wait for tasks in queue

    // waiting for threads to finish
    for (i = 0; i < threadPool->numOfThreads; i++) {
        if (pthread_cond_broadcast(&(threadPool->cond)) != 0) Error("system call", threadPool);
        if (pthread_join(threadPool->threads[i], NULL) != 0) Error("system call", threadPool);
    }

    // free memory
    while (!osIsQueueEmpty(threadPool->tasks)) free((Task*) osDequeue(threadPool->tasks));
    pthread_cond_destroy(&(threadPool->cond));
    pthread_mutex_destroy(&(threadPool->mutex));
    free(threadPool->threads);
    osDestroyQueue(threadPool->tasks);
    free(threadPool);
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param){
    Task* task;
    if (threadPool == NULL) {
        perror("Error in tpInsertTask");
        exit(-1);
    }
    if (threadPool->destroyFlag == true) return -1;

    // create task
    task = (Task*) malloc(sizeof(Task));
    if (task == NULL) Error("system call", threadPool);
    task->func = computeFunc;
    task->args = param;
    // insert task
    pthread_mutex_lock(&(threadPool->mutex));
    osEnqueue(threadPool->tasks, (void *)task);
    if (pthread_cond_signal(&(threadPool->cond)) != 0) {
        pthread_mutex_unlock(&(threadPool->mutex));
        Error("system call", threadPool);
    }
    pthread_mutex_unlock(&(threadPool->mutex));
    return 0;
}
