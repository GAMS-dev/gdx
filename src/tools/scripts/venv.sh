#!/bin/bash

set -e

source "$(dirname "$0")/directories.sh"

export UV_PROJECT_ENVIRONMENT="$VENV_DIRECTORY"
uv sync --project "$TESTS_DIRECTORY" --no-dev
