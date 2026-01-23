import os
from pathlib import Path

from gams.transfer import Container, Set, Variable  # type: ignore


def create_domain_example_1(file_path: Path) -> None:
    m = Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    i = Set(m, "i", records=["i1", "i2"])
    Variable(m, "a", domain=[i])

    m.write(file_path)  # pyright: ignore


def create_domain_example_2(file_path: Path) -> None:
    m = Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    j = Set(m, "j", records=["i1", "i2"])
    Variable(m, "a", domain=[j])

    m.write(file_path)  # pyright: ignore
