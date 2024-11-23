#include "shell.h"

/**
 * shell.c - A simple shell implementation
 *
 * This file contains the core functionality for a basic shell program,
 * including command parsing, memory management, and execution logic.
 * It handles user input, initializes commands, allocates necessary resources,
 * and executes processes in a controlled environment.
 */

// Initializes a command struct and allocates memory for its argv array
command* create_command(int argc) {
    // Allocate memory for the command struct
    command* rv = new command;

    // Set argc for command
    rv->argc = argc;

    // Allocate memory for argv (argc + 1 for NULL terminator)
    rv->argv = new char*[argc + 1];

    // Allocate memory for each argument string
    for (int i = 0; i < argc; i++) {
        // Using new char[] because the C++ test is using char
        // To avoid Valgrind mismatch errors
        rv->argv[i] = new char[MAX_ARG_LEN];
    }

    // Null terminate argv array
    rv->argv[argc] = NULL;

    // Return pointer to the newly created command struct
    return rv;
}

// Parses the input string into arguments and initializes a command structure
command* parse(char* line) {
    // Check validity of argument first
    if (line == NULL) {
        return NULL;
    }

    // Clone the input line to avoid modifying the original line
    // // Using new instead of malloc to avoid valgrind issues
    char* line_copy = new char[strlen(line) + 1];
    strcpy(line_copy, line);

    // Count the number of arguments
    int argc = 0;

    // Split the line into spaces, tabs, and newlines to count the args
    char* currentToken = strtok(line_copy, " \t\n");

    // Count args and resumes tokenization, loop until end of line_copy
    while (currentToken != NULL) {
        argc++;
        currentToken = strtok(NULL, " \t\n");
    }

    // Create a command structure
    command* cmd = create_command(argc);

    // If command creation fails
    if (cmd == NULL) {
        // Prevent memory leak by deallocating
        // delete[] being used to avoid mismatch issues of Valgrind
        delete[] line_copy;
        return NULL;
    }

    // Reset line_copy
    strcpy(line_copy, line);

    // Get the current starting token
    currentToken = strtok(line_copy, " \t\n");

    // Populate cmd->argv with tokens from the input line
    for (int i = 0; i < argc; i++) {
        // Check if token is not NULL before tokenizing and populating commands
        if (currentToken != NULL) {
            // Copy token to argv[i]
            strncpy(cmd->argv[i], currentToken, MAX_ARG_LEN - 1);
            // Null terminates each string in argv
            cmd->argv[i][MAX_ARG_LEN - 1] = '\0';
            // Continue tokenization
            currentToken = strtok(NULL, " \t\n");
        }
    }

    // Clean up
    // Using delete[] to prevent incompatibility issue with Valgrind
    delete[] line_copy;

    // Return the newly created command
    return cmd;
}

// Looks for the full path of a program in the directories listed in $PATH env
// variable
bool find_full_path(command* cmd) {
    // Check for validity of arguments first
    if (cmd == NULL || cmd->argv[0] == NULL) {
        return false;
    }

    // Get the $PATH var
    char* path_env = getenv("PATH");

    // Check if getenv successfully returns the env variable's path
    if (path_env == NULL) {
        perror("getenv failed");
        return false;
    }

    // Clone the $PATH to avoid modifying the original path
    // Using new instead of malloc to avoid compatibility issues with Valgrind
    char* path_copy = new char[strlen(path_env) + 1];
    strcpy(path_copy, path_env);

    // Tokenize the $PATH variable by colon(s) - each token is a directory
    char* currentToken = strtok(path_copy, ":");

    // Loop to construct the full path - iterates through all directories in
    // $PATH
    while (currentToken != NULL) {
        // Construct full path by appending argv[0] to current directory
        char full_path[MAX_LINE_SIZE];
        // Construct the full path by combining the current token (likely a
        // directory) and the first argument of the command
        snprintf(full_path, sizeof(full_path), "%s/%s", currentToken,
                 cmd->argv[0]);

        // Check if the file exists and is a regular file, using built-in
        // functions
        struct stat buffer;
        if (stat(full_path, &buffer) == 0 && S_ISREG(buffer.st_mode)) {
            // First delete old argv[0] to replace it with new data
            // Using delete[] to prevent incompatibility issue with Valgrind
            delete[] cmd->argv[0];

            // Modify cmd->argv[0] to be the full path
            // Using new instead of malloc to avoid valgrind issues
            cmd->argv[0] = new char[strlen(full_path) + 1];
            strcpy(cmd->argv[0], full_path);

            // Deallocate memory to prevent mem leaks
            // Using delete[] to prevent incompatibility issue with Valgrind
            delete[] path_copy;

            // Indicate that the find full path logic succeeded
            return true;
        }

        // Continue tokenizing
        currentToken = strtok(NULL, ":");
    }

    // Cleanup to prevent mem leaks
    // Using delete[] to prevent incompatibility issue with Valgrind
    delete[] path_copy;

    // If the loop finishes and no valid executable is found in any directory
    // in $PATH, return false.
    return false;
}

