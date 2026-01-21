#!/bin/bash
# shellcheck disable=SC2034

set -e

SCRIPTS_DIRECTORY="$(realpath "$(dirname "$0")")"
TOOLS_DIRECTORY="$(realpath "$SCRIPTS_DIRECTORY/..")"
GDX_DIRECTORY="$(realpath "$TOOLS_DIRECTORY/../..")"
VENV_DIRECTORY="$GDX_DIRECTORY/.venv"
