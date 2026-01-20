import os

import gams.transfer as gt  # pyright: ignore[reportMissingTypeStubs]
import pandas as pd


def get_test_string(count: int = 255) -> str:
    return f"|{'_' * (count - 2)}|"


def create_label_example(file_path: str) -> None:
    m = gt.Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    # create the set i
    i = gt.Set(
        m,
        "i",
        records=[("seattle", get_test_string()), ("san-diego", get_test_string())],
        description=get_test_string(),
    )

    # create the set j
    j = gt.Set(
        m,
        "j",
        records=[
            ("new-york", get_test_string()),
            ("chicago", get_test_string()),
            ("topeka", get_test_string()),
        ],
        description=get_test_string(),
    )

    # add 'd' parameter -- domain linked to set objects i and j
    d = gt.Parameter(m, "d", [i, j], description=get_test_string())

    # create some data as a generic DataFrame
    dist = pd.DataFrame(
        [
            ("seattle", "new-york", 2.5),
            ("seattle", "chicago", 1.7),
            ("seattle", "topeka", 1.8),
            ("san-diego", "new-york", 2.5),
            ("san-diego", "chicago", 1.8),
            ("san-diego", "topeka", 1.4),
        ],
        # Probably unnecessary here:
        columns=[get_test_string(), get_test_string(), get_test_string()],
    )

    # setRecords will automatically convert the dist DataFrame into a standard DataFrame format
    d.setRecords(dist)

    # write the GDX
    m.write(file_path)  # pyright: ignore[reportUnknownMemberType]
