import os
import platform
import subprocess
from dataclasses import dataclass


@dataclass(frozen=True)
class OutputPaths:
    gdxdump: str
    gdxdiff: str
    gdxmerge: str


@dataclass(frozen=True)
class DirectoryPaths:
    gdx: str
    build: str
    examples: str
    output: OutputPaths
    results: str


RUNNING_ON_WINDOWS = platform.system() == "Windows"

TESTS_DIRECTORY_PATH = os.path.dirname(os.path.abspath(__file__))
GDX_DIRECTORY_PATH = os.path.realpath(
    os.path.join(TESTS_DIRECTORY_PATH, "..", "..", "..")
)

DIRECTORY_PATHS = DirectoryPaths(
    gdx=GDX_DIRECTORY_PATH,
    build=os.path.join(
        GDX_DIRECTORY_PATH, "Release" if RUNNING_ON_WINDOWS else "build"
    ),
    examples=os.path.join(TESTS_DIRECTORY_PATH, "examples"),
    output=OutputPaths(
        gdxdump=os.path.join(TESTS_DIRECTORY_PATH, "output", "gdxdump"),
        gdxdiff=os.path.join(TESTS_DIRECTORY_PATH, "output", "gdxdiff"),
        gdxmerge=os.path.join(TESTS_DIRECTORY_PATH, "output", "gdxmerge"),
    ),
    results=os.path.join(TESTS_DIRECTORY_PATH, "results"),
)


def get_executable_path(executable_name: str) -> list[str]:
    build_directory_exists = os.path.isdir(DIRECTORY_PATHS.build)

    if RUNNING_ON_WINDOWS:
        return (
            [DIRECTORY_PATHS.build, f"{executable_name}.exe"]
            if build_directory_exists
            else [DIRECTORY_PATHS.gdx, "gdxtools", f"{executable_name}.exe"]
        )
    else:
        os.environ[
            "LD_LIBRARY_PATH" if platform.system() == "Linux" else "DYLD_LIBRARY_PATH"
        ] = DIRECTORY_PATHS.build if build_directory_exists else DIRECTORY_PATHS.gdx

        return (
            [DIRECTORY_PATHS.build, "src", "tools", executable_name, executable_name]
            if build_directory_exists
            else [DIRECTORY_PATHS.gdx, "gdxtools", executable_name]
        )


def run_executable(
    executable_name: str,
    command: list[str],
) -> subprocess.CompletedProcess[str]:
    executable_path = get_executable_path(executable_name)
    return subprocess.run(
        [os.path.join(*executable_path), *command],
        capture_output=True,
        text=True,
    )
