// Author: Jonathan Ly

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h> 
#include <fcntl.h> 
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "builtin.h"
#include <utime.h>

#define MAX_LINES 10

// Declarations
static void exitProgram(char** args, int argcp);
static void cd(char** args, int argcp);
static void pwd(char** args, int argcp);
static void ls(char** args, int argcp);
static void cp(char** args, int argcp);
static void env(char** args, int argcp);
static void stat_file(char** args, int argcp);
static void tail(char** args, int argcp);
static void touch(char** args, int argcp);

/* builtIn
 * builtIn checks each built-in command against the given command.
 * If the given command matches one of the built-in commands,
 * that command is executed, and builtIn returns 1.
 * If none of the built-in commands match the provided command,
 * builtIn returns 0.
 */
int builtIn(char** args, int argcp)
{
    if (strcmp(args[0], "exit") == 0) {
        exitProgram(args, argcp);
        return 1;
    } else if (strcmp(args[0], "pwd") == 0) {
        pwd(args, argcp);
        return 1;
    } else if (strcmp(args[0], "cd") == 0) {
        cd(args, argcp);
        return 1;
    } else if (strcmp(args[0], "ls") == 0) {
        ls(args, argcp);
        return 1;
    } else if (strcmp(args[0], "cp") == 0) {
        cp(args, argcp);
        return 1;
    } else if (strcmp(args[0], "env") == 0) {
        env(args, argcp);
        return 1;
    } else if (strcmp(args[0], "stat") == 0) {
        stat_file(args, argcp);
        return 1;
    } else if (strcmp(args[0], "tail") == 0) {
        tail(args, argcp);
        return 1;
    } else if (strcmp(args[0], "touch") == 0) {
        touch(args, argcp);
        return 1;
    }
    return 0;
}
/**
* Exit the program with specified exit value. 
* If an argument is provided it is parsed as the exit value
* otherwise, the default exit value is 0.
*/ 
static void exitProgram(char** args, int argcp)
{
    // Check if an argument is provided for the exit value
    int exitValue = 0;
    if (argcp > 1) {
        exitValue = atoi(args[1]);
    }
    exit(exitValue);
}

/**
 * Print the current working directory to the standard output.
 * If an error occurs while retrieving the current directory,
 * an error message is printed to the standard error.
 */

static void pwd(char** args, int argcp)
{
    // Get the current working directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("getcwd() error");
    }
}

/**
 * Change the current working directory to the directory specified in the arguments.
 * If no argument is provided, change to the home directory.
 * If an error occurs during the directory change, an error message is printed.
 */

static void cd(char** args, int argcp)
{
    // Change directory
    if (argcp > 1) {
        if (chdir(args[1]) != 0) {
            perror("cd");
        }
    } else {
        // If no directory provided, change to home directory
        char* home = getenv("HOME");
        if (home != NULL) {
            if (chdir(home) != 0) {
                perror("cd");
            }
        } else {
            printf("cd: No HOME environment variable\n");
        }
    }
}


// LS helper function
static void printPermissions(mode_t mode) {
    char perm[10] = "---------";

    // User permissions
    if (mode & S_IRUSR) perm[0] = 'r';
    if (mode & S_IWUSR) perm[1] = 'w';
    if (mode & S_IXUSR) perm[2] = 'x';

    // Group permissions
    if (mode & S_IRGRP) perm[3] = 'r';
    if (mode & S_IWGRP) perm[4] = 'w';
    if (mode & S_IXGRP) perm[5] = 'x';

    // Others permissions
    if (mode & S_IROTH) perm[6] = 'r';
    if (mode & S_IWOTH) perm[7] = 'w';
    if (mode & S_IXOTH) perm[8] = 'x';

    printf("%c%s", S_ISDIR(mode) ? 'd' : '-', perm);
}

