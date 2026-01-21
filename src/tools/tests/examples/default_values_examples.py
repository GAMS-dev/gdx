import os
from pathlib import Path

import gams.transfer as gt  # pyright: ignore[reportMissingTypeStubs]
import pandas as pd


def create_default_values_example_1(file_path: Path) -> None:
    m = gt.Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    gt.Variable(
        m,
        "v",
        "free",
        domain=["*"],
        records=pd.DataFrame(
            data=[("i" + str(i), i) for i in range(5)], columns=["domain", "marginal"]
        ),
    )

    m.write(file_path)  # pyright: ignore[reportUnknownMemberType]


def create_default_values_example_2(file_path: Path) -> None:
    m = gt.Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    v = gt.Variable(
        m,
        "v",
        "free",
        domain=["*"],
        records=pd.DataFrame(
            data=[("i" + str(i), i) for i in range(5)], columns=["domain", "marginal"]
        ),
    )
    v.records.drop(0, inplace=True)  # pyright: ignore[reportUnknownMemberType]

    m.write(file_path)  # pyright: ignore[reportUnknownMemberType]
