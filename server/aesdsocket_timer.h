#ifndef AESDSOCKET_TIMER_H
#define AESDSOCKET_TIMER_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>
#include <time.h>

int create_timestamp_timer(timer_t *timer_id, pthread_mutex_t *tmpfile_lock);


#endif//AESDSOCKET_TIMER_H
