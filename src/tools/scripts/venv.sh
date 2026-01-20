#!/bin/bash

set -e

source "$(dirname "$0")/directories.sh"

export UV_PROJECT_ENVIRONMENT="$VENV_DIRECTORY"
uv sync --directory "$TOOLS_DIRECTORY" --no-dev
