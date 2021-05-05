#include "parser.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <regex.h>

#include "run_program.h"
#include "builtins.h"

int process_input() {
    return 0;
}

// figure out if command matches a known builtin
int check_builtin_s(char *command, char *builtin) {
    char search[512];
    snprintf(search, 511, "^\\s*%s$|^\\s*%s\\s{1,}.*", builtin, builtin); // regex (accepts in the form of "exit" and "exit anything",
                                                                          // the latter for builtins like export. ignores whitespace)

    regex_t regex;
    int ret = regcomp(&regex, search, REG_EXTENDED); // use extended regex
    ret = regexec(&regex, command, 0, NULL, 0); // do the confirmation

    return ret;
}

// match command with possible builtin
int check_builtin(char *command) {
    if (check_builtin_s(command, "exit") == 0) { builtin_exit(); return 1; }
    if (check_builtin_s(command, "quit") == 0) { builtin_exit(); return 1; }
    if (check_builtin_s(command, "env") == 0) { builtin_env(); return 1; }
    if (check_builtin_s(command, "export") == 0) { builtin_export(command); return 1; }
    if (check_builtin_s(command, "unset") == 0) { builtin_unset(command); return 1; }
    if (check_builtin_s(command, "cd") == 0) { builtin_chdir(command); return 1; }

    return 0;
}

int find_comment(char *line) {
    int in_single = 0; // bool for if in single quote
    int in_double = 0; //   .   .   .  . double   .

    for (size_t i = 0; i < strlen(line); ++i) {
        if (line[i] == '#')
            if (!in_double && !in_single) return i;

        if (line[i] == '\'') in_single = !in_single;
        if (line[i] == '\"') in_double = !in_double;
    }

    return -1;
}

int process_line(char *line) {
    if (!line) return -1; // line should exist

    // remove anything that is a comment
    int comment_loc = find_comment(line);
    
    if (comment_loc > 0) {
        line[comment_loc] = '\0';
        line = realloc(line, strlen(line) + 1);
    }

    char *cur_poi = line; // current section(person) of interest (poi)
    char *temp = NULL; // stores individual command

    int first_pgid = 0;
    do {
        if (cur_poi[0] == ';') ++cur_poi; // advance if semicolon is the first char in line
        if (strchr(cur_poi, ';') != NULL) { // if rest of line contains semicolon, copy text between this and last semicolon as command
            size_t length = strchr(cur_poi, ';') - cur_poi;
            temp = calloc(length, sizeof(char));
            strncpy(temp, cur_poi, length);
        } else { // no semicolons found
            size_t length = 0;
            if (cur_poi[strlen(cur_poi) - 1] == '\n') length = strlen(cur_poi) - 1; // if the line is over
            else length = strlen(cur_poi);

            temp = calloc(length + 1, sizeof(char));
            strncpy(temp, cur_poi, length);
        }

        // remove trailing whitespace from beginning of command
        size_t i = 0;
        while (temp[i] == ' ' || temp[i] == '\t') i++;
        if (i > 0) {
            char *s = calloc(strlen(temp + i) + 1, sizeof(char));
            strcpy(s, temp + i);
            memset(temp, 0, strlen(s) + 1);
            temp = realloc(temp, strlen(s) + 1); // shift all the text
            strcpy(temp, s);
            free(s);
        }

        int is_builtin = check_builtin(temp);

        // remove trailing whitespace from end of command
        i = strlen(temp) - 1;
        while (temp[i] == ' ' || temp[i] == '\t') i--;
        if (i < strlen(temp) - 1) {
            temp[i + 1] = '\0';
            temp = realloc(temp, strlen(temp) + 1);
        }

        // split args by whitespace
        char *args = NULL;
        char *arg_loc = strchr(temp, ' ');
        if (arg_loc) {
            args = calloc(strlen(arg_loc + 1) + 1, sizeof(char));
            strcpy(args, arg_loc + 1);
            temp[arg_loc - temp] = '\0';
            temp = realloc(temp, strlen(temp) + 1);
        }

        
        // check to see if we should run a builtin instead of an executable, if not: run that executable.
        if (!is_builtin && strlen(temp) > 0) {
            pid_t pid = fork();
            if (pid < 0) {
                fprintf(stderr, "Failed to fork %s\n", temp);
            } else if (pid == 0) {
                setpgid(0, getpid());
                start_app(temp, args, NULL);
                exit(0);
            }
            
            setpgid(pid, first_pgid ? first_pgid : pid); // these strange lines, both above and below, make it so that we can send signals to the running
                                                         // process and ignore them on our end, as well as make job control possible.
            signal(SIGTTOU, SIG_IGN);
            if (first_pgid) {
                tcsetpgrp(STDIN_FILENO, pid);
                tcsetpgrp(STDOUT_FILENO, pid);
            }
            
        }

        if (temp) free(temp);
        temp = NULL; // clean up command name
    } while ((cur_poi = strchr(cur_poi, ';')) != NULL); 

    pid_t wpid;
    int status = 0;
    while ((wpid = wait(&status)) > 0); // wait for all children to finish jobs, ensures we prompt once and only after all command are done.

    tcsetpgrp(STDOUT_FILENO, getpid());
    tcsetpgrp(STDIN_FILENO, getpid());

    return 0;
}