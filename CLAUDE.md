# CLAUDE.md

## About this project
I'm working in a team of 4 to build a 3D cellular automata simulation in a week. We must use the C programming language, but we are free to use libraries from other languages.

## File structure
- All C headers in [`include`](/include/)
- All C source files in [`src`](/src/)
- All GLSL shaders in [`shaders`](/shaders/)
- All simulation configurations in [`custom_cell`](/custom_cell/)
- Entry point is [`cellular_automata.c`](/cellular_automata.c)
- Overall build toolchain is [`Makefile`](/Makefile)

## Objectives
- [GitHub issues](https://github.com/szeyoong-low/3D-Cellular-Automata/issues)

## Way of working — read before answering any prompt
The highest priority of this project is learning. I want to be capable of making
architectural decisions and reason about them. I would like the pace to be
manageable so that I can stay in control. Treat me as a rookie in graphics rendering
and accelerated computing, but also treat me as the person with the final say over
every detail in this project.

- My aim is to build a minimum viable product while making good design choices
  that scale well. Everything else is secondary.
- I will break work down into bite-sized chunks. Only work on these atomic
  pieces, **do not do more than asked for**.
- However, you are more than welcome to **challenge my decisions**
  constructively and suggest **potential improvements** and **next steps**.
- Instead of giving you objectives to fulfil (e.g. Build this feature), I will
  pose **questions** (e.g. How can this be built?)
- Start off by **suggesting an approach**, and always explain your architectural
  decisions like I'm a **first-timer** but **be concise**. Highlight the tradeoffs
  involved, and whether this decision will need to change down the line. I want
  a full understanding of the solution's rationale before any code is written.
- Give me **sources** where I can read more about these architectural details.
- Wait for my **explicit go ahead** before writing code.
- Make **as few changes as possible**. Explain what every change does and why
  **before** creating it. **One file at a time.** Wait for approval before
  moving to the next step.
- Prefer **simple, readable, and maintanable over clever**. No unnecessary
  abstractions, we can always refactor later.
- If a problem has a well-documented solution or design pattern regarded
  by industry as **best practice**, use it. If a library or external service can
  solve it, use it. I want to focus on building the interesting parts rather
  than reinventing the wheel. If there are real concerns about maintainability,
  we can refactor this later on.
- **Comment** every non-obvious line.
- If a task touches multiple concerns, **split** it into sequential steps.
- **Never run destructive commands** (e.g. drop tables, delete files, reset DBs)
  without explicit confirmation.
- Explain error messages before fixing them — **teach, don't just patch**. Let
  me try to debug first, don't give me the solution.
- Make sure you give me a heads up before **copying** or lightly adapting code
  from a single source. I may need to give acknowledgements as this is a
  portfolio project.
- Details like version numbers change constantly. I expect you to flag potential
  discrepancies (e.g. outdated version numbers, shell scripts not matching docs,
  dependency names different from examples in documentation) so that I can cross
  check them online.
- Never ever allow production to fail. We should check code rigorously before
  any PR gets merged to the `extension` branch.