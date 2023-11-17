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


static const char *syslog_ident = "aesdsocket";
const char *default_port = "9000";
const char *tmpfilename = "/var/tmp/aesdsocketdata";


bool doexit = false;


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


void get_client_ip(struct sockaddr *clientaddr, char **clientip) {
    switch(clientaddr->sa_family) {
        case AF_INET:
            struct in_addr addr4 = ((struct sockaddr_in*)clientaddr)->sin_addr;
            *clientip = calloc(sizeof(char), INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &addr4, *clientip, INET_ADDRSTRLEN);
            break;
        case AF_INET6:
            struct in6_addr addr6 = ((struct sockaddr_in6*)clientaddr)->sin6_addr;
            *clientip = calloc(sizeof(char), INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &addr6, *clientip, INET6_ADDRSTRLEN);
            break;
        default:
            *clientip = calloc(sizeof(char), 1);
            *clientip[0] = '\0';
    }
}


/* 
 * Read lines from file fdin and write each line to fdout until EOF of fdin is reached.
 * Returns the number of lines transfered.
 */
int transfer_lines(int fdin, int fdout, int maxlines) {
    int nlines = 0;
    FILE *fin = fdopen(fdin, "r");
    char *line = NULL;
    size_t len = 0;

    while (nlines < maxlines && getline(&line, &len, fin) != -1) {
        syslog(LOG_DEBUG, "Received: %s", line);
        write(fdout, line, len);
        nlines++;
    }

    free(line);
    return nlines;
}


void sighandler(int signal) {
    switch (signal) {
        case SIGINT:
        case SIGTERM:
            doexit = true;
        default:
    }
}


void setup_signal_handler() {
    struct sigaction sa = {0};
    sa.sa_handler = &sighandler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}


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

    // parse daemonize option
    bool daemonize = false;
    if (argc > 1
            && strlen(argv[1]) > 1
            && !strncmp("-d", argv[1], 2)) {
        daemonize = true;
    }

    int sock = bind_to_port(default_port);

    if (sock < 0) {
        exit(-1);
    }

    if (daemonize) {
        closelog();

        fork_and_exit();
        setsid();
        fork_and_exit();
        
        openlog(syslog_ident, LOG_PID, LOG_DAEMON);
    }

    if (listen(sock, 5) != 0) {
        syslog(LOG_DEBUG, "Error listening on port %s!\n", default_port);
        exit(-1);
    }
    
    fprintf(stdout, "Listening on %s\n", default_port);

    int newsock, tmpfile, packetcnt;
    struct sockaddr clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);


    // receiver loop
    while (!doexit) {
        newsock = accept(sock, &clientaddr, &clientaddrlen);

        if (newsock < 0) {
            syslog(LOG_DEBUG, "Error accepting connection");
            continue;
        }

        char *clientip;
        get_client_ip(&clientaddr, &clientip);
        
        syslog(LOG_INFO, "Accepted connection from %s", clientip);

        /* receive packets */
        tmpfile = open(tmpfilename, O_CREAT | O_APPEND | O_WRONLY);
        packetcnt = transfer_lines(newsock, tmpfile, 1);
        fsync(tmpfile);
        close(tmpfile);
        syslog(LOG_DEBUG, "Received %d packets from %s", packetcnt, clientip);

        tmpfile = open(tmpfilename, O_RDONLY);
        /* send file contents */
        packetcnt = transfer_lines(tmpfile, newsock, 0);
        close(tmpfile);
        syslog(LOG_DEBUG, "Sent %d packets to %s", packetcnt, clientip);

        syslog(LOG_INFO, "Closed connection from %s", clientip);
        free(clientip);
    }
    
    syslog(LOG_INFO, "Caught signal, exiting");

    unlink(tmpfilename);

    exit(0);
}
