#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif 

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "aesdsocket_connectionhandler.h"
#include "aesdsocket_threadlist.h"
#include "aesdsocket_timer.h"

/* 
 * Config
 **/
static const char *syslog_ident = "aesdsocket";
const char *default_port = "9000";
const char *tmpfilename = "/var/tmp/aesdsocketdata";


/*
 * Globals
 **/
volatile bool _doexit = false; /* controls the server loop */


/*
 * Get the ip address from a sockaddr struct as string.
 * Allocate output parameter `str` accordingly.
 **/
char *get_addr_str(struct sockaddr *sa) {
    char *str;

    switch(sa->sa_family) {
        case AF_INET: { /* ipv4 */
            struct in_addr ina = ((struct sockaddr_in*)sa)->sin_addr;
            str = calloc(sizeof(char), INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &ina, str, INET_ADDRSTRLEN);
            break; }
        case AF_INET6: {/* ipv6 */
            struct in6_addr in6a = ((struct sockaddr_in6*)sa)->sin6_addr;
            str = calloc(sizeof(char), INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &in6a, str, INET6_ADDRSTRLEN);
            break; }
        default: /* should not happen */
            return "";
    }

    return str;
}


/*
 * Create a socket that binds to the specified port.
 */
int bind_to_port(const char* port) {
    int result, sock;
    struct addrinfo hints = {0}, *sockinfo = NULL, *si;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, port, &hints, &sockinfo);

    if (result != 0) {
        syslog(LOG_PERROR, "getaddrinfo: %s\n", gai_strerror(result));
        return -1;
    }

    for (si = sockinfo; si != NULL; si = si->ai_next) {
        sock = socket(si->ai_family, si->ai_socktype, si->ai_protocol);

        if (sock == -1)
            continue;

        const int enable = 1; /* boolean integer flag */
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
        setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

        if (bind(sock, si->ai_addr, si->ai_addrlen) == 0)
            break;

        close(sock);
    }

    if (si == NULL) {
        syslog(LOG_PERROR, "Error binding to port %s!\n", port);
        sock = -1;
    }
    else {
        char *sockaddr = get_addr_str(si->ai_addr);
        syslog(LOG_INFO, "Socket bound to %s\n", sockaddr);
        free(sockaddr);
    }

    freeaddrinfo(sockinfo);

    return sock;
}


/*
 * Signal handler for SIGINT and SIGTERM, cancels server loop.
 **/
void sighandler(int signal) {
    switch (signal) {
        case SIGINT:
        case SIGTERM:
            _doexit = true;
        default:
            ;
    }
}


/*
 * Register the signal handler above with the kernel.
 **/
void setup_signal_handler() {
    struct sigaction sa = {0};
    sa.sa_handler = &sighandler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}


/*
 * Convenience function, call twice to daemonize
 **/
void fork_and_exit() {
    int pid = fork();

    if (pid < 0) {
        exit(-1);
    }
    else if (pid > 0) {
        exit(0);
    }
}


int main(int argc, char* argv[]) {
    /* init syslog */
    openlog(syslog_ident, LOG_PERROR|LOG_PID, LOG_USER);

    /* setup signal handler */
    setup_signal_handler();

    /* parse commandline options */
    bool daemonize = false;

    if (argc > 1 && !strncmp(argv[1], "-d", 2)) {
        daemonize = true;
    }

    /* bind to socket, do this before daemonizing */
    int sock = bind_to_port(default_port);

    if (sock < 0) {
        exit(-1);
    }

    /* fork to daemon if requested on commandline */
    if (daemonize) {
        closelog();

        fork_and_exit();
        setsid();
        fork_and_exit();

        /* we have to change syslog flags for this */
        openlog(syslog_ident, LOG_PID, LOG_DAEMON);
    }

    /* now start listening for connections */
    if (listen(sock, 5) != 0) {
        syslog(LOG_PERROR, "Error listening on port %s!", default_port);
        exit(-1);
    }

    syslog(LOG_INFO, "Listening on %s\n", default_port);

    struct sockaddr clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);

    struct threadlist_node_t *children = NULL;

    pthread_mutex_t tmpfile_lock;
    pthread_mutex_init(&tmpfile_lock, NULL);

    timer_t timestamp_timer_id;
    if (create_timestamp_timer(&timestamp_timer_id, &tmpfile_lock) != 0) {
        syslog(LOG_PERROR, "Failed to arm timer");
    }

    /* server loop, exited by signals */
    while (!_doexit) {
        /* wait for connections */
        int newsock = accept(sock, &clientaddr, &clientaddrlen);

        if (newsock < 0) {
            syslog(LOG_PERROR, "Error accepting connection");
            continue;
        }

        char *clientip = get_addr_str(&clientaddr);

        syslog(LOG_INFO, "Accepted connection from %s", clientip);

        struct threadlist_node_t *newborn = threadlist_node_create();

        struct connection_handler_args_t *connection_handler_args = connection_handler_create_args(newsock, clientip, &tmpfile_lock);

        /* spawn thread to handle connection */
        if (pthread_create(&newborn->thread_id, NULL, connection_handler, connection_handler_args) != 0) {
            syslog(LOG_PERROR, "Error creating thread");
            connection_handler_destroy_args(&connection_handler_args);
            free(newborn);
            continue;
        }

        threadlist_attach(&children, newborn);
    }

    syslog(LOG_INFO, "Caught signal, exiting");

    /* Wait for remaining threads */
    threadlist_cleanup(&children);

    close(sock);
    timer_delete(timestamp_timer_id);
    pthread_mutex_destroy(&tmpfile_lock);
    unlink(tmpfilename); /* remove tempfile, note: posix has special tempfiles for this... */

    exit(0);
}
