#include "process_batch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "run_program.h"
#include "parser.h"


int parse_batch(int argc, char **argv) {
    FILE *f;

    // open file to process
    f = fopen(argv[0], "r");
    if (f == NULL) {
        start_app(argv[0], NULL, argv); // try to start it anyways, it might be in PATH.
        return 0;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t bytes_read;
    
    if ((bytes_read = getline(&line, &len, f)) != -1) {
        if (bytes_read > 2 && line[0] == '#' && line[1] == '!') { // check for shebang
            if (line[bytes_read - 1] == '\n') line[bytes_read - 1] = '\0'; // remove new line in filename
            
            start_app(argv[0], NULL, argv); // start the shebang with the arguments provided
            return 0;
        }
    }


    // get the length of the file needed to be read.
    fseek(f, 0L, SEEK_END);
    len = ftell(f);
    fseek(f, 0L, SEEK_SET);

    char read[len];
    char c = 0;
    int i = 0;

    // bulid a buffer of our lines
    while((c = fgetc(f)) != EOF) {
        read[i++] = (c == '\n') ? '\0' : c; // replace new lines with nulls on the fly.
        if (i > len) {
            fprintf(stderr, "Fatal Error!\n");
            exit(-1);
        }
    }


    for (int i = 0; i < len; ++i) {
        printf("%s\n", read + i);
        process_line(read + i);
        i += strlen(read + i); // send pointers to process line that are the characters inbetween the new lines (nulls).
    }


    fclose(f);

    return 0;
}