#!/usr/bin/env bash
set -e
# run clang-tidy for changed source files
files=$(git diff --name-only --cached --relative | grep -E '\.(cpp|c|cc)$' || true)
if [ -z "$files" ]; then
  exit 0
fi
for f in $files; do
  echo "clang-tidy -> $f"
  clang-tidy "$f" -p="$BUILD_DIR" || true
done