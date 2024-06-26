from common import gt, pd, os, gams_system_directory, gdx_files_directory

m = gt.Container(system_directory=gams_system_directory)

i = gt.Set(m, 'i', records=['seattle', 'san-diego'], description='supply')
j = gt.Set(m, 'j', records=['new-york', 'chicago', 'topeka'], description='markets')

d = gt.Parameter(m, 'd', [i, j], description='distance in thousands of miles')

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

d.setRecords(dist)

m.write(os.path.join(gdx_files_directory, 'sv_example.gdx'))
