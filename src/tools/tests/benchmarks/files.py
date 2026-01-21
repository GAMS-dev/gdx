import sys
from pathlib import Path

from ..common import TESTS_DIRECTORY_PATH
from .common import benchmark_executable


def main() -> int:
    commands: dict[str, list[str | Path]] = {
        "gdxdump": [
            TESTS_DIRECTORY_PATH / "gdxfiles" / "large" / "20_front.gdx",
        ],
        "gdxdiff": [
            TESTS_DIRECTORY_PATH / "gdxfiles" / "30" / "gdx_20.gdx",
            TESTS_DIRECTORY_PATH / "gdxfiles" / "30" / "gdx_27.gdx",
            "diff_file.gdx",
        ],
        "gdxmerge": [
            TESTS_DIRECTORY_PATH / "gdxfiles" / "30" / "gdx_20.gdx",
            TESTS_DIRECTORY_PATH / "gdxfiles" / "30" / "gdx_27.gdx",
            "Output=merge_file.gdx",
        ],
    }
    for i, executable_name in enumerate(commands):
        print(
            f"\033[4m\033[1mBENCHMARK {i + 1}\033[0m\033[4m ({executable_name}):\033[0m"
        )
        benchmark_executable(executable_name, commands[executable_name])
        if i < len(commands) - 1:
            print("\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
