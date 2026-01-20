import os
import platform
from dataclasses import dataclass


@dataclass(frozen=True)
class OutputPaths:
    gdxdump: str
    gdxdiff: str
    gdxmerge: str


@dataclass(frozen=True)
class DirectoryPaths:
    gdx: str
    examples: str
    output: OutputPaths
    results: str


TESTS_DIRECTORY_PATH = os.path.dirname(os.path.abspath(__file__))
DIRECTORY_PATHS = DirectoryPaths(
    gdx=os.path.realpath(os.path.join(TESTS_DIRECTORY_PATH, "..", "..", "..")),
    examples=os.path.join(TESTS_DIRECTORY_PATH, "examples"),
    output=OutputPaths(
        gdxdump=os.path.join(TESTS_DIRECTORY_PATH, "output", "gdxdump"),
        gdxdiff=os.path.join(TESTS_DIRECTORY_PATH, "output", "gdxdiff"),
        gdxmerge=os.path.join(TESTS_DIRECTORY_PATH, "output", "gdxmerge"),
    ),
    results=os.path.join(TESTS_DIRECTORY_PATH, "results"),
)


def get_executable_path(executable_name: str) -> list[str]:
    if platform.system() == "Windows":
        return (
            ["Release", f"{executable_name}.exe"]
            if os.path.isdir("Release")
            else ["gdxtools", f"{executable_name}.exe"]
        )
    else:
        build_directory_exists = os.path.isdir("build")
        os.environ[
            "LD_LIBRARY_PATH" if platform.system() == "Linux" else "DYLD_LIBRARY_PATH"
        ] = (
            os.path.join(DIRECTORY_PATHS.gdx, "build")
            if build_directory_exists
            else DIRECTORY_PATHS.gdx
        )
        return (
            ["build", "src", "tools", executable_name, executable_name]
            if build_directory_exists
            else ["gdxtools", executable_name]
        )
