#include "aesdsocket_threadlist.h"

#include <stdlib.h>
#include <string.h>


/*
 * Create and zero-init a threadlist node
 */
struct threadlist_node_t * threadlist_node_create() {
    struct threadlist_node_t *node = malloc(sizeof(struct threadlist_node_t));
    
    if (node != NULL) {
        memset(node, 0, sizeof(struct threadlist_node_t));
    }

    return node;
}


/*
 * Iterate over the nodes in a threadlist
 * Remove finished threads
 * Append the new thread to the end
 **/
void threadlist_attach(struct threadlist_node_t **node, struct threadlist_node_t *newborn) {
    while (*node != NULL) {
        struct threadlist_node_t *child = *node;

        if (pthread_tryjoin_np(child->thread_id, NULL) == 0) {
            /* Cleanup finished thread */
            *node = child->next;
            free(child);
        }
        else {
            node = &(child->next);
        }
    }

    *node = newborn;
}


/*
 * Wait for all remaining threads to end
 * Cleanup the threadlist
 **/
void threadlist_cleanup(struct threadlist_node_t **head) {
    for (struct threadlist_node_t *node = *head; node != NULL; ) {
        pthread_join(node->thread_id, NULL);
        struct threadlist_node_t *next = node->next;
        free(node);
        node = next;
    }

    *head = NULL;
}
