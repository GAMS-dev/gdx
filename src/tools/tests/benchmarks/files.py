import os
import subprocess
import sys

from ..common import DIRECTORY_PATHS, TESTS_DIRECTORY_PATH, get_executable_path


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
        " ".join([os.path.join(DIRECTORY_PATHS.gdx, *executable_path), *command]),
        "--command-name",
        f"{executable_name} (Delphi)",
        " ".join([executable_name, *command]),
    ]
    print(f"{' '.join(full_command)}\n")
    os.makedirs(DIRECTORY_PATHS.results, exist_ok=True)
    subprocess.run(full_command)


def main() -> int:
    executables: dict[str, list[str]] = {
        "gdxdump": [f"{TESTS_DIRECTORY_PATH}/gdxfiles/large/20_front.gdx"],
        "gdxdiff": [
            f"{TESTS_DIRECTORY_PATH}/gdxfiles/30/gdx_20.gdx",
            f"{TESTS_DIRECTORY_PATH}/gdxfiles/30/gdx_27.gdx",
            "diff_file.gdx",
        ],
        "gdxmerge": [
            f"{TESTS_DIRECTORY_PATH}/gdxfiles/30/gdx_20.gdx",
            f"{TESTS_DIRECTORY_PATH}/gdxfiles/30/gdx_27.gdx",
            "Output=merge_file.gdx",
        ],
    }
    for i, executable_name in enumerate(executables):
        print(
            f"\033[4m\033[1mBENCHMARK {i + 1}\033[0m\033[4m ({executable_name}):\033[0m"
        )
        benchmark_executable(executable_name, executables[executable_name])
        if i < len(executables) - 1:
            print("\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
