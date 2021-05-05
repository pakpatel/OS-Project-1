/*
 * @title   Operating Systems Project 1
 * @author  Group Raspberry
 * @date    Due February 16, 2021
 *
 * The goal is to write a command line interpreter, 
 * aka a shell, to run on unix-based systems.
 */

// Pound Defines (#D)



// End #D

// System Includes (SI)

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// End SI

// Local Includes (LI)

#include "console.h"
#include "process_batch.h"

// End LI

int main(int argc, char ** argv) {
  if (argc > 1) {
    for (int i = 1; i < argc; ++i) {
      if (strcmp(argv[i], "-i") == 0) clearenv();
      else if (argv[i][0] != '-') {
        parse_batch(argc - i, argv + i);
        return 0;
      }
      else {
        fprintf(stderr, "Unknown argument: %s\n", argv[i]);
        return -1;
      }
    }
  }
  
  interact();

  return 0;

}
