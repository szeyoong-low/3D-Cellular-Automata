# Guidelines

## Code style
- Before committing, run [clang-format](https://clang.llvm.org/docs/ClangFormat.html) with `make format`, [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) with `make lint`, and [valgrind](https://valgrind.org/) with `make memleak`. The linter and formatter are configured to follow the llvm style. To run all 3, use `make all`. Use `make build` for compilation only.
- [This style guide](https://users.ece.cmu.edu/~eno/coding/CCodingStandard.html) has pretty reasonable suggestions on linting. But if it conflicts with clang-format/lint, clang-format/lint has the final say
- Descriptive variable names
- Binary constants should be defined in hex. All other constants should be defined in decimal. Whenever possible, specify the types of integer constants using the `U` (unsigned) and `L` (long) suffixes.
- Organise the program with header files in the appropriate include directory, and source files in the appropriate src directory
- Abstract Data Type pattern: types and related functions are kept in the same module (pair of header and source files)
- Validate programme state rigorously with assertions
- Use the `extern` and `static` keywords
- Use include guards in every header file. The phantom constant should be `MODULE_NAME_H`. However, if possible, try to remove transitive imports
- Do simple things like shifting and masking directly using built-in operators, don't define a function. Avoid macros as much as possible. There will not be a global utility function library
- The `CPU_State` is the programme's heartbeat. It should always be the first parameter of a function
- Global functions (`extern`) should be prepended with the module name
- `<inttypes.h>` is preferred over `<stdint.h>`
- Use `typedef` to alias meaningful types. You might consider placing them in the global types header. Avoid defining new `structs` if possible
- Use `#define` over `const` for defining constants as the former requires no memory at runtime. Define constants in the module where they most meaningful, so there will not be a global constants header
- Use `SCREAMING_SNAKE_CASE` convention when naming constants
- Pass `struct`s by pointer
- Error handling is the job of the callee, not the caller. Errors terminate the programme immediately.

## Build toolchain
- Use `make`, do not use `cb` or `CMake`
- Makefiles must be correct and efficient

## Testing
- Run test suite locally before committing by moving the executables from the `bin` directory to the test suite's `solution` directory, and executing `./run` and `./run -s` (yes, you must run both)

## Git
- Configure Git to use your full name and Imperial email using `git config <--global/local> user.<name/email> XXX`
- Always pull from main first, never merge a stale branch
- 1 branch per feature (do not delete after merge, but do not reuse branches)
- Do not commit build artifacts (`.o` or `.bit` files)
- Make small local commits whenever you have code that works
- Use [conventional commits](https://www.conventionalcommits.org/en/v1.0.0/), and keep messages meaningful
- Tag your short code in `[]` in front of the commit message. If doing pair programming, tag your partner too.
- Do not squash merge or rebase
- Always push your branches and open a draft PR, and describe the scope of work upfront

## Merge requests
- Merge requests should have 1 reviewer, but everyone should be informed of changes before. At least one other person who is less involved on the PR should go ahead
merging occurs
- Ping the chat to ask who's available for code review
- Merge requests must be complete features that pass the relevant tests, as `main` goes to LabTS

## Documentation
- Elaborate on technical decisions and project organisation in the [Wiki](https://gitlab.doc.ic.ac.uk/lab2526_summer/armv8_63/-/wikis/home) tab on GitLab, and in merge requests
- Useful commenting is much appreciated. Usage-related comments at point of declaration (headers), implementation-related comments at point of definition (source)
