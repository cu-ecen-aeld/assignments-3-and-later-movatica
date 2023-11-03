#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...) fprintf(stdout, "threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) fprintf(stderr, "threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    struct thread_data* thread_args = (struct thread_data *) thread_param;
    
    DEBUG_LOG("started thread");

    usleep(thread_args->wait_to_obtain_ms * 1000);

    int rc = pthread_mutex_lock(thread_args->mutex);
    if (rc) {
        ERROR_LOG("failed to lock mutex [%d]", rc);
        return thread_param;
    }
    
    DEBUG_LOG("successfully locked mutex after %d ms", thread_args->wait_to_obtain_ms);

    usleep(thread_args->wait_to_release_ms * 1000);

    rc = pthread_mutex_unlock(thread_args->mutex);
    if (rc) {
        ERROR_LOG("failed to unlock mutex [%d]", rc);
        return thread_param;
    }
    
    DEBUG_LOG("successfully unlocked mutex after %d ms", thread_args->wait_to_release_ms);
    
    thread_args->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data * thread_param = (struct thread_data*) malloc(sizeof(struct thread_data));

    thread_param->mutex = mutex;
    thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_param->wait_to_release_ms = wait_to_release_ms;
    thread_param->thread_complete_success = false;

    int rc = pthread_create(thread, NULL, threadfunc, thread_param);

    if (rc) {
        ERROR_LOG("failed to create thread [%d]", rc);
        return false;
    }

    DEBUG_LOG("successfully created thread");

    return true;
}

