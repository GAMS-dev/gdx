import os
from pathlib import Path

import gams.transfer as gt  # type: ignore
import pandas as pd


def create_equation_default_values_example_1(file_path: Path) -> None:
    m = gt.Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    gt.Equation(
        m,
        "e",
        "nonbinding",
        domain=["*"],
        records=pd.DataFrame(
            data=[("i1", 1.0)],
            columns=["domain", "level"],
        ),
    )

    m.write(file_path)  # type: ignore


def create_equation_default_values_example_2(file_path: Path) -> None:
    m = gt.Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    gt.Equation(
        m,
        "e",
        "nonbinding",
        domain=["*"],
        records=pd.DataFrame(
            data=[("i1", 1.0), ("i2", 0.0)],
            columns=["domain", "level"],
        ),
    )

    m.write(file_path)  # type: ignore