// Executes the command by first checking if it is a built-in and then executing
// external commands
int execute(command* cmd) {
    // Check for validity of arguments first
    if (cmd == NULL || cmd->argv[0] == NULL) {
        return ERROR;
    }

    // For built-in commands:
    // Check if the command is a built-in command
    if (is_builtin(cmd)) {
        return do_builtin(cmd);
    }

    // For external commands:
    // Check if that command exist
    if (!find_full_path(cmd)) {
        printf("Command %s not found!\n", cmd->argv[0]);
        return ERROR;
    }

    // Create a new process by duplicating the current process
    pid_t pid = fork();  // Using the Process API

    // If fork() returns a negative value, it means the process creation failed
    if (pid < 0) {
        perror("fork failed");
        return ERROR;
    }

    // In the child process:
    if (pid == 0) {
        // replace the current process image with a new program specified by
        // argv[0] and pass the args list argv to the new program
        execv(cmd->argv[0], cmd->argv);

        // If execv() fails, it returns and does not replace the process
        perror("execv failed");
        exit(EXIT_FAILURE);
    } else {
        // In the parent process: (pid > 0 returned to the parent)
        int status;

        // Wait for the child process to terminate and get its status
        if (waitpid(pid, &status, 0) == -1) {
            // If waitpid() fails
            perror("waitpid failed");
            return ERROR;
        }

        // Check the child's terminations status:
        // WIFEXITED(status): true if the child terminated normally
        // WEXITSTATUS(status): retrieves the exit status of the child
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return SUCCESS;
        } else {
            return ERROR;
        }
        // If the child exited normally and its exit status is 0, return
        // SUCCESS. Otherwise, return ERROR
    }
}

// Frees all memory associated with the command struct
void cleanup(command* cmd) {
    // Check for validity of argument first
    if (cmd == NULL) {
        return;
    }

    // Free each argument string individually
    for (int i = 0; i < cmd->argc; i++) {
        delete[] cmd->argv[i];
    }

    // Free the argv array entirely
    delete[] cmd->argv;

    // Free the command structure itself to prevent mem leaks
    delete cmd;
}

// Determines if the command is a built-in (cd or exit)
bool is_builtin(command* cmd) {
    // Do not modify
    char* executable = cmd->argv[0];

    if (strcmp(executable, "cd") == 0 || strcmp(executable, "exit") == 0)
        return true;

    return false;
}

// Executes built-in commands (cd or exit)
int do_builtin(command* cmd) {
    // Do not modify
    if (strcmp(cmd->argv[0], "exit") == 0) exit(SUCCESS);

    // cd
    if (cmd->argc == 1)
        return chdir(getenv("HOME"));  // cd with no arguments
    else if (cmd->argc == 2)
        return chdir(cmd->argv[1]);  // cd with 1 arg
    else {
        fprintf(stderr, "cd: Too many arguments\n");
        return ERROR;
    }
}
