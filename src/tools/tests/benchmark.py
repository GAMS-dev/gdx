import platform
import subprocess
import os
import sys


TESTS_DIRECTORY_PATH = os.path.dirname(os.path.abspath(__file__))
EXAMPLES_DIRECTORY_PATH = os.path.join(TESTS_DIRECTORY_PATH, 'examples')
FILE_PATHS = {
    'small_example': os.path.join(EXAMPLES_DIRECTORY_PATH, 'small_example.gdx'),
    'full_example': os.path.join(EXAMPLES_DIRECTORY_PATH, 'full_example.gdx')
}


def benchmark_executable(executable_name: str, command: list[str]) -> None:
    if platform.system() == 'Windows':
        EXECUTABLE_PATH = ['Release', f'{executable_name}.exe']
    else:
        EXECUTABLE_PATH = ['build', executable_name]
    full_command = [
        'hyperfine', '-i',
        '--warmup', '3',
        f'\'{' '.join([
            os.path.join(TESTS_DIRECTORY_PATH, '..', '..', '..', *EXECUTABLE_PATH),
            *command
        ])}\'',
        f'\'{' '.join([
            executable_name,
            *command
        ])}\''
    ]
    print(f'{' '.join(full_command)}\n')
    subprocess.run(full_command)


def main() -> int:
    EXECUTABLE_NAMES = ['gdxdump', 'gdxdiff', 'gdxmerge']
    for i, executable_name in enumerate(EXECUTABLE_NAMES):
        benchmark_executable(executable_name, [
            FILE_PATHS['small_example'],
            FILE_PATHS['full_example']
        ])
        if i < len(EXECUTABLE_NAMES) - 1:
            print('\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
