import gams.transfer as gt
import pandas as pd


def create_special_values_example(file_path: str) -> None:
    m = gt.Container()

    # create the sets i, j
    i = gt.Set(m, 'i', records=[('seattle', 'text 1'), ('san-diego', 'text 2')], description='supply')
    j = gt.Set(m, 'j', records=[('new-york', 'text 3'), ('chicago', 'text 4'), ('topeka', 'text 5')], description='markets')

    # add 'd' parameter -- domain linked to set objects i and j
    d = gt.Parameter(m, 'd', [i, j], description='distance in thousands of miles')

    # create some data as a generic DataFrame
    dist = pd.DataFrame(
        [
            ('seattle', 'new-york', gt.SpecialValues.EPS),
            ('seattle', 'chicago', gt.SpecialValues.NA),
            ('seattle', 'topeka', gt.SpecialValues.POSINF),
            ('san-diego', 'new-york', gt.SpecialValues.NEGINF),
            ('san-diego', 'chicago', gt.SpecialValues.UNDEF),
            ('san-diego', 'topeka', 0),
        ],
        columns=['from', 'to', 'thousand_miles'],
    )

    # setRecords will automatically convert the dist DataFrame into a standard DataFrame format
    d.setRecords(dist)

    # write the GDX
    m.write(file_path)
