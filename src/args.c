#include "args.h"
#include <argp.h>
#include <stdlib.h>

#define THREADS_FLAG 't'
#define DIMENSIONS_FLAG 'd'
#define STEPTIME_FLAG 's'
#define OPACITY_FLAG 'o'
#define LIGHTING_FLAG 'l'
#define INIT_SEED_FLAG 'i'

// items for argp
// provides a version for --version
const char *argp_program_version = "Cellular Automata 1.0";
// adds this to the top of --help
static char doc[] = "A multi-threaded 3D cellular automata simulation engine";
// specifies that a custom cell path is needed
static char args_doc[] = "CELL_PATH";

// the possible flags
static struct argp_option options[] = {
    // takes a name, key, arg type, flag, doc, group
    {"threads", THREADS_FLAG, "INT", 0,
     "Number of threads for the simulation (must be > 0)", 0},
    {"dimensions", DIMENSIONS_FLAG, "WxHxD", 0,
     "Grid dimensions specified as WIDTHxHEIGHTxDEPTH", 0},
    {"steptime", STEPTIME_FLAG, "DOUBLE", 0,
     "The minimum time for a step (could be longer due to performance issues, "
     "must be > 0)",
     0},
    {"opacity", OPACITY_FLAG, NULL, 0, "Enable support for opacity", 0},
    {"lighting", LIGHTING_FLAG, NULL, 0, "Enable Phong lighting", 0},
    {"initialseed", INIT_SEED_FLAG, "UINT", 0,
     "Set a custom starting seed for random number generator", 0},
    // array end
    {0}};

// decides how to parse our arguments
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  args *args = state->input;

  switch (key) {
  case THREADS_FLAG: {
    char *endptr;
    long val = strtol(arg, &endptr, 10);
    if (*endptr != '\0' || val <= 0) {
      argp_error(state, "Flag -t requires an integer greater than 0");
    }
    args->num_threads = (size_t)val;
    break;
  }
  case DIMENSIONS_FLAG: {
    int width, height, depth;
    if (sscanf(arg, "%dx%dx%d", &width, &height, &depth) != 3 || width <= 0 ||
        height <= 0 || depth <= 0) {
      argp_error(
          state,
          "Invalid dimension \"%s\"Must be positive integers (e.g. 10x10x10)",
          arg);
    }
    args->width = (uint)width;
    args->height = (uint)height;
    args->depth = (uint)depth;
    break;
  }
  case STEPTIME_FLAG: {
    char *endptr;
    double time = strtod(arg, &endptr);
    if (*endptr != '\0' || time <= 0) {
      argp_error(state, "Flag -s requires a double greater than 0");
    }
    args->steptime = time;
    break;
  }
  case OPACITY_FLAG: {
    args->opacity = true;
    break;
  }
  case LIGHTING_FLAG: {
    args->lighting = true;
    break;
  }
  case INIT_SEED_FLAG: {
    args->custom_seed = true;
    char *endptr;
    long val = strtol(arg, &endptr, 10);
    if (*endptr != '\0' || val < 0) {
      argp_error(state,
                 "Flag -i requires an integer greater than or equal to 0");
    }
    args->seed = (uint)val;
    break;
  }
  // handles a string not attached to a flag (our cell path)
  case ARGP_KEY_ARG: {
    // checks the number of strings passed
    if (state->arg_num >= 1) {
      // print usage instructions and exit
      argp_usage(state);
    }
    args->cell_path = arg;
    break;
  }
  // triggers once all args have been checked
  case ARGP_KEY_END: {
    if (state->arg_num < 1) {
      argp_usage(state);
    }
    break;
  }
  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, NULL, NULL, NULL};

args parse_args(int argc, char **argv) {
  args args = {
      .cell_path = NULL,
      .num_threads = 0,
      .width = 10,
      .height = 10,
      .depth = 10,
      .steptime = 1,
      .opacity = false,
      .lighting = false,
      .custom_seed = false,
      .seed = 0, // Just a dummy number
  };

  // parse cli args
  if (argp_parse(&argp, argc, argv, 0, 0, &args) != 0) {
    exit(EXIT_FAILURE);
  }

  return args;
}
