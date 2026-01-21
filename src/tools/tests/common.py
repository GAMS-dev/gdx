import inspect
import os
import platform
import subprocess
import unittest
from dataclasses import dataclass
from pathlib import Path
from typing import Literal

import gams.transfer as gt  # type: ignore


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

type ExecutableName = Literal["gdxdump", "gdxdiff", "gdxmerge"]


def get_executable_path(executable_name: ExecutableName) -> Path:
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
    executable_name: ExecutableName,
    command: list[str | Path],
) -> subprocess.CompletedProcess[str]:
    executable_path = get_executable_path(executable_name)
    command_as_strings: list[str] = [
        str(value) if isinstance(value, Path) else value for value in command
    ]
    return subprocess.run(
        [str(executable_path), *command_as_strings],
        capture_output=True,
        text=True,
    )


def check_output(
    test_instance: unittest.TestCase,
    executable_name: ExecutableName,
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


def check_gdx_file_symbols(
    test_instance: unittest.TestCase,
    executable_name: ExecutableName,
    container: gt.Container,
    symbol_names: list[str],
) -> None:
    for symbol_name in symbol_names:
        with test_instance.subTest(symbol_name=symbol_name):
            test_instance.assertIn(symbol_name, container)

    if executable_name == "gdxmerge":
        test_instance.assertEqual(len(container), len(symbol_names) + 1)
    else:
        test_instance.assertEqual(len(container), len(symbol_names))


def check_gdx_file_values(
    test_instance: unittest.TestCase,
    container: gt.Container,
    symbol_name: str,
    expected_values: list[list[str | Path | float]],
) -> None:
    path_values_as_strings: list[list[str | float]] = [
        [str(value) if isinstance(value, Path) else value for value in values]
        for values in expected_values
    ]
    test_instance.assertIn(symbol_name, container)
    symbol: gt.Parameter = container[symbol_name]  # type: ignore
    values = symbol.records.values.tolist()  # type: ignore
    test_instance.assertEqual(values, path_values_as_strings)


type GamsSymbols = dict[str, list[list[str | Path | float]]]


def check_gdx_file(
    test_instance: unittest.TestCase,
    executable_name: ExecutableName,
    file_path: Path,
    symbols: GamsSymbols,
) -> gt.Container:
    container = gt.Container(
        system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"),
        load_from=file_path,
    )

    check_gdx_file_symbols(
        test_instance,
        executable_name,
        container,
        list(symbols.keys()),
    )

    for symbol_name in symbols:
        check_gdx_file_values(
            test_instance,
            container,
            symbol_name,
            symbols[symbol_name],
        )

    return container
