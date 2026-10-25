#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // initialize the success flag to false; set it to true only after all operations succeed
    thread_func_args->thread_complete_success = false;

    //  sleep wait_to_obtain_ms milliseconds before acquiring the mutex
    if (usleep(thread_func_args->wait_to_obtain_ms * 1000) != 0) {
        ERROR_LOG("usleep failed before obtaining mutex");
        return thread_param;
    }

    // acquire the mutex
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("pthread_mutex_lock failed");
        return thread_param;
    }

    // hold the mutex for wait_to_release_ms milliseconds
    if (usleep(thread_func_args->wait_to_release_ms * 1000) != 0) {
        ERROR_LOG("usleep failed before releasing mutex");
        pthread_mutex_unlock(thread_func_args->mutex);
        return thread_param;
    }

    // release the mutex
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        ERROR_LOG("pthread_mutex_unlock failed");
        return thread_param;
    }

    // all operations completed successfully
    thread_func_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,
                                  int wait_to_obtain_ms, int wait_to_release_ms)
{
    // dynamically allocate thread_data so the thread can return it after completion for the caller to free
    struct thread_data* thread_data = malloc(sizeof(struct thread_data));
    if (thread_data == NULL) {
        ERROR_LOG("malloc failed for thread_data");
        return false;
    }

    // fill in the thread parameters
    thread_data->mutex = mutex;
    thread_data->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data->wait_to_release_ms = wait_to_release_ms;
    thread_data->thread_complete_success = false;

    // create the thread; if it fails, free the memory and return false
    if (pthread_create(thread, NULL, threadfunc, thread_data) != 0) {
        ERROR_LOG("pthread_create failed");
        free(thread_data);
        return false;
    }

    return true;
}

