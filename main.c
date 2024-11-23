/**
 * Complete, there should be no need to edit this file
 *
 * Usage (after running make): ./main
 *
 * This file takes input from stdin and outputs to stdout. No CLI args
 */

#include "shell.h"

int _main(int argc, const char* argv[]) {
    char input[MAX_LINE_SIZE + 1];

    while (true) {
        printf("%s", SHELL_PROMPT);
        fgets(input, MAX_LINE_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0';  // Remove newline from user input
        command* cmd = parse(input);

        if (cmd->argc > 0) {
            if (execute(cmd) == ERROR)
                fprintf(stderr, "%s command failed\n", input);
        }

        cleanup(cmd);
    }

    return EXIT_SUCCESS;
}

/**
 * tests.cpp needs to test the main function
 * However, GTest tests executable already defines a main function so can't
 * include main function
 * In Makefile, for tests executable, this file compiled with flag -DTEST_MODE
 * to exclude main function
 */
#ifndef TEST_MODE

int main(int argc, const char** argv) { return _main(argc, argv); }

#endif  // TEST_MODE
