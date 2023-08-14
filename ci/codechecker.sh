#!/usr/bin/env bash
# Stolen from @afust and modified
# see: https://git.gams.com/devel/gams-mii/-/blob/develop/ci/codechecker.sh
# Log your project.
CodeChecker log -b "make -j4" -o compilation_database.json
CodeChecker analyze --analyzers clang-tidy clangsa -o reports compilation_database.json
# Create the report file by using the CodeChecker parse command.
CodeChecker parse --trim-path-prefix $(pwd) -e codeclimate reports > gl-code-quality-report.json
exit 0
