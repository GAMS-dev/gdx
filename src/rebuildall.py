# In GDX directory:
# 1. "python -m venv venv"
# 2. "source venv/bin/activate"
# 3. "pip install -r requirements.txt"

import os
import subprocess

import yaml2doxy
import yaml2cwrap

# Rebuild main header file (gdx.h)
yaml2doxy.generate_method_declarations(
    'gdxapi.yaml',
    'templates',
    'gdxheader.template.j2',
    'gdx.h'
)

# Rebuild C wrapper around GDX object (cwrap.hpp)
yaml2cwrap.generate_c_wrapper(
    'gdxapi.yaml',
    'templates',
    'cwrap.template.j2',
    '../generated/cwrap.hpp'
)

# Rebuild OOP C++ wrapper around C API (gdxcppwrap.h)
yaml2cwrap.generate_c_wrapper(
    'gdxapi.yaml',
    'templates',
    'gdxcppwrap.template.j2',
    '../generated/gdxcppwrap.h'
)

# Rebuild gdxcc.{h,c} and gdxcclib.cpp
cmd = ('python src/apigenerator/src/mkapi.py' +
       f' --apidef {os.getcwd()}/gdxapi.yaml --outputpath generated/ --output cc cpplib')
subprocess.run(cmd.split(' '), shell=False, cwd='../')