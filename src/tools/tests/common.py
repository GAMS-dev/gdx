import inspect
import os
import platform
import subprocess
import tempfile
import unittest
from collections.abc import Generator
from contextlib import contextmanager
from dataclasses import dataclass
from pathlib import Path
from typing import Literal

from gams.transfer import Container, Parameter


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

LIBRARY_PATH_VARIABLE = (
    "LD_LIBRARY_PATH" if platform.system() == "Linux" else "DYLD_LIBRARY_PATH"
)

# The tools pass a buffer of GMS_SSSIZE bytes to the GDX library for its error messages.
GMS_SSSIZE = 256

WINDOWS_LIBRARY_SEARCH_REASON = (
    "Windows also searches the executable's directory and PATH for the GDX library"
)

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

type ShadowKind = Literal["directory", "file"]
SHADOW_KINDS: tuple[ShadowKind, ...] = ("directory", "file")


@contextmanager
def shadowing_path(gdx_file_path: Path, kind: ShadowKind) -> Generator[Path]:
    """Create a directory or an empty file named like the GDX file without extension.

    Yields the path without extension, which the tools must resolve to the GDX file.
    """
    path = gdx_file_path.with_suffix("")
    if kind == "directory":
        path.mkdir()
    else:
        path.touch()
    try:
        yield path
    finally:
        if kind == "directory":
            path.rmdir()
        else:
            path.unlink()


def get_executable_path(executable_name: ExecutableName) -> Path:
    build_directory_exists = DIRECTORY_PATHS.build.is_dir()

    if RUNNING_ON_WINDOWS:
        executable_name_with_suffix = f"{executable_name}.exe"
        if build_directory_exists:
            return DIRECTORY_PATHS.build / executable_name_with_suffix
        else:
            return DIRECTORY_PATHS.gdx / "gdxtools" / executable_name_with_suffix
    else:
        os.environ[LIBRARY_PATH_VARIABLE] = str(
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
        check=False,
    )


def run_executable_without_gdx_library(
    executable_name: ExecutableName,
    command: list[str | Path],
) -> subprocess.CompletedProcess[str]:
    """Run a tool with an empty library search path, so loading the GDX library fails.

    Not usable on Windows, where the loader also searches the executable's directory and PATH.
    """
    executable_path = get_executable_path(executable_name)
    command_as_strings: list[str] = [
        str(value) if isinstance(value, Path) else value for value in command
    ]

    with tempfile.TemporaryDirectory() as empty_directory:
        environment = dict(os.environ)
        environment[LIBRARY_PATH_VARIABLE] = empty_directory
        return subprocess.run(
            [str(executable_path), *command_as_strings],
            capture_output=True,
            text=True,
            check=False,
            env=environment,
            cwd=empty_directory,
        )


def check_gdx_library_load_error(
    test_instance: unittest.TestCase,
    output: subprocess.CompletedProcess[str],
    return_code: int,
    message_prefix: str,
) -> None:
    """Check that a tool reported the loader message for a GDX library it could not load."""
    test_instance.assertEqual(output.returncode, return_code)
    test_instance.assertEqual(output.stderr, "")

    message_start = output.stdout.find(message_prefix)
    test_instance.assertNotEqual(message_start, -1, output.stdout)
    message = output.stdout[message_start + len(message_prefix) :].split("\n", 1)[0]
    test_instance.assertTrue(
        message.startswith("Could not load shared library "), message
    )
    test_instance.assertLessEqual(len(message), GMS_SSSIZE - 1)


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
    container: Container,
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
    container: Container,
    symbol_name: str,
    expected_values: list[list[str | Path | float]],
) -> None:
    test_instance.assertIn(symbol_name, container)

    symbol: Parameter = container[symbol_name]
    values = symbol.records.values.tolist()

    path_values_as_strings: list[list[str | float]] = [
        [str(value) if isinstance(value, Path) else value for value in values]
        for values in expected_values
    ]

    test_instance.assertEqual(values, path_values_as_strings)


type GamsSymbols = dict[str, list[list[str | Path | float]]]


def check_gdx_file(
    test_instance: unittest.TestCase,
    executable_name: ExecutableName,
    file_path: Path,
    symbols: GamsSymbols,
) -> Container:
    container = Container(
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
