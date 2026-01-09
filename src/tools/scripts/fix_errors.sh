#!/bin/bash

set -e

ORIGINAL_DIRECTORY="$(pwd)"
TOOLS_DIRECTORY="$(realpath "$(dirname "$0")")"
GDX_DIRECTORY="$(realpath "$TOOLS_DIRECTORY/../..")"

cd "$GDX_DIRECTORY"

cmake -B ./build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cd "$TOOLS_DIRECTORY"

if [[ "$(uname)" == 'Linux' ]]; then
  clang-tidy -p "$GDX_DIRECTORY/build" \
    --fix-errors \
    ./*/*.*pp

  # run-clang-tidy -p "$GDX_DIRECTORY/build" \
  #   -fix -j 4 ./*/*.*pp

elif [[ "$(uname)" == 'Darwin' ]]; then
  # clang-tidy -p "$GDX_DIRECTORY/build" \
  #   --extra-arg=-isysroot"$(xcrun --show-sdk-path)" \
  #   --fix-errors \
  #   ./*/*.*pp

  run-clang-tidy -p "$GDX_DIRECTORY/build" \
    -extra-arg=-isysroot"$(xcrun --show-sdk-path)" \
    -fix -j 4 ./*/*.*pp
fi

clang-format -i ./*/*.*pp

cd "$ORIGINAL_DIRECTORY"
