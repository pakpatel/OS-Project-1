#ifndef __BUILTINS_H__
#define __BUILTINS_H__


extern void builtin_exit(); // exits the program
extern void builtin_env(); // prints out the current environment
extern void builtin_export(char *command); // should export env variable TODO
extern void builtin_unset(char *command); // should export env variable TODO
extern void builtin_chdir(char *command); // equiv to cd


#endif