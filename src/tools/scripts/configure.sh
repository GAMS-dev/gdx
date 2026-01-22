#!/bin/bash

set -e

if [[ "$(uname)" == 'Darwin' ]]; then
  cmake -B ./build \
    -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
    -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
else
  cmake -B ./build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi
