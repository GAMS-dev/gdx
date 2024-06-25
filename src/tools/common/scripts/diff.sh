#!/bin/zsh

EXECUTABLE_PATH_GAMS='/Library/Frameworks/GAMS.framework/Resources/gdxdump'
EXECUTABLE_PATH_TEST='../../../../build/gdxdump'
GDX_FILE_PATH="$HOME/Documents/GAMS/Studio/workspace/test.gdx"

# --side-by-side
diff --unified --color \
    <($EXECUTABLE_PATH_GAMS $GDX_FILE_PATH) \
    <($EXECUTABLE_PATH_TEST $GDX_FILE_PATH) \
    $@
