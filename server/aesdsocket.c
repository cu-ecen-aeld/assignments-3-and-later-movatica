#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif 

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>


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
 * Create a socket that binds to the specified port.
 */
int bind_to_port(const char* port) {
    int result, sock;
    struct addrinfo hints = {0}, *sockinfo, *si;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, port, &hints, &sockinfo);
    if (result != 0) {
        syslog(LOG_DEBUG, "getaddrinfo: %s\n", gai_strerror(result));
        return -1;
    }

    for (si = sockinfo; si != NULL; si = si->ai_next) {
        sock = socket(si->ai_family, si->ai_socktype, si->ai_protocol);

        if (sock == -1)
            continue;
    
        const char reuse = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(char));

        if (bind(sock, si->ai_addr, si->ai_addrlen) == 0)
            break;

        close(sock);
    }

    freeaddrinfo(sockinfo);

    if (si == NULL) {
        syslog(LOG_DEBUG, "Error binding to port %s!\n", port);
        return -1;
    }

    return sock;
}


/*
 * Get the ip address from a sockaddr struct as string.
 * Allocate output parameter `str` accordingly.
 **/
void get_addr_str(struct sockaddr *sa, char **str) {
    switch(sa->sa_family) {
        case AF_INET: { /* ipv4 */
            struct in_addr ina = ((struct sockaddr_in*)sa)->sin_addr;
            *str = calloc(sizeof(char), INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &ina, *str, INET_ADDRSTRLEN);
            break; }
        case AF_INET6: {/* ipv6 */
            struct in6_addr in6a = ((struct sockaddr_in6*)sa)->sin6_addr;
            *str = calloc(sizeof(char), INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &in6a, *str, INET6_ADDRSTRLEN);
            break; }
        default: /* should not happen, alloc an empty string just in case */
            *str = calloc(sizeof(char), 1);
            *str[0] = '\0';
    }
}


/* 
 * Copy line from instream to outstream.
 * Return number of bytes copied or -1 on error.
 **/
ssize_t transfer_line(FILE *instream, FILE *outstream) {
    char *buffer = NULL;
    size_t buflen = 0;
    ssize_t result;

    if ((result = getline(&buffer, &buflen, instream)) > 0) {
        fputs(buffer, outstream);
        fflush(outstream);
    }
    free(buffer);

    return result;
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
    if (argc > 1
            && strlen(argv[1]) > 1
            && !strncmp("-d", argv[1], 2)) {
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
        syslog(LOG_DEBUG, "Error listening on port %s!\n", default_port);
        exit(-1);
    }
    
    fprintf(stdout, "Listening on %s\n", default_port);

    struct sockaddr clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);

    /* server loop, exited by signals */
    while (!_doexit) {
        /* wait for connections */
        int newsock = accept(sock, &clientaddr, &clientaddrlen);

        if (newsock < 0) {
            syslog(LOG_DEBUG, "Error accepting connection");
            continue;
        }

        /* its actually some hazzle to generically get the client ip address */
        char *clientip;
        get_addr_str(&clientaddr, &clientip);
        
        syslog(LOG_INFO, "Accepted connection from %s", clientip);

        FILE *fsock = fdopen(newsock, "a+");
        FILE *tmpfile = fopen(tmpfilename, "a");
        
        ssize_t transres = transfer_line(fsock, tmpfile);
        if (transres == -1) {
            syslog(LOG_PERROR, "Error receiving from %s", clientip);
        }
        else {
            syslog(LOG_DEBUG, "Received %ld bytes from %s", transres, clientip);
        }

        tmpfile = freopen(tmpfilename, "r", tmpfile);

        while ((transres = transfer_line(tmpfile, fsock)) > 0) {
            syslog(LOG_DEBUG, "Sent %ld bytes to %s", transres, clientip);
        }
        if (transres == -1) {
            syslog(LOG_PERROR, "Error sending to %s", clientip);
        }

        fclose(tmpfile);
        fclose(fsock);

        syslog(LOG_INFO, "Closed connection from %s", clientip);

        free(clientip);
    }
    
    syslog(LOG_INFO, "Caught signal, exiting");

    close(sock);
    unlink(tmpfilename); /* remove tempfile, note: posix has special tempfiles for this... */

    exit(0);
}
