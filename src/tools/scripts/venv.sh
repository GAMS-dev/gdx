#!/bin/bash

set -e

# shellcheck source=./src/tools/scripts/directories.sh
source "$(dirname "$0")/directories.sh"

export UV_PROJECT_ENVIRONMENT="$VENV_DIRECTORY"
uv sync --directory "$TOOLS_DIRECTORY"
