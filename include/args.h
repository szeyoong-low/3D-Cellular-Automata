#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include <sys/types.h>

typedef struct {
  char *cell_path;
  size_t num_threads;
  uint width;
  uint height;
  uint depth;
  double steptime;
  bool opacity;
  bool lighting;
  bool custom_seed;
  uint seed;
  bool no_walls;
} args;

// parses the arguments passed into the program
// note: dlclose() is never called as we assume the library is needed
//       for the life of the program, where it is closed on exit
// side-effects: terminates the program if parsing fails
// return: a populated argument struct, with default values
//         (10x10x10 dimensions, max threads) for unset fields
extern args parse_args(int argc, char **argv);

#endif
