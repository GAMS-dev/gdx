import os
import subprocess
import sys

from ..common import DIRECTORY_PATHS, get_executable_path
from ..examples.full_example import create_full_example
from ..examples.small_example import create_small_example

FILE_NAMES = ["small_example", "full_example", "diff_file", "merge_file"]
FILE_PATHS = {
    file_name: os.path.join(DIRECTORY_PATHS.examples, f"{file_name}.gdx")
    for file_name in FILE_NAMES
}


def benchmark_executable(executable_name: str, command: list[str]) -> None:
    executable_path = get_executable_path(executable_name)
    full_command = [
        "hyperfine",
        "--shell=none",
        "--warmup",
        "5",
        "--ignore-failure",
        "--export-markdown",
        os.path.join(DIRECTORY_PATHS.results, f"{executable_name}.md"),
        "--command-name",
        f"{executable_name} (C++)",
        " ".join([os.path.join(*executable_path), *command]),
        "--command-name",
        f"{executable_name} (Delphi)",
        " ".join([executable_name, *command]),
    ]
    print(f"{' '.join(full_command)}\n")
    os.makedirs(DIRECTORY_PATHS.results, exist_ok=True)
    subprocess.run(full_command)


def main() -> int:
    create_small_example(FILE_PATHS["small_example"])
    create_full_example(FILE_PATHS["full_example"])

    executables: dict[str, list[str]] = {
        "gdxdump": [FILE_PATHS["full_example"]],
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
    for i, executable_name in enumerate(executables):
        benchmark_executable(executable_name, executables[executable_name])
        if i < len(executables) - 1:
            print("\n")

    for file_path in FILE_PATHS.values():
        os.remove(file_path)

    return 0


if __name__ == "__main__":
    sys.exit(main())
