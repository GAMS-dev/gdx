#!/bin/zsh

export MallocStackLogging=1

EXECUTABLE_PATH='../../../../build/gdxdump'
GDX_FILE_PATH="$HOME/Documents/GAMS/Studio/workspace/test.gdx"

leaks -list -atExit -- $EXECUTABLE_PATH $GDX_FILE_PATH $@
