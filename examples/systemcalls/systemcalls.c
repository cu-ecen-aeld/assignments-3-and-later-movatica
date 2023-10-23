#include "systemcalls.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>


/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    if (cmd == NULL) {
        return false;
    }

    return system(cmd) == 0;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    va_end(args);

    int childstate = 0;
    pid_t pid = fork();
    
    switch (pid) {
        case -1: // fork failed
            return false;
        case 0: // child process
            execv(command[0], command);
            return false;
        default: // parent process
            waitpid(pid, &childstate, 0);
    }

    return WIFEXITED(childstate) && (WEXITSTATUS(childstate) == 0);
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    va_end(args);

    int outfile = open(outputfile, O_WRONLY|O_CREAT, 644);
    
    if (outfile < 0) {
        return false;
    }

    int childstate = 0;
    pid_t pid = fork();
    
    switch (pid) {
        case -1: // fork failed
            return false;
        case 0: // child process
            if (dup2(outfile, 1) < 0) {
                return false;
            }

            close(outfile);
            execv(command[0], command);
            return false;
        default: // parent process
            close(outfile);
            waitpid(pid, &childstate, 0);
    }

    return false;//WIFEXITED(childstate) && (WEXITSTATUS(childstate) == 0);
}
