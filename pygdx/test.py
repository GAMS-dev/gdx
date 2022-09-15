import pygdx
import os

rc = pygdx.create_gdx_file('test.gdx')
print(f'Return code = {rc}.')
if os.path.isfile('test.gdx'):
    print('File has been created. SUCCESS!')
    os.remove('test.gdx')