// LS helper function
static void printFileInfo(struct dirent* entry, char* path) {
    struct stat statbuf;
    char fullPath[1024];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

    if (stat(fullPath, &statbuf) == -1) {
        perror("stat");
        return;
    }

    // Print file permissions
    printPermissions(statbuf.st_mode);

    // Print number of links
    printf(" %lu", (unsigned long)statbuf.st_nlink);

    // Print owner name
    struct passwd *pwd = getpwuid(statbuf.st_uid);
    printf(" %s", pwd ? pwd->pw_name : "???");

    // Print group name
    struct group *grp = getgrgid(statbuf.st_gid);
    printf(" %s", grp ? grp->gr_name : "???");

    // Print file size
    printf(" %5ld", (long)statbuf.st_size);

    // Print modification time
    char timeBuf[20];
    struct tm *tm = localtime(&statbuf.st_mtime);
    strftime(timeBuf, sizeof(timeBuf), "%b %d %H:%M", tm);
    printf(" %s", timeBuf);

    // Print file name
    printf(" %s\n", entry->d_name);
}

/**
 * List the contents of the current directory.
 * If the '-l' argument is provided, 
 * print detailed information about each entry.
 * If an error occurs during directory opening or reading,
 *  an error message is printed.
 */

static void ls(char** args, int argcp) {
    DIR* dir;
    struct dirent* entry;
    int longFormat = 0;

    // Check for '-l' argument
    for (int i = 1; i < argcp; i++) {
        if (strcmp(args[i], "-l") == 0) {
            longFormat = 1;
            break;
        }
    }

    // Open current directory
    char* path = ".";
    dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    // Read and print entries
    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files
        if (entry->d_name[0] == '.') continue;

        if (longFormat) {
            printFileInfo(entry, path);
        } else {
            printf("%s\n", entry->d_name);
        }
    }

    // Close directory
    closedir(dir);
}
/**
 * Copy a file from a source path to a destination path.
 * 
 */
static void cp(char** args, int argcp) {
    if (argcp != 3) {
        fprintf(stderr, "Usage: cp <source> <destination>\n");
        return;
    }

    const char* srcPath = args[1];
    const char* destPath = args[2];

    // Check if source and destination are the same
    struct stat srcStat, destStat;
    if (stat(srcPath, &srcStat) == 0) {
        if (stat(destPath, &destStat) == 0) {
            if (srcStat.st_ino == destStat.st_ino && srcStat.st_dev == destStat.st_dev) {
                fprintf(stderr, "cp: '%s' and '%s' are the same file\n", srcPath, destPath);
                return;
            }
        }
    } else {
        perror("Error getting source file info");
        return;
    }

    // Open source file
    int srcFD = open(srcPath, O_RDONLY);
    if (srcFD == -1) {
        perror("Error opening source file");
        return;
    }

    // Open destination file
    int destFD = open(destPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (destFD == -1) {
        perror("Error opening destination file");
        close(srcFD); // Close the source file descriptor before returning
        return;
    }

    // Copy process
    char buffer[4096];
    ssize_t readBytes, writeBytes;
    
    while ((readBytes = read(srcFD, buffer, sizeof(buffer))) > 0) {
        writeBytes = write(destFD, buffer, readBytes);
        if (writeBytes != readBytes) {
            perror("Error writing to destination file");
            break;
        }
    }

    if (readBytes == -1) {
        perror("Error reading source file");
    }

    // Close file descriptors
    close(srcFD);
    close(destFD);
}



extern char **environ; // External variable pointing to the environment

/**
 * Display environment variables 
 * or set a new environment variable.
 * If no arguments provided, print all environment variables.
 * If one argument provided in the format NAME=VALUE, attempt 
 * to set the environment variable.
 */

static void env(char** args, int argcp) {
    if (argcp == 1) {
        // Print all environment variables
        for (char **env = environ; *env != 0; env++) {
            char *thisEnv = *env;
            printf("%s\n", thisEnv);
        }
    } else if (argcp == 2) {
        // Attempt to set an environment variable
        char *arg = args[1];
        char *name = arg;
        char *value = strchr(arg, '='); // Find '=' character

        if (value == NULL) {
            fprintf(stderr, "Invalid format. Use NAME=VALUE.\n");
            return;
        }

        // Split the string into name and value
        *value = '\0'; // Null-terminate the name
        value++; // Move to the next character after '='

        if (*value == '\0') {
            fprintf(stderr, "Invalid format. Use NAME=VALUE.\n");
            return;
        }

        if (setenv(name, value, 1) == 0) {
            // Provide feedback that the variable was set
            printf("Environment variable '%s' set to '%s'\n", name, value);
        } else {
            perror("Failed to set environment variable");
        }
    } else {
        fprintf(stderr, "Usage: env or env NAME=VALUE\n");
    }
}

// Stat Helper Function

static void printFileStat(const char *path) {
    struct stat sb;

    if (stat(path, &sb) == -1) {
        perror(path);
        return;
    }

    printf("  File: '%s'\n", path);
    printf("  Size: %lld\tBlocks: %lld\tIO Block: %ld\t", (long long)sb.st_size, (long long)sb.st_blocks, (long)sb.st_blksize);

    // File type
    printf("%s\n", S_ISDIR(sb.st_mode) ? "Directory" : S_ISREG(sb.st_mode) ? "Regular file" : "Other");

    // Permissions
    printf("  Access: (%04o/", sb.st_mode & ~S_IFMT);
    printPermissions(sb.st_mode);
    printf(")\n");

    // UID and GID
    struct passwd *pwd = getpwuid(sb.st_uid);
    struct group *grp = getgrgid(sb.st_gid);
    printf("  UID: (%d/%s)   GID: (%d/%s)\n", sb.st_uid, (pwd != NULL) ? pwd->pw_name : "unknown", sb.st_gid, (grp != NULL) ? grp->gr_name : "unknown");

    // Times
    char timebuf[256];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&sb.st_atime));
    printf("  Access: %s\n", timebuf);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&sb.st_mtime));
    printf("  Modify: %s\n", timebuf);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&sb.st_ctime));
    printf("  Change: %s\n", timebuf);
}
/**
 * Print file or directory statistics 
 * for each specified file or directory.
 */
