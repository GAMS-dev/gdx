#!/bin/bash

set -e

SCRIPTS_DIRECTORY="$(realpath "$(dirname "$0")")"
TOOLS_DIRECTORY="$(realpath "$SCRIPTS_DIRECTORY/..")"
TESTS_DIRECTORY="$TOOLS_DIRECTORY/tests"
GDX_DIRECTORY="$(realpath "$TOOLS_DIRECTORY/../..")"
VENV_DIRECTORY="$GDX_DIRECTORY/.venv"
