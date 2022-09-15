import os
import shutil
import sys

gams_sysdir = '/home/andre/gamsdist'
cwd = os.getcwd()
cwrap_dir = '../cmake-build-debug/'


def query_so_path():
    for fn in os.listdir('build/'):
        if 'lib.' in fn and os.path.isdir('build/' + fn):
            for fn2 in os.listdir('build/' + fn + '/'):
                if fn2.endswith('.so'):
                    return f'build/{fn}/{fn2}'
    return None


def cleanup():
    if os.path.isdir('build'):
        shutil.rmtree('build')
    # os.remove(cwrap_dir + 'libgdxcwrap.so')
    for fn in os.listdir('.'):
        if fn.endswith('.so'):
            os.remove(fn)


def prepare():
    if not os.path.exists('pygdx.so'):
        if not os.path.exists('build'):
            os.system('python setup.py build')
        shutil.copy(query_so_path(), 'pygdx.so')
    if not os.path.exists('libgdxcwrap.so'):
        shutil.copy(cwrap_dir + 'libgdxcwrap.so', 'libgdxcwrap.so')


def run():
    os.system(f'LD_LIBRARY_PATH={cwd}:{gams_sysdir} python test.py')


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'clean':
        cleanup()
    else:
        prepare()
        run()
