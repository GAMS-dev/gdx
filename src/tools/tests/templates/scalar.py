import gams.transfer as gt
from common import gt, os, gams_system_directory, gdx_files_directory

m = gt.Container(system_directory=gams_system_directory)

# create a scalar
pi = gt.Parameter(m, "pi", records=3.14159, description='  (pi scalar)')

# write the GDX
m.write(os.path.join(gdx_files_directory, 'scalar.gdx'))
