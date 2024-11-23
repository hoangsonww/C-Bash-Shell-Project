#ifndef SHELL_H
#define SHELL_H

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHELL_PROMPT "thsh$ "

#define SUCCESS 0
#define ERROR -1

#define MAX_LINE_SIZE 1000
#define MAX_ARG_LEN 100
#define MAX_ENV_VAR_LEN getpagesize() * 32

// Disallow exec*p* variants, lest we spoil the fun
#pragma GCC poison execlp execvp execvpe

/**
 * Represents a shell command
 *
 * For example, "ls -la" would be represented as argc: 2 and argv: {"ls", "-la",
 * NULL}. Note that argv is NULL-terminated.
 */
typedef struct {
    int argc;
    char** argv;
} command;

/**
 * Creates a command with argc set to the value of the parameter argc.
 *
 * Mallocs memory for rv->argv, which should have length argc + 1.
 * rv->argv[0] ... rv->argv[argc - 1] are the arguments.
 *
 * rv->argv[argc] should be NULL.
 *
 * Also, for each char* in cmd->argv, malloc MAX_ARG_LEN chars.
 *
 * @note See shell.h, command struct docstring
 * @note To review how to allocate memory for a char**, see the final slide of
 * [Memory
 * Allocation](https://uncch.instructure.com/users/9947/files/6650292?verifier=Ck555bz4rVkIUYSwscINfbZEJulLQL2N0gV09VUc&wrap=1)
 * @param argc
 * @return command*
 */
command* create_command(int argc);

/**
 * Parses line (user-inputted command) into command* and returns it.
 *
 * Determine argc, the number of arguments, by parsing line.
 *
 * Then, call create_command(argc) and **copy** the arguments from line into
 * cmd->argv.
 *
 * For examples, see tests.cpp
 *
 * To parse line, we recommend strtok. If used correctly, it'll handle all
 * test inputs correctly.
 * https://systems-encyclopedia.cs.illinois.edu/articles/c-strtok/
 *
 * strtok's functionality is similar to that of the string split method in
 * Python and Java
 *
 * strtok mutates the input char* (by inserting null terminators). So, to
 * be safe, you should use strtok on a clone of the input char*. You can create
 * a clone using strdup
 *
 * Alternatively, you may reimplement tokenizing logic yourself, but be
 * sure to handle extra whitespaces correctly.
 *
 * @note When copying the arguments (strings) to char** argv, DO NOT use just
 * the assignment operator =. You must actually copy the strings with
 * str(n)cpy (or reimplement its logic).
 * @param line string input from user
 * @return command*
 */
command* parse(char* line);

/**
 * cmd->argv[0] is a program, e.g., "ls", "rm", "vim", etc.
 * This function determines whether the program can be found
 * in one of the directories in $PATH. If so, mutates cmd->argv[0] to be the
 * full path to the program and returns true. Else, returns false. In short,
 * this behaves similarly to the `which` command.
 *
 * To see the format of $PATH, run `echo $PATH` in your terminal, which should
 * output something like
 * /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/mnt/learncli/bin
 *
 * For example, suppose prog is "ls". This function attempts to find its
 * absolute path in the file system. For the $PATH above,
 * it appends "/ls" to the first directory, resulting in
 * /usr/local/sbin/ls. However, this file does not exist, and same for the next
 * two directories. However, /usr/bin/ls exists and is a regular file, so the
 * function mutates cmd->argv[0] to be "/usr/bin/ls" and returns true.
 *
 * If the executable is "doesnotexist", then this function would not find that
 * file after checking all directories in $PATH, so it would return false.
 *
 * To access $PATH within C, use getenv. For example, getenv("PATH");
 *
 * If you need to specify a maximum length (e.g., when declaring a char[] to
 * store the $PATH string or as an argument to strncpy), use MAX_ENV_VAR_LEN.
 *
 * To parse PATH, we recommend strtok, as described in the docstring for the
 * parse function.
 *
 * To check whether a file exists and is a regular file, use stat and S_ISREG.
 * See https://man7.org/linux/man-pages/man2/stat.2.html and
 * https://stackoverflow.com/a/40163393/18479243.
 *
 * @param cmd
 * @return true (program in PATH) | false (program not in PATH)
 */
bool find_full_path(command* cmd);

/**
 * Executes a command.
 *
 * When this function is called, cmd has already been parsed. See main.c
 * Specifically, cmd is the result of calling parse on the user input
 *
 * cmd may be a built-in command (cd or exit) or non-built-in
 *
 * Use is_builtin and do_builtin to detect and execute built-in commands
 *
 * For non-built-in commands, use find_full_path.
 * Also, use fork, execv, and wait. For details, see Process API content in
 * README section Background reading
 *
 * You cannot use execlp, execvp, execvpe, or any other p variant that searches
 * PATH for you. Doing so would cause a compile error due to a line in shell.h
 *
 * If a command cannot be found, print "Command {command} not found!\n" to
 * stdout. For example, if the command is "thisisnotacommand", then this
 * function should print "Command thisisnotacommand not found!\n". In this case,
 * this function should also return ERROR.
 *
 * @param cmd
 * @return SUCCESS | ERROR
 */
int execute(command* cmd);

/**
 * Frees memory used by cmd. Free each char* in cmd->argv,
 * free cmd->argv, and free cmd (in this order).
 *
 * @note Not unit tested in tests.cpp but necessary to pass Valgrind test case
 * @param cmd
 * @return void
 */
void cleanup(command* cmd);

/**
 * Determines whether cmd is a valid built-in command (in this lab, cd or exit)
 *
 * @param cmd
 * @return true | false
 */
bool is_builtin(command* cmd);

/**
 * Executes built-in commands (in this lab, cd or exit)
 *
 * @param cmd
 * @return SUCCESS | ERROR
 */
int do_builtin(command* cmd);

#endif  // SHELL_H
