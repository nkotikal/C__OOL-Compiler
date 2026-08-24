#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
LEXER="$ROOT/../PA2/lexer"
PARSER="$ROOT/parser"
TESTDIR="$ROOT/tests"

if [[ ! -x "$LEXER" ]]; then
  echo "error: lexer not built; run 'make -C ../PA2 lexer' first" >&2
  exit 1
fi

if [[ ! -x "$PARSER" ]]; then
  echo "error: parser not built; run 'make' first" >&2
  exit 1
fi

run_parser() {
  local file="$1"
  (cd "$TESTDIR" && "$LEXER" "$file" 2>/dev/null | "$PARSER" "$file" >out.tmp 2>err.tmp; cat out.tmp err.tmp; rm -f out.tmp err.tmp)
}

pass=0
fail=0

for input in "$TESTDIR"/*.cl; do
  [[ -e "$input" ]] || continue
  base="$(basename "$input")"
  expected="$TESTDIR/${base%.cl}.expected"

  if [[ ! -f "$expected" ]]; then
    echo "SKIP ${base%.cl} (no expected output)"
    continue
  fi

  if diff -u "$expected" <(run_parser "$base") > /dev/null; then
    echo "PASS ${base%.cl}"
    pass=$((pass + 1))
  else
    echo "FAIL ${base%.cl}"
    diff -u "$expected" <(run_parser "$base") || true
    fail=$((fail + 1))
  fi
done

echo
echo "$pass passed, $fail failed"
[[ "$fail" -eq 0 ]]
