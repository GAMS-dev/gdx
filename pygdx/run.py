import os
import shutil
import sys

gams_sysdir = '/home/andre/gamsdist'
cwd = os.getcwd()
build_dir = '../cmake-build-debug/'
build_with_cmake = True
py_cmd = 'python' if os.path.exists('/usr/bin/python') else 'python3'


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
    if os.path.isfile('stubwarnings.txt'):
        os.remove('stubwarnings.txt')


def prepare():
    if build_with_cmake:
        if not os.path.isfile('pygdx.so'):
            src_path = f'{build_dir}libpygdx.so'
            if not os.path.isfile(src_path):
                os.system('cmake --build .. --target pygdx')
            shutil.copy(src_path, 'pygdx.so')
        return

    # Use distutils.core setup instead of CMake
    if not os.path.exists('pygdx.so'):
        if not os.path.exists('build'):
            os.system(f'{py_cmd} setup.py build')
        shutil.copy(query_so_path(), 'pygdx.so')
    if not os.path.exists('libgdxcwrap.so'):
        shutil.copy(build_dir + 'libgdxcwrap.so', 'libgdxcwrap.so')


def run():
    os.system(f'LD_LIBRARY_PATH={cwd}:{gams_sysdir} {py_cmd} test.py')


if __name__ == '__main__':
    args = sys.argv[1:]
    if any('clean' in arg for arg in args):
        cleanup()
    if any('run' in arg for arg in args):
        prepare()
        run()
