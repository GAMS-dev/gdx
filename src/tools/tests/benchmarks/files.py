import platform
import subprocess
import os
import sys


TESTS_DIRECTORY_PATH = os.path.dirname(os.path.abspath(__file__))
GDX_DIRECTORY_PATH = os.path.join(TESTS_DIRECTORY_PATH, "..", "..", "..", "..")


def benchmark_executable(executable_name: str, command: list[str]) -> None:
    if platform.system() == "Windows":
        EXECUTABLE_PATH = ["Release", f"{executable_name}.exe"]
    else:
        build_directory_exists = os.path.isdir("build")
        os.environ[
            "LD_LIBRARY_PATH" if platform.system() == "Linux" else "DYLD_LIBRARY_PATH"
        ] = (
            os.path.join(GDX_DIRECTORY_PATH, "build")
            if build_directory_exists
            else GDX_DIRECTORY_PATH
        )
        EXECUTABLE_PATH = (
            ["build", "src", "tools", executable_name, executable_name]
            if build_directory_exists
            else ["gdxtools", executable_name]
        )
    full_command = [
        "hyperfine",
        "--shell=none",
        "--warmup",
        "3",
        "--ignore-failure",
        "--export-markdown",
        os.path.join(TESTS_DIRECTORY_PATH, "results", f"{executable_name}.md"),
        "--command-name",
        f"{executable_name} (C++)",
        " ".join([os.path.join(GDX_DIRECTORY_PATH, *EXECUTABLE_PATH), *command]),
        "--command-name",
        f"{executable_name} (Delphi)",
        " ".join([executable_name, *command]),
    ]
    print(f"{' '.join(full_command)}\n")
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
