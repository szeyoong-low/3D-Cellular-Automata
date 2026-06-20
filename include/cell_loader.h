#ifndef CELL_LOADER_H
#define CELL_LOADER_H

#include "simulate.h"

// opens the library referenced by path and attempts to extract
// the functions/data from within
// side-effect: terminates the program if the library cannot be
//              loaded or the symbols required can't be loaded
//              and loads the handle into out_handle to allow for
//              it to be closed
// return: a CellConfig populated from the symbols exported by the library
extern CellConfig load_cell_config(const char *cell_path, void **out_handle);

#endif
