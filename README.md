# Shell Implementation in C

This project is a simple shell implementation written in C, designed to provide basic shell functionality such as parsing commands, searching for executables in the system's `PATH`, handling built-in commands (`cd`, `exit`), and executing external programs. The shell provides a controlled environment for process management and command execution.

## Table of Contents

1. [Overview](#overview)
2. [Features](#features)
3. [File Structure](#file-structure)
4. [Compilation and Execution](#compilation-and-execution)
5. [Usage](#usage)
6. [Code Highlights](#code-highlights)
7. [Future Improvements](#future-improvements)
8. [Testing](#testing)
9. [Acknowledgments](#acknowledgments)
10. [Author](#author)

## Overview

The **Shell Implementation in C** provides fundamental features for executing user commands. It includes:
- Parsing user input into arguments.
- Identifying commands (both built-in and external).
- Handling the `PATH` environment variable to locate executables.
- Executing commands in a child process while the parent process waits.
- Memory management and error handling to avoid leaks and crashes.
- Support for built-in commands like `cd` and `exit`.
- A simple interactive shell interface for user input and output.
- A Makefile for easy compilation and testing.
- Unit tests to verify the shell's functionality.
- Compatibility with GNU/Linux systems.


## Features

1. **Command Parsing**: The shell parses user input into tokens (arguments) and constructs a `command` structure.
2. **Built-in Commands**:
   - `cd`: Changes the current working directory.
   - `exit`: Exits the shell.
3. **External Command Execution**: Supports executing external programs by searching for their full paths using the `PATH` environment variable.
4. **Memory Management**: Implements dynamic memory allocation using `new` and `delete`, ensuring no memory leaks or dangling pointers.
5. **Error Handling**: Comprehensive error handling for memory allocation, invalid commands, and execution errors.

## File Structure

- **`shell.h`**: Header file containing definitions, constants, and function prototypes.
- **`shell.c`**: Main implementation of the shell logic.
- **`Makefile`**: File for building the project using `make`.
- **`tests.cpp`**: Unit tests to verify the shell's functionality (optional, if included).

## Compilation and Execution

### Requirements
- **Operating System**: Must be GNU/Linux system (might be incompatible with Windows or macOS).
- **C Compiler**: GCC or Clang (with C++ compatibility if using `new`/`delete`).
- **Make**: Build system utility.
- **Valgrind** (optional): For memory leak detection.

### Steps to Compile and Run
1. Clone or download the project to your local machine.
2. Navigate to the project directory.
3. Compile the code:
   ```bash
   make
   ```
4. Run the shell:
   ```bash
   ./main
   ```

### Running with Valgrind
To ensure there are no memory leaks:
```bash
make valgrind
```

## Usage

1. Start the shell:
   ```bash
   ./main
   ```
2. Enter a command:
   - Example of a built-in command:
     ```bash
     cd /home/user
     ```
        or
        ```bash
        pwd
        ```
   - Example of an external command:
     ```bash
     ls -l
     ```
3. Exit the shell:
   ```bash
   exit
   ```

## Code Highlights

### 1. Command Parsing
The `parse` function takes a user input string, tokenizes it into arguments, and constructs a `command` structure:
```c
command* parse(char* line) {
    // Clone input to avoid modifying original
    char* line_copy = new char[strlen(line) + 1];
    strcpy(line_copy, line);

    // Tokenize input and count arguments
    int argc = 0;
    char* currentToken = strtok(line_copy, " \t\n");
    while (currentToken != NULL) {
        argc++;
        currentToken = strtok(NULL, " \t\n");
    }

    // Create command structure
    command* cmd = create_command(argc);
    if (!cmd) return NULL;

    // Populate arguments
    strcpy(line_copy, line); // Reset line_copy
    currentToken = strtok(line_copy, " \t\n");
    for (int i = 0; i < argc && currentToken != NULL; i++) {
        strncpy(cmd->argv[i], currentToken, MAX_ARG_LEN - 1);
        cmd->argv[i][MAX_ARG_LEN - 1] = '\0';
        currentToken = strtok(NULL, " \t\n");
    }

    delete[] line_copy;
    return cmd;
}
```

### 2. Built-in Commands
Support for built-in commands `cd` and `exit` is implemented in the `do_builtin` function:
```c
int do_builtin(command* cmd) {
    if (strcmp(cmd->argv[0], "exit") == 0) {
        exit(SUCCESS);
    }
    if (cmd->argc == 1) {
        return chdir(getenv("HOME")); // cd with no arguments
    } else if (cmd->argc == 2) {
        return chdir(cmd->argv[1]); // cd with one argument
    } else {
        fprintf(stderr, "cd: Too many arguments\n");
        return ERROR;
    }
}
```

### 3. External Command Execution
The shell uses `execv` to execute external commands, following a search in the `PATH` environment variable:
```c
bool find_full_path(command* cmd) {
    char* path_env = getenv("PATH");
    if (!path_env) return false;

    char* path_copy = new char[strlen(path_env) + 1];
    strcpy(path_copy, path_env);

    char* currentToken = strtok(path_copy, ":");
    while (currentToken != NULL) {
        char full_path[MAX_LINE_SIZE];
        snprintf(full_path, sizeof(full_path), "%s/%s", currentToken, cmd->argv[0]);

        struct stat buffer;
        if (stat(full_path, &buffer) == 0 && S_ISREG(buffer.st_mode)) {
            delete[] cmd->argv[0];
            cmd->argv[0] = new char[strlen(full_path) + 1];
            strcpy(cmd->argv[0], full_path);

            delete[] path_copy;
            return true;
        }
        currentToken = strtok(NULL, ":");
    }

    delete[] path_copy;
    return false;
}
```

### 4. Memory Management
Memory is managed using `new` and `delete` throughout the project, ensuring no leaks.

## Future Improvements

1. **Enhanced Built-in Commands**: Add more built-ins like `history` or `export`.
2. **Background Processing**: Add support for background tasks.
3. **Advanced Parsing**: Handle pipes (`|`), redirections (`>`, `<`), and quotes.
4. **Interactive Features**: Improve user experience with command auto-completion and history navigation.
5. **Error Reporting**: Provide more descriptive error messages for better debugging.

## Testing

Unit tests are provided in the `tests.cpp` file to verify the shell's functionality. The tests cover parsing, built-in commands, external command execution, and memory management. To run the tests, compile the project with the `tests` target:
```bash
make tests
```

Tests are provided using the Google Test framework. You might need to install the Google Test library to run the tests.

## Acknowledgments

This shell was built as part of a project to understand and implement fundamental system programming concepts in C. It incorporates concepts like dynamic memory allocation, process creation, and system calls, inspired by Unix-based shell functionality.

## Author

**Son Nguyen**  
GitHub: [hoangsonww](https://github.com/hoangsonww)

---

Created with ❤️ by [Son Nguyen](https://github.com/hoangsonww) in 2024.
