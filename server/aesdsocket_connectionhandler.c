#include "aesdsocket_connectionhandler.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>


extern const char *tmpfilename;


/*
 * Thread internal dynamic resources that need cleanup on exit.
 **/
struct connection_handler_res {
    FILE *socket;
    FILE *tmpfile;
    pthread_mutex_t *tmpfile_lock;
    char *client_ip;
};


/*
 * Cleanup all thread resources.
 **/
static void destroy_connection_handler_res(void *args) {
    struct connection_handler_res *res = (struct connection_handler_res *)args;

    if (res->socket) fclose(res->socket);
    if (res->tmpfile) fclose(res->tmpfile);
    if (res->tmpfile_lock) pthread_mutex_unlock(res->tmpfile_lock);
    if (res->client_ip) free(res->client_ip);
}


/* 
 * Copy line from instream to outstream.
 * Return number of bytes copied or -1 on error.
 **/
static ssize_t transfer_line(FILE *instream, FILE *outstream) {
    char *buffer = NULL;
    size_t buflen = 0;
    ssize_t result;

    result = getline(&buffer, &buflen, instream);

    if (result > 0) {
        fputs(buffer, outstream);
        fflush(outstream);
    }

    free(buffer);

    return result;
}


/*
 * Thread function to handle incoming connections
 **/
void *connection_handler(void *connection_handler_args) {
    struct connection_handler_args_t *args = (struct connection_handler_args_t *)connection_handler_args;

    struct connection_handler_res res = { 0 };
    res.tmpfile_lock = args->tmpfile_lock;
    res.client_ip = args->client_ip;

    pthread_cleanup_push(destroy_connection_handler_res, &res);
    
    res.socket = fdopen(args->socket_id, "a+");

    free(args); args = NULL;

    if (res.socket == NULL) {
        pthread_exit(NULL);
    }

    pthread_mutex_lock(res.tmpfile_lock);
    res.tmpfile = fopen(tmpfilename, "a");

    if (res.tmpfile == NULL) {
        pthread_exit(NULL);
    }

    setlinebuf(res.tmpfile);
    setlinebuf(res.socket);

    ssize_t transres = transfer_line(res.socket, res.tmpfile);

    res.tmpfile = freopen(tmpfilename, "r", res.tmpfile);
    pthread_mutex_unlock(res.tmpfile_lock);
    
    if (transres == -1) {
        syslog(LOG_PERROR, "Error receiving from %s", res.client_ip);
        pthread_exit(NULL);
    }
    else {
        syslog(LOG_DEBUG, "Received %ld bytes from %s", transres, res.client_ip);
    }

    if (res.tmpfile == NULL) {
        pthread_exit(NULL);
    }

    setlinebuf(res.tmpfile);

    ssize_t transsum = 0;
    while ((transres = transfer_line(res.tmpfile, res.socket)) > 0) {
        transsum += transres;
    }

    if (transsum > 0) {
        syslog(LOG_DEBUG, "Sent %ld bytes to %s", transsum, res.client_ip);
    }
    else {
        syslog(LOG_PERROR, "Error sending to %s", res.client_ip);
    }

    syslog(LOG_INFO, "Closed connection from %s", res.client_ip);

    pthread_cleanup_pop(1);

    return NULL;
}
