#!/bin/bash

set -e

SCRIPTS_DIRECTORY="$(realpath "$(dirname "$0")")"
TOOLS_DIRECTORY="$(realpath "$SCRIPTS_DIRECTORY/..")"
GDX_DIRECTORY="$(realpath "$TOOLS_DIRECTORY/../..")"
