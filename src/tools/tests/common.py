import inspect
import os
import platform
import subprocess
import unittest
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
    command: list[str | Path],
) -> subprocess.CompletedProcess[str]:
    executable_path = get_executable_path(executable_name)
    command_as_strings = [str(element) for element in command]
    return subprocess.run(
        [str(executable_path), *command_as_strings],
        capture_output=True,
        text=True,
    )


def check_output(
    test_instance: unittest.TestCase,
    executable_name: str,
    output: subprocess.CompletedProcess[str],
    return_code: int,
    file_name: str | None,
    first_offset: int | None,
    first_negative_offset: int | None,
    second_offset: int | None,
    second_negative_offset: int | None,
    first_delete: list[int],
    second_delete: list[int],
) -> None:
    test_instance.assertEqual(output.returncode, return_code)
    first = output.stdout.split("\n")[first_offset:first_negative_offset]
    for i in first_delete:
        del first[i]
    if file_name is None:
        file_name = f"{inspect.stack()[2].function.removeprefix('test_')}.txt"
    with open(
        getattr(DIRECTORY_PATHS.output, executable_name) / file_name, "r"
    ) as file:
        second = file.read().split("\n")[second_offset:second_negative_offset]
    for i in second_delete:
        del second[i]
    test_instance.assertEqual(first, second)
    test_instance.assertEqual(output.stderr, "")
