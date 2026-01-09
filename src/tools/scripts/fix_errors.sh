#!/bin/bash

set -e

source "$(dirname "$0")/directories.sh"

pushd "$GDX_DIRECTORY" > /dev/null

"$SCRIPTS_DIRECTORY/configure.sh"

pushd "$TOOLS_DIRECTORY" > /dev/null

clang-tidy -p "$GDX_DIRECTORY/build" \
  --fix-errors \
  ./*/*.*pp

# run-clang-tidy -p "$GDX_DIRECTORY/build" \
#   -fix -j 4 ./*/*.*pp

clang-format -i ./*/*.*pp

popd > /dev/null
popd > /dev/null
