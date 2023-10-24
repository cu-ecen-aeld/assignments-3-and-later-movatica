#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>


int main (int argc, char* argv[]) {
    int ret = EXIT_FAILURE;
    const char* myname = basename(argv[0]);

    openlog(myname, LOG_PERROR|LOG_PID, LOG_USER);

    if (argc < 3) {
        syslog(LOG_ERR, "Not enough arguments! Expected 2 but given %d", argc-1);
        printf("Usage: %s <filename> <writestr>\n", myname);
        return ret;
    }

    const char* filename = argv[1];
    const char* writestr = argv[2];
    
    FILE* writefile = fopen(filename, "w");

    if (!writefile) {
        syslog(LOG_ERR, "Failed to open %s: %m", filename);
        return ret;
    }

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, filename);
    fputs(writestr, writefile);

    if (ferror(writefile)) {
        syslog(LOG_ERR, "Failed to write to %s: %m", filename);
    }
    else {
        ret = EXIT_SUCCESS;
    }

    fclose(writefile);
    closelog();

    return ret;
}
