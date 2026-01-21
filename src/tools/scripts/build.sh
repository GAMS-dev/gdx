#!/bin/bash

set -e

# shellcheck source=./src/tools/scripts/directories.sh
source "$(dirname "$0")/directories.sh"

pushd "$GDX_DIRECTORY" > /dev/null

"$SCRIPTS_DIRECTORY/configure.sh"

cmake --build ./build

popd > /dev/null
