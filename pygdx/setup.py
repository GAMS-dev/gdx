from distutils.core import setup, Extension

module1 = Extension('pygdx',
                    include_dirs=['..'],
                    libraries=['gdxcwrap'],
                    library_dirs=['../cmake-build-debug'],
                    sources=['pygdx.c'])
#                    extra_compile_args=['-fPIC'])

setup(name='PackageName',
      version='1.0',
      description='A tiny GDX library wrapper',
      ext_modules=[module1])
