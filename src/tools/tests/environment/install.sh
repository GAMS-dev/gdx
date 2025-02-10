#!/bin/bash

ENVIRONMENT_DIRECTORY="$(dirname $0)"
GDX_DIRECTORY="$ENVIRONMENT_DIRECTORY/../../../.."

python3 -m venv "$GDX_DIRECTORY/venv"
source "$GDX_DIRECTORY/venv/bin/activate"

python3 -m pip install --upgrade pip
python3 -m pip install -r "$ENVIRONMENT_DIRECTORY/requirements.txt"