static void stat_file(char** args, int argcp) {
    if (argcp < 2) {
        fprintf(stderr, "Usage: stat <file/directory>...\n");
        return;
    }

    for (int i = 1; i < argcp; i++) {
        printFileStat(args[i]);
        if (i < argcp - 1) {
            printf("\n"); // Separate the stat info for multiple files/directories
        }
    }
}

// Tail helper function
static void printLastLines(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror(filename);
        return;
    }

    char* lines[MAX_LINES] = {NULL}; // Array to hold the last 10 lines, initialized to NULL
    size_t currentLine = 0;
    size_t totalLines = 0;

    // Read file line by line and store the last 10 lines
    char* buffer = NULL;
    size_t len = 0;
    ssize_t nread;
    while ((nread = getline(&buffer, &len, file)) != -1) {
        if (lines[currentLine] != NULL) {
            free(lines[currentLine]); // Free the old line if it exists
        }
        lines[currentLine] = strdup(buffer); // Store the new line
        currentLine = (currentLine + 1) % MAX_LINES; // Circular buffer logic
        totalLines++;
    }
    fclose(file);
    free(buffer);

    // Determine the starting line for printing
    size_t startLine = (totalLines < MAX_LINES) ? 0 : currentLine;

    // Print the last 10 lines or less if the file is shorter
    for (size_t i = 0; i < MAX_LINES && lines[(startLine + i) % MAX_LINES]; i++) {
        printf("%s", lines[(startLine + i) % MAX_LINES]);
        free(lines[(startLine + i) % MAX_LINES]); // Free after printing
    }
}

/**
 * Print the last few lines of each specified file.
 */
static void tail(char** args, int argcp) {
    if (argcp < 2) {
        fprintf(stderr, "Usage: tail <file1...fileN>\n");
        return;
    }

    for (int i = 1; i < argcp; i++) {
        if (argcp > 2) { // Print header if there are multiple files
            printf("==> %s <==\n", args[i]);
        }
        printLastLines(args[i]);
        if (i < argcp - 1) {
            printf("\n"); // Separate output for multiple files
        }
    }
}

/**
 * Create a new empty file or update the access and
 *  modification times of an existing file.
 */

static void touch(char** args, int argcp) {
    // Check if the correct number of arguments are provided
    if (argcp != 2) {
        fprintf(stderr, "Usage: touch <filename>\n");
        return;
    }

    // Get the file name from the arguments
    char* filename = args[1];

    // Check if the file already exists
    if (access(filename, F_OK) == 0) {
        // File already exists, update its access and modification times
        if (utime(filename, NULL) == -1) {
            perror("utime");
            return;
        }
        printf("Updated access and modification times of '%s'\n", filename);
        return; // Early return
    }

    // File does not exist, create a new empty file
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("fopen");
        return;
    }
    fclose(file);
    printf("Created new file '%s'\n", filename);
}

