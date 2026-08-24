# COOL Lexer

Hand-written lexical analyzer for the Classroom Object-Oriented Language (COOL), built with [Flex](https://github.com/westes/flex).

## Building

```bash
make
```

This produces the `lexer` binary, a stand-alone token dumper for COOL source files.

## Usage

```bash
./lexer path/to/file.cl
```

Each token is printed on its own line with the source line number and token kind. String and error payloads are quoted and escaped for readability.

## Testing

```bash
make test
```

Tests live in `tests/` as paired `.cl` input and `.expected` output files. The suite covers:

- Keywords, operators, and punctuation
- Integer, type, and object identifiers
- Case rules for keywords vs. boolean literals
- Single-line and nested block comments
- String literals, escape sequences, and line continuations
- Lexical error recovery and line-number tracking

## Source layout

| File | Role |
|------|------|
| `cool.flex` | Lexer specification (edit this) |
| `cool-lex.cc` | Flex-generated scanner (do not edit) |
| `lextest.cc` | Stand-alone driver |
| `utilities.cc` | Token formatting helpers |
| `stringtab.cc` | Interned string tables |
| `handle_flags.cc` | Command-line flag parsing |

Header files (`cool-parse.h`, `stringtab.h`, etc.) come from the course infrastructure at `/usr/class/include/PA2`.

## Design notes

**Comments.** `--` comments run to end-of-line. `(* ... *)` comments nest; an unclosed block comment or stray `*)` is a lexical error.

**Strings.** Standard escapes (`\n`, `\t`, `\b`, `\f`) are supported, along with backslash-newline continuation and generic `\<char>` for any other character. Null bytes in strings are rejected. Constants are capped at 1024 characters.

**Identifiers.** Type identifiers start with an uppercase letter; object identifiers start with lowercase. Both may contain letters, digits, and underscores.

**Keywords.** Reserved words are matched case-insensitively, except `true` and `false`, which must begin with lowercase `t` and `f` respectively.

**Errors.** After a string error, the lexer skips ahead to the closing quote (or newline) before resuming normal scanning, so later tokens are still reported.
