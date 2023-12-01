#ifndef AESDSOCKET_THREADLIST_H
#define AESDSOCKET_THREADLIST_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>

/*
 * Node struct for thread list
 **/
struct threadlist_node_t {
    pthread_t thread_id;
    struct threadlist_node_t *next;
};

struct threadlist_node_t * threadlist_node_create();

void threadlist_attach(struct threadlist_node_t **head, struct threadlist_node_t *newborn);

void threadlist_cleanup(struct threadlist_node_t **head);

#endif//AESDSOCKET_THREADLIST_H
