import os
from pathlib import Path

from gams.transfer import Container, Variable
from pandas import DataFrame


def create_default_values_example_1(file_path: Path) -> None:
    m = Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    Variable(
        m,
        "v",
        "free",
        domain=["*"],
        records=DataFrame(
            data=[("i" + str(i), i) for i in range(5)], columns=["domain", "marginal"]
        ),
    )

    m.write(file_path)


def create_default_values_example_2(file_path: Path) -> None:
    m = Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    v = Variable(
        m,
        "v",
        "free",
        domain=["*"],
        records=DataFrame(
            data=[("i" + str(i), i) for i in range(5)], columns=["domain", "marginal"]
        ),
    )
    v.records.drop(0, inplace=True)

    m.write(file_path)
