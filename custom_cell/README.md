# Custom Cell Configs
To create a custom cell you can create a C file in this directory
It will be compiled by the makefile into a `.so` file in the bin directory
This can then be used when running the main program
The empty cell config can be used as an example

# Rules
- Your implementation should be guided by `config.h`, implement the required parts of this interface in your code
- The implementation of these functions is up to the user
- Do not use any threads, the simulator itself uses threads to speed up (it is not the responsibility of the cell)
- `rand()` is seeded for you, so don't reseed.
