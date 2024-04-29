#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "argparse.h"
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define FALSE (0)
#define TRUE  (1)

/*
* argCount is a helper function that takes in a String and returns the number of "words" in the string assuming that whitespace is the only possible delimiter.
*/
static int argCount(char* line)
{
    int count = 0;
    int inWord = FALSE; // flag to track whether currently in a word

    // Iterate through each character in the string
    while (*line != '\0') {
        if (isspace(*line)) {
            // If the current character is whitespace and we were in a word, increment the count
            if (inWord) {
                count++;
                inWord = FALSE; // reset the flag
            }
        } else {
            // If the current character is not whitespace, set the flag to indicate we are in a word
            if (!inWord) {
                inWord = TRUE;
            }
        }
        line++; // Move to the next character
    }

    // If we were in a word when reaching the end of the string, increment the count
    if (inWord) {
        count++;
    }

    return count;
}

/*
* Argparse takes in a String and returns an array of strings from the input.
* The arguments in the String are broken up by whitespaces in the string.
* The count of how many arguments there are is saved in the argcp pointer
*/
char** argparse(char* line, int* argcp)
{
    // Count the number of arguments
    *argcp = argCount(line);

    // Allocate memory for an array of strings (pointers)
    char** args = (char**)malloc((*argcp + 1) * sizeof(char*)); // Add 1 for NULL terminator
    if (args == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    while (*line != '\0') {
        // Skip leading whitespace
        while (isspace(*line)) {
            line++;
        }
        if (*line == '\0') {
            break; // Exit loop if end of string is reached
        }

        // Store the starting position of the argument
        char* start = line;

        // Find the end of the argument
        while (*line != '\0' && !isspace(*line)) {
            line++;
        }

        // Calculate the length of the argument
        size_t length = line - start;

        // Allocate memory for the argument and copy it into the args array
        args[i] = strndup(start, length);
        if (args[i] == NULL) {
            // Free all previously allocated memory
            for (int j = 0; j < i; j++) {
                free(args[j]);
            }
            free(args);
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        i++;
    }

    // Add NULL terminator to the end of the args array
    args[i] = NULL;

    return args;
}
