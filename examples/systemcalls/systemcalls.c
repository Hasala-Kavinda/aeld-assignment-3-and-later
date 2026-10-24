#include "systemcalls.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    if (cmd == NULL) {
        fprintf(stderr, "do_system: cmd is NULL\n");
        return false;
    }

    int ret = system(cmd);
    if (ret == -1) {
        fprintf(stderr, "do_system: system() failed: %s\n", strerror(errno));
        return false;
    }

    if (WIFEXITED(ret)) {
        if (WEXITSTATUS(ret) == 0) {
            return true;
        }
        fprintf(stderr, "do_system: command exited with status %d\n", WEXITSTATUS(ret));
        return false;
    }

    fprintf(stderr, "do_system: command did not exit normally\n");
    return false;
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

    // Flush stdout before fork so buffered output is not duplicated in the child
    fflush(stdout);

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "do_exec: fork() failed: %s\n", strerror(errno));
        return false;
    }

    if (pid == 0) {
        // Child process: exec the command. execv only returns on failure.
        execv(command[0], command);
        fprintf(stderr, "do_exec: execv() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Parent process: wait for the child to finish
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        fprintf(stderr, "do_exec: waitpid() failed: %s\n", strerror(errno));
        return false;
    }

    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 0) {
            return true;
        }
        fprintf(stderr, "do_exec: child exited with status %d\n", WEXITSTATUS(status));
        return false;
    }

    fprintf(stderr, "do_exec: child did not exit normally\n");
    return false;
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

    // Open the output file BEFORE forking (see SO answer: open in parent)
    int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "do_exec_redirect: open('%s') failed: %s\n",
                outputfile, strerror(errno));
        return false;
    }

    fflush(stdout);

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "do_exec_redirect: fork() failed: %s\n", strerror(errno));
        close(fd);
        return false;
    }

    if (pid == 0) {
        // Child: redirect stdout to the file, then exec
        if (dup2(fd, STDOUT_FILENO) < 0) {
            fprintf(stderr, "do_exec_redirect: dup2() failed: %s\n", strerror(errno));
            close(fd);
            exit(EXIT_FAILURE);
        }
        close(fd);  // STDOUT_FILENO now refers to the same file; original fd unused

        execv(command[0], command);
        fprintf(stderr, "do_exec_redirect: execv() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Parent: close our copy of the fd, then wait for the child
    close(fd);

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        fprintf(stderr, "do_exec_redirect: waitpid() failed: %s\n", strerror(errno));
        return false;
    }

    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 0) {
            return true;
        }
        fprintf(stderr, "do_exec_redirect: child exited with status %d\n", WEXITSTATUS(status));
        return false;
    }

    fprintf(stderr, "do_exec_redirect: child did not exit normally\n");
    return false;
}
