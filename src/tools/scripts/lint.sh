#!/bin/bash

set -e

# shellcheck source=./src/tools/scripts/directories.sh
source "$(dirname "$0")/directories.sh"

pushd "$GDX_DIRECTORY" >/dev/null

"$SCRIPTS_DIRECTORY/configure.sh"

pushd "$TOOLS_DIRECTORY" >/dev/null

export UV_PROJECT_ENVIRONMENT="$VENV_DIRECTORY"

uv run clang-format -i ./*/*.*pp

if [[ "$(uname)" == 'Darwin' ]]; then
  PARALLEL_INSTANCES=$(sysctl -n hw.logicalcpu)
else
  PARALLEL_INSTANCES=$(nproc)
fi

uv run clangd-tidy \
  -j "$PARALLEL_INSTANCES" \
  ./*/*.*pp

uv run ruff check --fix .
uv run ruff format .

uv run ty check .
uv run mypy --strict .

popd >/dev/null

uv run pre-commit run --all-files

popd >/dev/null
