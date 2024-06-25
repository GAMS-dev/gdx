import platform
import os
import random
import string

import gams.transfer as gt
import pandas as pd


match platform.system():
    case 'Windows': gams_system_directory = 'C:\\GAMS\\46'
    case 'Darwin': gams_system_directory = '/Library/Frameworks/GAMS.framework/Resources'
    case other: raise Exception('Current operating system is not yet supported')

gdx_files_directory = os.path.join('.', 'output', 'gdx_files')


def get_random_ascii_letters(count: int) -> str:
    return ''.join([random.choice(string.ascii_letters) for _ in range(count)])
