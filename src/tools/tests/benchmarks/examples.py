import os
import sys
from pathlib import Path

from ..common import DIRECTORY_PATHS, ExecutableName
from ..examples.full_example import create_full_example
from ..examples.small_example import create_small_example
from .common import benchmark_executable

FILE_NAMES = [
    "small_example",
    "full_example",
    "diff_file",
    "merge_file",
]
FILE_PATHS = {
    file_name: DIRECTORY_PATHS.examples / f"{file_name}.gdx" for file_name in FILE_NAMES
}


def main() -> int:
    create_small_example(FILE_PATHS["small_example"])
    create_full_example(FILE_PATHS["full_example"])

    commands: dict[ExecutableName, list[str | Path]] = {
        "gdxdump": [
            FILE_PATHS["full_example"],
        ],
        "gdxdiff": [
            FILE_PATHS["small_example"],
            FILE_PATHS["full_example"],
            FILE_PATHS["diff_file"],
        ],
        "gdxmerge": [
            FILE_PATHS["small_example"],
            FILE_PATHS["full_example"],
            f"Output={FILE_PATHS['merge_file']}",
        ],
    }
    for i, executable_name in enumerate(commands):
        benchmark_executable(executable_name, commands[executable_name])
        if i < len(commands) - 1:
            print("\n")

    for file_path in FILE_PATHS.values():
        os.remove(file_path)

    return 0


if __name__ == "__main__":
    sys.exit(main())
