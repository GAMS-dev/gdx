import gams.transfer as gt
import pandas as pd
import os


def create_default_values_example_1(file_path: str) -> None:
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

    m.write(file_path)


def create_default_values_example_2(file_path: str) -> None:
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
    v.records.drop(0, inplace=True)

    m.write(file_path)
