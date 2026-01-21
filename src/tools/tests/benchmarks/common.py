import os
import subprocess
from pathlib import Path

from ..common import DIRECTORY_PATHS, get_executable_path


def benchmark_executable(executable_name: str, command: list[str | Path]) -> None:
    executable_path = get_executable_path(executable_name)
    command_as_strings = [str(element) for element in command]

    full_command = [
        "hyperfine",
        "--shell=none",
        "--warmup",
        "5",
        "--ignore-failure",
        "--export-markdown",
        str(DIRECTORY_PATHS.results / f"{executable_name}.md"),
        "--command-name",
        f"{executable_name} (C++)",
        " ".join([str(executable_path), *command_as_strings]),
        "--command-name",
        f"{executable_name} (Delphi)",
        " ".join([executable_name, *command_as_strings]),
    ]
    print(f"{' '.join(full_command)}\n")

    os.makedirs(DIRECTORY_PATHS.results, exist_ok=True)
    subprocess.run(full_command)
