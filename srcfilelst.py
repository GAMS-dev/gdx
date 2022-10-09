# Script to automatically collect list of source files for devel/products Makefile

import os

type_ext = [
    ('mycfiles', '.c'),
    ('mycxxfiles', '.cpp'),
    ('myhfiles', '.h'),
    ('myhxxfiles', '.hpp')
]

exts = [ext for name, ext in type_ext]

exclusion_substrs = ['test', 'inc', 'cmake']


def no_extension(fn):
    return fn if '.' not in fn else fn[0:fn.rfind('.')]


def recursive_collect(root='.'):
    file_lists = {name: [] for name, ext in type_ext}

    def collect(path):
        for fn in os.listdir(path):
            ffn = path + os.sep + fn
            if any(ess in fn for ess in exclusion_substrs):
                continue
            if os.path.isfile(ffn) and ('.' + fn.split('.')[-1]) in exts:
                for name, ext in type_ext:
                    if fn.endswith(ext):
                        pflen = len(root + os.sep)
                        ffn_stem = no_extension(ffn[pflen:])
                        file_lists[name].append(ffn_stem)
                        break
            elif os.path.isdir(ffn):
                collect(ffn)

    collect(root)
    return file_lists


def add_prefix(dct, prefix):
    return {k: [prefix + elem for elem in v] for k, v in dct.items()}


def main():
    for name, lst in add_prefix(recursive_collect(), 'gdxnative' + os.sep).items():
        print(f'{name.ljust(11)}= {" ".join(lst)}')


if __name__ == '__main__':
    main()
