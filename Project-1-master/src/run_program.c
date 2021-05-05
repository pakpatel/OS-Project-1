#include "run_program.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "process_batch.h"

char ** argify(char * filename, char * args) {
  char ** temp_args = calloc(1, sizeof(char*)); // allocate space for required filename argv[0]
  temp_args[0] = calloc(strlen(filename) + 1, sizeof(char)); // allocate space for filename itself
  strcpy(temp_args[0], filename); // set first element of argv to filename

  // look whitespace but ignore inside quotes
  size_t len = strlen(args) + 1;
  for (size_t i = 0; i < len; ++i) {
    if (args[i] == '\'') while (args[++i] != '\'');
    if (args[i] == '\"') while (args[++i] != '\"');
    if (!ispunct(args[i]) && !isalnum(args[i])) args[i] = '\0';
  }

  // skip the whitespace at the beginning
  size_t count = 1;
  size_t skip = 0;
  while (args[skip] == '\0') if (++skip > len) break;
  char *prev = args + skip;

  for (size_t i = skip; i < len; ++i) {
    if (args[i] == '\0') { // if it is null, the command is everything preceeding it until the next null.
      temp_args = realloc(temp_args, (++count) * sizeof(char*));
      temp_args[count - 1] = calloc(strlen(prev) + 1, sizeof(char));
      strcpy(temp_args[count - 1], prev); // set our args to include new found args.

      while (args[++i] == '\0') if (i > len) break; // loop until end of nullbytes to find start of next arg.
      prev = args + i; // skip final nullbyte.
    }
  }

  temp_args = realloc(temp_args, (count+1) * sizeof(char*)); // argv must be terminated with a nullpointer (pointer to null, not null itself)
  temp_args[count] = NULL; // setting it to null here.

  return temp_args; // return generated list.
}

// Returns null when best option is the name provided.
char * get_absolute_path(char *filename) {
  if (!filename) return NULL; // gotta have a filename.
  
  char *path = NULL;
  if ((path = getenv("PATH")) == NULL) return NULL; // determine if path is set, if so, use it
  if (filename[0] == '/' || (strlen(filename) > 1 ? (filename[0] == '.') && (filename[1] == '/') : 0)) return NULL; // if best route is local, take it

  char *path_copy = calloc(strlen(path) + 1, sizeof(char)); // dont destroy path environment variable
  strcpy(path_copy, path);

  const char del[2] = ":"; 
  char *token;

  token = strtok(path_copy, del); // separate path into delimited sections (:)
  
  while (token != NULL) { // loop until no more paths
    char full_name[FILENAME_MAX]; // for our path's decision
    snprintf(full_name, FILENAME_MAX - 1, "%s/%s", token, filename); // tack on the path to the filename

    struct stat sb;
    // first, determine if the file exists, then determine if it is executable
    if ((access(full_name, F_OK) == 0) && ((stat(full_name, &sb) == 0) && sb.st_mode & S_IXUSR)) {
      char *ret = calloc(strlen(full_name) + 1, sizeof(char));
      strcpy(ret, full_name);
      return ret; // send this successful path back to caller.
    }

    token = strtok(NULL, del); // continue looking
  }

  return NULL; // didn't find anything in the path, try what you sent us.
}

void start_app(char * app_name, char * args, char ** argv) {
  // Do some setup before running application.
  // HERE

  char *path = NULL;
  if ((path = get_absolute_path(app_name)) == NULL) path = app_name; // look for executable in path


  pid_t pid = fork(); // create child process which will be taken over by the request
  if (pid < 0) { // Failed to fork.
    fprintf(stderr, "Failed to fork %s\n", app_name);
    return;
  }

  if (pid == 0) { // Child process, run the program.
    // Setup directly before the process starts
    // Maybe cleanup file descriptors.

    setpgid(0, getpid());
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    
    if (args && !argv) { // will prefer argv
      argv = argify(app_name, args); // create argument list
    } else { // if no args, just create generic list with app name and nullbyte
      argv = calloc(2, sizeof(char*)); 
      argv[0] = calloc(strlen(app_name) + 1, sizeof(char));
      strcpy(argv[0], app_name);
    }


    int ret = execv(path, argv); // run the program

    switch(errno) {
    case E2BIG:
      fprintf(stderr, "Environment or arguments too large!\n");
      break;
    case EACCES:
      fprintf(stderr, "File is not executable!\n");
      break;
    case EFAULT:
      fprintf(stderr, "Filename fault!\n");
      break;
    case EINVAL:
      fprintf(stderr, "ELF invalid!\n");
      break;
    case EIO:
      fprintf(stderr, "I/O error!\n");
      break;
    case EISDIR:
      fprintf(stderr, "ELF interpreter is a directory!\n");
      break;
    case ELIBBAD:
      fprintf(stderr, "ELF interpreter format not recognized!\n");
      break;
    case ELOOP:
      fprintf(stderr, "Too many symbolic links were encountered in resolving filename or the name of a script or ELF interpreter.\n");
      break;
    case EMFILE:
      fprintf(stderr, "Process has too many open files!\n");
      break;
    case ENAMETOOLONG:
      fprintf(stderr, "Filename is too long!\n"); // I don't actually think this is possible here...
      break;
    case ENFILE:
      fprintf(stderr, "System open file limit reached!\n");
      break;
    case ENOENT:
      fprintf(stderr, "Command '%s' not found!\n", app_name);
      break;
    case ENOEXEC: ;
      int argc = 0;
      while (argv[argc] != NULL) ++argc;
      parse_batch(argc, argv);
      break;
    case ENOMEM:
      fprintf(stderr, "Ran out of memory!\n");
      break;
    case EPERM:
      fprintf(stderr, "You do not have permission to run this!\n");
      break;
    case ETXTBSY:
      fprintf(stderr, "Executable is open for writing elsewhere!\n");
      break;
    default:
      fprintf(stderr, "An unknown error occured\n"); // includes ENOTDIR
      break;
    }

    exit(ret);
  } else { // Parent process, this is still our shell.
    setpgid(pid, pid);
    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDIN_FILENO, pid);
    tcsetpgrp(STDOUT_FILENO, pid);

    int status = 0;
    while (1) {
      waitpid(pid, &status, WUNTRACED); // Reap the child process so we don't have any zombies running around
                                        // as well as wait until the process is over to present a prompt.

      if (WIFSTOPPED(status)) {
        fprintf(stderr, "This shell does not support job control.\n");
        fprintf(stderr, "Resuming in 3 seconds...\n");
        sleep(3);
        kill(pid, SIGCONT);
      } else break;
    }

    tcsetpgrp(STDOUT_FILENO, getpid());
    tcsetpgrp(STDIN_FILENO, getpid());

    // Avoiding garbage collection, this thread gets destroyed anyways.
  }
}
