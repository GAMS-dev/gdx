import pygdx
import os

#print(obj.dumpfile())
#rc = pygdx.create_gdx_file('test.gdx')
#print(f'Return code = {rc}.')

obj = pygdx.GDXDataStorage()
print(obj)
print(f'open_write rc = {"ok" if obj.open_write("custom_name.gdx") else "fail"}')
print(f'set1D rc = {obj.set1D("i", ["new-york", "chicago", "topeka"])}')
print(f'close rc = {obj.close()}')
if os.path.isfile('custom_name.gdx'):
    print('File has been created. SUCCESS!')
    os.system("gdxdump custom_name.gdx")
    os.remove('custom_name.gdx')
try:
    obj.open_read('does_not_exist.gdx')
except Exception as e:
    print('Good. Trying to open non-existant file throws exception: ' + str(e))