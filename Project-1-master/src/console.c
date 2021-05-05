#include "console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

#include "parser.h"

#define LINE_LENGTH (512) // Continue reading after

static char prompt_text[PATH_MAX + 1]; // default prompt

void sigign_handler() {
    write(0, "\n", 1);
    write(1, prompt_text, strlen(prompt_text));
}

void tstp_handler() {
    char *txt = "\nThis shell does not support job control.\n";
    write(0, txt, strlen(txt));
    write(0, prompt_text, strlen(prompt_text));
}

void prompt() {
    // Check every prompt because it's possible it changed.
    char *ps1 = getenv("PS1");
    if (ps1) snprintf(prompt_text, PATH_MAX, "%s", ps1);
    else {
        char s[PATH_MAX - 2];
        getcwd(s, sizeof(s));
        snprintf(prompt_text, PATH_MAX, "%s$ ", s);
    }

    printf("%s", prompt_text); // important to note that this does not support
                               // normal ps1 syntax and will be displayed plainly.
}

void get_input(char **request) {
    if (!request || *request) {
        fprintf(stderr, "Severe error: %d\n", __LINE__); // Should never happen.
        exit(-1);
    }

    char buffer[LINE_LENGTH] = {0}; // init to all zeros or baaaad bug.
    char *temp = malloc(1); memset(temp, 0, 1); // Simply to not have to
                                                // call strcpy before strcat.

    int long_line = 0;
    while (1) {
        if (fgets(buffer, LINE_LENGTH, stdin) == 0) exit(0); // EOF

        temp = realloc(temp, (temp ? strlen(temp) : 0) + strlen(buffer) + 1); // build our user input LINE_LENGTH at a time
        strcat(temp, buffer); // add it to the full request

        if (buffer[strlen(buffer) - 1] == '\n') break; // break if end of line
        if (buffer[strlen(buffer) - 1] == '#') break; // break if comment

        long_line = 1; // print out warning message. inconsequential.
    }

    if (long_line) fprintf(stderr, "Warning: Long line.\n");
    if (temp && temp[0] == '#') return;

    *request = temp; // send it back to the caller.
}

void interact() {
    // The following lines let us ignore some killing signals, more importantly the reprint
    // the prompt on a new line. v
    struct sigaction sa;
    sa.sa_handler = sigign_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    struct sigaction tstp;
    tstp.sa_handler = tstp_handler;
    tstp.sa_flags = SA_RESTART;
    sigemptyset(&tstp.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        printf("Causing an interrupt will kill this shell!\n");
        perror("sigaction 0");
    }

    if (sigaction(SIGTSTP, &tstp, NULL) == -1) {
        printf("Causing a stop will kill this shell!\n");
        perror("sigaction 1");
    }

    if (sigaction(SIGQUIT, &sa, NULL) == -1) {
        printf("Causing a quit will kill this shell!\n");
        perror("sigaction 2");
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        printf("Causing a terminate will kill this shell!\n");
        perror("sigaction 3");
    }
    // to here ^

    while (1) { 
        prompt(); // send prompt

        char *request = NULL;
        get_input(&request); // get the request

        process_line(request); // process the request

        if (request) free(request); // free the request
    }
}