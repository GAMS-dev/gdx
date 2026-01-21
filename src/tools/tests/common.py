import os
import platform
import subprocess
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class OutputPaths:
    gdxdump: Path
    gdxdiff: Path
    gdxmerge: Path


@dataclass(frozen=True)
class DirectoryPaths:
    gdx: Path
    build: Path
    examples: Path
    output: OutputPaths
    results: Path


RUNNING_ON_WINDOWS = platform.system() == "Windows"

TESTS_DIRECTORY_PATH = Path(__file__).resolve().parent
GDX_DIRECTORY_PATH = TESTS_DIRECTORY_PATH.parents[2]

DIRECTORY_PATHS = DirectoryPaths(
    gdx=GDX_DIRECTORY_PATH,
    build=GDX_DIRECTORY_PATH / ("Release" if RUNNING_ON_WINDOWS else "build"),
    examples=TESTS_DIRECTORY_PATH / "examples",
    output=OutputPaths(
        gdxdump=TESTS_DIRECTORY_PATH / "output" / "gdxdump",
        gdxdiff=TESTS_DIRECTORY_PATH / "output" / "gdxdiff",
        gdxmerge=TESTS_DIRECTORY_PATH / "output" / "gdxmerge",
    ),
    results=TESTS_DIRECTORY_PATH / "results",
)


def get_executable_path(executable_name: str) -> Path:
    build_directory_exists = DIRECTORY_PATHS.build.is_dir()

    if RUNNING_ON_WINDOWS:
        executable_name_with_suffix = f"{executable_name}.exe"
        if build_directory_exists:
            return DIRECTORY_PATHS.build / executable_name_with_suffix
        else:
            return DIRECTORY_PATHS.gdx / "gdxtools" / executable_name_with_suffix
    else:
        environment_variable = (
            "LD_LIBRARY_PATH" if platform.system() == "Linux" else "DYLD_LIBRARY_PATH"
        )
        os.environ[environment_variable] = str(
            DIRECTORY_PATHS.build if build_directory_exists else DIRECTORY_PATHS.gdx
        )

        if build_directory_exists:
            return (
                DIRECTORY_PATHS.build
                / "src"
                / "tools"
                / executable_name
                / executable_name
            )
        else:
            return DIRECTORY_PATHS.gdx / "gdxtools" / executable_name


def run_executable(
    executable_name: str,
    command: list[str],
) -> subprocess.CompletedProcess[str]:
    executable_path = get_executable_path(executable_name)
    return subprocess.run(
        [str(executable_path), *command],
        capture_output=True,
        text=True,
    )
