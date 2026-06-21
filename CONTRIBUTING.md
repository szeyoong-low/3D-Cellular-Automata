# Guidelines

## Build toolchain
- Dependencies: see tech stack under [README](/README.md). You will need `gcc`, `clang-tidy`, and `clang-format`.
- Run `make build` from the project root. Add the `clean` target if you wish to start afresh, and run `make all` if you want to run the formatter and linter as well.
- To run a simulator, execute `./bin/cellular_automata ./bin/<simulation name>`. Use `--help` to see available flags.

## Code style
- Before committing, run [clang-format](https://clang.llvm.org/docs/ClangFormat.html) with `make format`, [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) with `make lint`, and [valgrind](https://valgrind.org/) with `make memleak`. The linter and formatter are configured to follow the llvm style. To run all 3, use `make all`. Use `make build` for compilation only.
- [This style guide](https://users.ece.cmu.edu/~eno/coding/CCodingStandard.html) has pretty reasonable suggestions on linting. But if it conflicts with clang-format/lint, clang-format/lint has the final say
- Descriptive variable names
- Binary constants should be defined in hex. All other constants should be defined in decimal. Whenever possible, specify the types of integer constants using the `U` (unsigned) and `L` (long) suffixes.
- Organise the program with header files in the appropriate include directory, and source files in the appropriate src directory
- Abstract Data Type pattern: types and related functions are kept in the same module (pair of header and source files)
- Use the `extern` and `static` keywords
- Use include guards in every header file. The phantom constant should be `MODULE_NAME_H`. However, if possible, try to remove transitive imports
- Do simple things like shifting and masking directly using built-in operators, don't define a function. Avoid macros as much as possible. There will not be a global utility function library
- Global functions (`extern`) should be prepended with the module name
- `<inttypes.h>` is preferred over `<stdint.h>`
- Use `typedef` to alias meaningful types. You might consider placing them in the global types header. Avoid defining new `structs` if possible
- Use `#define` over `const` for defining constants as the former requires no memory at runtime. Define constants in the module where they most meaningful, so there will not be a global constants header
- Use `SCREAMING_SNAKE_CASE` convention when naming constants
- Pass `struct`s by pointer
- Error handling is the job of the callee, not the caller. Errors terminate the programme immediately.

For all other matters, we will follow the conventions that we have been observing thus far.

## Git
- Do not commit to the Imperial GitLab repo anymore! Do not commit to the private archive!
- Configure Git to use your full name and the email registered with GitHub using `git config <--global/local> user.<name/email> XXX`
- Always pull from main first, never merge a stale branch
- 1 branch per feature (do not delete after merge, but do not reuse branches)
- Do not commit build artifacts (`.o` or `.bit` files)
- Make small local commits whenever you have code that works
- Use [conventional commits](https://www.conventionalcommits.org/en/v1.0.0/), and keep messages meaningful
- No need to tag your short code in front of the commit message
- Do not squash merge or rebase
- Always push your branches and open a draft PR, and describe the scope of work upfront

## Pull requests, CI/CD
- Never push to main (it is barred by branch protection anyways)
- Pull requests should have 1 reviewer, but everyone should be informed of changes before. Ideally, one other person who is less involved on the PR should give the go ahead for merging
- Ping the chat to ask who's available for code review
- Pull requests must be complete features that pass the relevant tests
- Make feature suggestions and report bugs in the [Issues](https://github.com/szeyoong-low/3D-Cellular-Automata/issues) tab

## Branch protection and repo settings
- Auto-close issues with merged linked pull requests
- Allow comments on individual commits
- Always suggest updating pull request branches
- Allow merge commits only (no squash/rebase)
- Main branch
    - Require a pull request before merging
    - Require approvals (1)
    - Dismiss stale pull request approvals when new commits are pushed
    - Require conversation resolution before merging
    - Do not allow bypassing the above settings


## Documentation
- Elaborate on technical decisions and project organisation in the [Wiki](https://github.com/szeyoong-low/3D-Cellular-Automata/wiki) tab on GitHub, and in [pull requests](https://github.com/szeyoong-low/3D-Cellular-Automata/pulls)
- Useful commenting is much appreciated. Usage-related comments at point of declaration (headers), implementation-related comments at point of definition (source)
