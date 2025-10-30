#!/usr/bin/env bash
set -e
files=$(git diff --name-only --cached --relative | grep -E '\.(cpp|c|cc|hpp|h|hh)$' || true)
if [ -z "$files" ]; then
  exit 0
fi
for f in $files; do
  clang-format -i "$f"
  git add "$f"
done