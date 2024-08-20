#!/bin/zsh

DIRECTORY_PATH="$(dirname $0)"

python3 -m venv "$DIRECTORY_PATH/../../venv"
source "$DIRECTORY_PATH/../../venv/bin/activate"
python3 -m pip install -r "$DIRECTORY_PATH/requirements.txt"
