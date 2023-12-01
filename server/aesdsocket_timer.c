#include "aesdsocket_timer.h"

#include <signal.h>
#include <stdio.h>
#include <syslog.h>


extern char *tmpfilename;


/*
 * Timer thread function to write timestamps
 **/
static void timer_thread(union sigval sigev_value) {
    pthread_mutex_t *tmpfile_lock = (pthread_mutex_t *)sigev_value.sival_ptr;

    char timestamp[64];
    time_t now = time(NULL);
    struct tm *local_now = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %T %z\n", local_now);

    syslog(LOG_INFO, "Writing %s to tmpfile\n", timestamp);

    pthread_mutex_lock(tmpfile_lock);

    FILE *tmpfile = fopen(tmpfilename, "a");
    fputs(timestamp, tmpfile);
    fclose(tmpfile);

    pthread_mutex_unlock(tmpfile_lock);
}


/*
 * Create timer that spawns a timestamp writer thread every 10 seconds
 **/
int create_timestamp_timer(timer_t *timer_id, pthread_mutex_t *tmpfile_lock) {
    struct sigevent sigev = { 0 };
    struct itimerspec timespec = {
        .it_value.tv_sec = 0,
        .it_value.tv_nsec = 1,
        .it_interval.tv_sec = 10,
        .it_interval.tv_nsec = 0,
    };

    sigev.sigev_notify = SIGEV_THREAD;
    sigev.sigev_notify_function = &timer_thread;
    sigev.sigev_value.sival_ptr = tmpfile_lock;

    int result = timer_create(CLOCK_REALTIME, &sigev, timer_id);

    if (result == 0) {
        result = timer_settime(*timer_id, 0, &timespec, NULL);
    }

    return result;
}
