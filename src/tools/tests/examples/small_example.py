import gams.transfer as gt
import pandas as pd
import os


def create_small_example(file_path: str) -> None:
    m = gt.Container(system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"))

    # create the sets i, j
    i = gt.Set(m, "i", records=["seattle", "san-diego"], description="supply")
    j = gt.Set(m, "j", records=["new-york", "chicago", "topeka"], description="markets")

    # add 'd' parameter -- domain linked to set objects i and j
    d = gt.Parameter(m, "d", [i, j], description="distance in thousands of miles")

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
        columns=["from", "to", "thousand_miles"],
    )

    # setRecords will automatically convert the dist DataFrame into a standard DataFrame format
    d.setRecords(dist)

    # write the GDX
    m.write(file_path)
