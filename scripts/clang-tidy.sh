#!/usr/bin/env bash
set -e
BUILD_DIR=build/debug
if [ ! -d "$BUILD_DIR" ]; then
  cmake -S . -B "$BUILD_DIR" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi
# run clang-tidy for changed source files (do not fail commit; adjust if you want strict)
files=$(git diff --name-only --cached --relative | grep -E '\.(cpp|c|cc)$' || true)
if [ -z "$files" ]; then
  exit 0
fi
for f in $files; do
  echo "clang-tidy -> $f"
  clang-tidy "$f" -p="$BUILD_DIR" || true
done