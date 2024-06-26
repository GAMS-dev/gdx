from enum import Enum
import platform
import os


cli_options: dict[str, bool | str] = {}


class Platform(Enum):
    WINDOWS = 1
    MACOS = 2
    LINUX = 3


executable_name = 'gdxdump'
executable_paths: dict[str, str] = {}

match platform.system():
    case 'Windows':
        current_platform = Platform.WINDOWS
        gams_system_directory = 'C:\\GAMS\\46'
        executable_filename = f'{executable_name}.exe'
        executable_paths['test'] = os.path.join('..\\..\\..\\..\\build\\Debug', executable_filename)
        executable_paths['powershell'] = 'C:\\Program Files\\PowerShell\\7\\pwsh.exe'
    case 'Darwin':
        current_platform = Platform.MACOS
        gams_system_directory = '/Library/Frameworks/GAMS.framework/Resources'
        executable_filename = executable_name
        executable_paths['test'] = os.path.join('../../../../build', executable_filename)
    case other:
        current_platform = Platform.LINUX
        raise Exception('Current operating system is not yet supported')

executable_paths['gams'] = os.path.join(gams_system_directory, executable_filename)

# The real executable path is replaced with this so that the outputs do not differ
executable_path_current_directory = os.path.join('.', executable_name)

directory_paths = {
    'templates': os.path.join('.', 'templates'),
    'output': os.path.join('.', 'output'),
}


class Output(Enum):
    STATIC = 1
    TEST = 2


output_directory_paths = {
    'gdx_files': os.path.join(directory_paths['output'], 'gdx_files'),
    'static': os.path.join(directory_paths['output'], 'static'),
    'test': os.path.join(directory_paths['output'], 'test'),
}
