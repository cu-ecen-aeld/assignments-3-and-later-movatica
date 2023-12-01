#ifndef AESDSOCKET_CONNECTIONHANDLER_H
#define AESDSOCKET_CONNECTIONHANDLER_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>

/*
 * Param struct for connection handler threads
 **/
struct connection_handler_args_t {
    int socket_id;
    char *client_ip;
    pthread_mutex_t *tmpfile_lock;
};

struct connection_handler_args_t *connection_handler_create_args(int socket_id, char *client_ip, pthread_mutex_t *tmpfile_lock);

void connection_handler_destroy_args(struct connection_handler_args_t **args);

/*
 * Thread function to handle incoming connections
 **/
void *connection_handler(void *connection_handler_args);

#endif//AESDSOCKET_CONNECTIONHANDLER_H
