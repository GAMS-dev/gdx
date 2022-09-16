import pygdx
import os

#print(obj.dumpfile())
#rc = pygdx.create_gdx_file('test.gdx')
#print(f'Return code = {rc}.')

obj = pygdx.GDXDataStorage()
print(obj)
print(f'open_write rc = {"ok" if obj.open_write("custom_name.gdx") else "fail"}')
print(f'close rc = {obj.close()}')
if os.path.isfile('custom_name.gdx'):
    print('File has been created. SUCCESS!')
    os.remove('custom_name.gdx')
