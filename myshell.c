/* CS 347 -- Mini Shell!
 * Original author Phil Nelson 2000
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include "argparse.h"
#include "builtin.h"

/* PROTOTYPES */
void processline(char *line);
ssize_t getinput(char** line, size_t* size);

/* main
 * This function is the main entry point to the program.  This is essentially
 * the primary read-eval-print loop of the command interpreter.
 */
int main() {
    char* line = NULL;
    size_t size = 0;

    while (1) {
        printf("%s ", "%myshell%"); // Prompt
        ssize_t len = getinput(&line, &size);
        if (len < 0) {
            fprintf(stderr, "Error reading input\n");
            continue;
        } else if (len == 0) {
            // Empty line, continue to the next iteration
            continue;
        }

        // Process the line
        processline(line);

        // Free the allocated memory for line
        free(line);
        line = NULL;
    }

    return EXIT_SUCCESS;
}

/* getinput
 * line     A pointer to a char* that points at a buffer of size *size or NULL.
 * size     The size of the buffer *line or 0 if *line is NULL.
 * returns  The length of the string stored in *line.
 *
 * This function prompts the user for input, e.g., %myshell%.  If the input fits in the buffer
 * pointed to by *line, the input is placed in *line.  However, if there is not
 * enough room in *line, *line is freed and a new buffer of adequate space is
 * allocated.  The number of bytes allocated is stored in *size.
 * Hint: There is a standard i/o function that can make getinput easier than it sounds.
 */
ssize_t getinput(char** line, size_t* size) {
    ssize_t len = getline(line, size, stdin);
    if (len == -1) {
        // Error or end-of-file
        if (feof(stdin)) {
            exit(EXIT_SUCCESS); // End-of-file
        } else {
            perror("getline");
            return -1; // Error
        }
    }
    // Remove newline character if present
    if ((*line)[len - 1] == '\n') {
        (*line)[len - 1] = '\0';
        len--;
    }
    return len;
}

/* processline
 * The parameter line is interpreted as a command name.  This function creates a
 * new process that executes that command.
 * Note the three cases of the switch: fork failed, fork succeeded and this is
 * the child, fork succeeded and this is the parent (see fork(2)).
 * processline only forks when the line is not empty, and the line is not trying to run a built in command
 */
void processline(char *line) {
    // Check whether line is empty
    if (line[0] == '\0') {
        return;
    }

    pid_t cpid;
    int status;
    int argCount;
    char** arguments = argparse(line, &argCount);

    // Check whether arguments are built-in commands
    if (!builtIn(arguments, argCount)) {
        // Fork to execute the command
        cpid = fork();
        if (cpid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (cpid == 0) {
            // Child process
            execvp(arguments[0], arguments);
            // If execvp returns, it must have failed
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            waitpid(cpid, &status, 0);
        }
    }

    // Free memory allocated for each argument
    for (int i = 0; i < argCount; i++) {
        free(arguments[i]);
    }
    // Free memory allocated for the array of arguments
    free(arguments);
}
