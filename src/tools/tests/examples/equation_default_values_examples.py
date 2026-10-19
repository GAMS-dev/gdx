import os
from pathlib import Path

from gams.transfer import Container, Equation
from pandas import DataFrame


def create_equation_default_values_example_1(file_path: Path) -> None:
    m = Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    Equation(
        m,
        "e",
        "nonbinding",
        domain=["*"],
        records=DataFrame(
            data=[("i1", 1.0)],
            columns=["domain", "level"],
        ),
    )

    m.write(file_path)


def create_equation_default_values_example_2(file_path: Path) -> None:
    m = Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    Equation(
        m,
        "e",
        "nonbinding",
        domain=["*"],
        records=DataFrame(
            data=[("i1", 1.0), ("i2", 0.0)],
            columns=["domain", "level"],
        ),
    )

    m.write(file_path)
