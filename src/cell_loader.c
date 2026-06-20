#include "cell_loader.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

// loads a data symbol from the provided handle
// note: terminates the program if the symbol cannot be loaded
static void *load_data_symbol(void *cell_handle, const char *symbol_name) {
  // clear any previous errors
  dlerror();

  void *symbol_ptr = dlsym(cell_handle, symbol_name);

  char *error = dlerror();
  if (error != NULL) {
    fprintf(stderr, "Error loading symbol \"%s\": %s\n", symbol_name, error);
    dlclose(cell_handle);
    exit(EXIT_FAILURE);
  }

  return symbol_ptr;
}

// loads a function pointer from the provided handle
// and makes sure there are no warnings from -pedantic
// note: terminates the program if the symbol cannot be loaded
static void (*load_func_symbol(void *cell_handle,
                               const char *symbol_name))(void) {
  // clear any previous errors
  dlerror();

  // use a union to avoid pedantic errors for casting
  // a void * to a function pointer (POSIX allows this)
  union {
    void (*func_ptr)(void);
    void *obj_ptr;
  } cast;

  cast.obj_ptr = dlsym(cell_handle, symbol_name);

  char *error = dlerror();
  if (error != NULL) {
    fprintf(stderr, "Error loading symbol \"%s\": %s\n", symbol_name, error);
    dlclose(cell_handle);
    exit(EXIT_FAILURE);
  }

  return cast.func_ptr;
}

CellConfig load_cell_config(const char *cell_path, void **out_handle) {
  // opens the provided library and loads all references now
  void *cell_handle = dlopen(cell_path, RTLD_NOW);
  assert(cell_handle != NULL);

  // clear any existing errors
  dlerror();

  size_t *cell_size_ptr = load_data_symbol(cell_handle, "cell_size");
  UpdateCell cell_update =
      (UpdateCell)load_func_symbol(cell_handle, "cell_update");
  InitialiseCell cell_init =
      (InitialiseCell)load_func_symbol(cell_handle, "cell_init");
  InitialiseDims dims_init =
      (InitialiseDims)load_func_symbol(cell_handle, "dims_init");

  CellConfig cell_config;
  cell_config.cell_size = *cell_size_ptr;
  cell_config.cell_update = cell_update;
  cell_config.cell_init = cell_init;
  cell_config.dims_init = dims_init;

  *out_handle = cell_handle;

  return cell_config;
}
