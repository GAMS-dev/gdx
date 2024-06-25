from common import gt, pd, os, gams_system_directory, gdx_files_directory

m = gt.Container(system_directory=gams_system_directory)

# create the sets i, j
i = gt.Set(m, 'i', records=['seattle', 'san-diego'], description='supply')
j = gt.Set(m, 'j', records=['new-york', 'chicago', 'topeka'], description='markets')

# add 'd' parameter -- domain linked to set objects i and j
d = gt.Parameter(m, 'd', [i, j], description='distance in thousands of miles')

# create some data as a generic DataFrame
dist = pd.DataFrame(
    [
        ('seattle', 'new-york', 2.5),
        ('seattle', 'chicago', 1.7),
        ('seattle', 'topeka', 1.8),
        ('san-diego', 'new-york', 2.5),
        ('san-diego', 'chicago', 1.8),
        ('san-diego', 'topeka', 1.4),
    ],
    columns=['from', 'to', 'thousand_miles'],
)

# setRecords will automatically convert the dist DataFrame into a standard DataFrame format
d.setRecords(dist)

# write the GDX
m.write(os.path.join(gdx_files_directory, 'small_example.gdx'))
