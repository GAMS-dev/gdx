#!/bin/bash

set -e

# shellcheck source=./src/tools/scripts/directories.sh
source "$(dirname "$0")/directories.sh"

pushd "$GDX_DIRECTORY" >/dev/null

"$SCRIPTS_DIRECTORY/configure.sh"

pushd "$TOOLS_DIRECTORY" >/dev/null

if [[ "$(uname)" == 'Darwin' ]]; then
  PARALLEL_INSTANCES=$(sysctl -n hw.logicalcpu)
else
  PARALLEL_INSTANCES=$(nproc)
fi

set +e

run-clang-tidy -p "$GDX_DIRECTORY/build" \
  -j "$PARALLEL_INSTANCES" \
  -fix ./*/*.*pp

EXIT_CODE=$?

set -e

export UV_PROJECT_ENVIRONMENT="$VENV_DIRECTORY"
uv run clang-format -i ./*/*.*pp

popd >/dev/null
popd >/dev/null

exit $EXIT_CODE
