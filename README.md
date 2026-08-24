# C__OOL-Compiler

A from-scratch compiler for the COOL (Classroom Object-Oriented Language), covering lexing, parsing, and code generation.

## Components

| Phase | Directory | Primary implementation | Status |
|-------|-----------|------------------------|--------|
| Lexer | `PA2/` | [`cool.flex`](PA2/cool.flex) | Complete |
| Parser | `PA3/` | [`cool.y`](PA3/cool.y) | Complete |

Most of the compiler logic for these phases lives in those two files; the rest of each directory is build tooling, drivers, and course-provided infrastructure.

## Quick start

```bash
cd PA2 && make && make test    # build and test the lexer
cd PA3 && make && make test    # build and test the parser (requires PA2 lexer)
```
