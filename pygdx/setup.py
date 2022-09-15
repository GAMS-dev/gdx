from distutils.core import setup, Extension

module1 = Extension('pygdx',
                    libraries=['gdxcwrap'],
                    library_dirs=['../cmake-build-debug'],
                    sources=['pygdx.c'])

setup(name='PackageName',
      version='1.0',
      description='A tiny GDX library wrapper',
      ext_modules=[module1])
