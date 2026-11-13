import os
from pathlib import Path

from gams.transfer import Container, Set


def create_description_example_1(file_path: Path) -> None:
    m = Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    Set(m, "i", records=[("seattle", "text 1"), ("san-diego", "text 2")])

    m.write(file_path)


def create_description_example_2(file_path: Path) -> None:
    m = Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    Set(m, "i", records=[("seattle", "text 3"), ("san-diego", "text 4")])

    m.write(file_path)
