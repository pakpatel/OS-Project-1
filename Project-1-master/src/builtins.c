#include "builtins.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

extern char **environ;

void builtin_exit() {
    exit(0); // just close the program
}

void builtin_env() {
    if (environ == NULL) return; // if there is no environment, break (or else a segfault).
    int i = 1;
    char *s = *environ; // environ defined by linux

    for (; s; i++) { // loop and print all strings returned by environ
        printf("%s\n", s);
        s = *(environ+i);
    }
}

void builtin_export(char *command) {
    char *args = command + 7; // skip the word 'export'


    char *copy = calloc(strlen(args) + 1, sizeof(char)); // must be on heap
    strcpy(copy, args);
    putenv(copy);
}

void builtin_unset(char *command) {
    char *args = command + 6; // skip the word 'unset'

    if (unsetenv(args) != 0) fprintf(stderr, "Could not unset environment variable %s!\n", args);
}

void builtin_chdir(char *command) {
    int dir_loc = 0;
    for (int i = 3; i <= strlen(command); ++i) { // figure out if there are too many args after whitespace
        if (!ispunct(command[i]) && !isalnum(command[i]) && dir_loc == 0) {
            dir_loc = i;
            while (ispunct(command[i]) || isalnum(command[i])) ++i;
            command[i] = '\0';
        }
        if (dir_loc != 0 && (ispunct(command[i]) || isalnum(command[i]))) {
            fprintf(stderr, "cd: too many arguments!\n");
            return;
        }
    }

    char *args = calloc(strlen(command + 3) + 1, sizeof(char));
    strcpy(args, command + 3);

    if (chdir(args) != 0) fprintf(stderr, "cd: could not change directories!\n");

    free(args);
}
