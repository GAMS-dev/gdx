import os

import gams.transfer as gt  # pyright: ignore[reportMissingTypeStubs]


def create_domain_example_1(file_path: str) -> None:
    m = gt.Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    i = gt.Set(m, "i", records=["i1", "i2"])
    gt.Variable(m, "a", domain=[i])

    m.write(file_path)  # pyright: ignore[reportUnknownMemberType]


def create_domain_example_2(file_path: str) -> None:
    m = gt.Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    j = gt.Set(m, "j", records=["i1", "i2"])
    gt.Variable(m, "a", domain=[j])

    m.write(file_path)  # pyright: ignore[reportUnknownMemberType]
