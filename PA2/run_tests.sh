#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
LEXER="$ROOT/lexer"
TESTDIR="$ROOT/tests"

if [[ ! -x "$LEXER" ]]; then
  echo "error: lexer not built; run 'make' first" >&2
  exit 1
fi

pass=0
fail=0

for input in "$TESTDIR"/*.cl; do
  [[ -e "$input" ]] || continue
  base="$(basename "$input" .cl)"
  expected="$TESTDIR/$base.expected"

  if [[ ! -f "$expected" ]]; then
    echo "SKIP $base (no expected output)"
    continue
  fi

  if diff -u "$expected" <(cd "$TESTDIR" && "$LEXER" "$base.cl") > /dev/null; then
    echo "PASS $base"
    pass=$((pass + 1))
  else
    echo "FAIL $base"
    diff -u "$expected" <(cd "$TESTDIR" && "$LEXER" "$base.cl") || true
    fail=$((fail + 1))
  fi
done

echo
echo "$pass passed, $fail failed"
[[ "$fail" -eq 0 ]]
