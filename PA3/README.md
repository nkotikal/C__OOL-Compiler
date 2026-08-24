# COOL Parser

LALR parser for COOL, built with [Bison](https://www.gnu.org/software/bison/). Reads tokens from the PA2 lexer and builds an abstract syntax tree (AST).

## Prerequisites

Build the lexer first:

```bash
make -C ../PA2 lexer
```

## Building

```bash
make
```

This produces the `parser` binary.

## Usage

The parser reads a token stream from stdin (typically piped from the lexer):

```bash
../PA2/lexer file.cl | ./parser file.cl
```

On success, the AST is printed to stdout. Syntax errors go to stderr and the process exits with status 1.

## Testing

```bash
make test
```

Tests live in `tests/` as paired `.cl` input and `.expected` output files. The suite covers:

- Class declarations, inheritance, attributes, and methods
- Expression forms: arithmetic, conditionals, loops, blocks, `let`, `case`
- Dispatch (static and dynamic) and formal parameters
- Operator precedence and associativity
- Syntax error reporting and parser recovery

## Source layout

| File | Role |
|------|------|
| `cool.y` | Grammar specification (edit this) |
| `cool-parse.cc` | Bison-generated parser (do not edit) |
| `parser-phase.cc` | Stand-alone driver |
| `tokens-lex.cc` | Token reader for the parser pipeline |
| `cool-tree.cc` | AST node constructors |
| `dumptype.cc` | AST pretty-printer |

Header files come from the course infrastructure at `/usr/class/include/PA3`.

## Design notes

**Grammar structure.** Programs are lists of classes. Each class has features (attributes or methods). Expressions cover the full COOL surface syntax with precedence declared for arithmetic, comparison, dispatch, and unary operators.

**Error recovery.** Error productions on `class_list`, `feature_list`, and `expr_list` allow the parser to continue after localized mistakes. `let_expr` has recovery rules for malformed bindings.

**Default inheritance.** A class without an explicit `inherits` clause defaults to `Object`.

**Let associativity.** The `FLAG` precedence token resolves ambiguity between `let x : T in e` and `let x : T, y : U in e`.
