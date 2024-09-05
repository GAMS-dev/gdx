import platform
import subprocess
import os
import sys

from examples.small_example import create_small_example
from examples.full_example import create_full_example


TESTS_DIRECTORY_PATH = os.path.dirname(os.path.abspath(__file__))
DIRECTORY_PATHS = {
    'examples': os.path.join(TESTS_DIRECTORY_PATH, 'examples'),
    'results': os.path.join(TESTS_DIRECTORY_PATH, 'results')
}
FILE_PATHS = {
    'small_example': os.path.join(DIRECTORY_PATHS['examples'], 'small_example.gdx'),
    'full_example': os.path.join(DIRECTORY_PATHS['examples'], 'full_example.gdx'),
    'diff_file': os.path.join(DIRECTORY_PATHS['examples'], 'diff_file.gdx'),
    'merge_file': os.path.join(DIRECTORY_PATHS['examples'], 'merge_file.gdx')
}


def benchmark_executable(executable_name: str, command: list[str]) -> None:
    if platform.system() == 'Windows':
        EXECUTABLE_PATH = ['Release', f'{executable_name}.exe']
    else:
        EXECUTABLE_PATH = ['build', executable_name]
    os.makedirs(DIRECTORY_PATHS['results'], exist_ok=True)
    full_command = [
        'hyperfine',
        '--shell=none',
        '--warmup', '5',
        '--ignore-failure',
        '--export-markdown', os.path.join(
            DIRECTORY_PATHS['results'],
            f'{executable_name}.md'
        ),
        '--command-name', f'{executable_name} (C++)',
        ' '.join([
            os.path.join(TESTS_DIRECTORY_PATH, '..', '..', '..', *EXECUTABLE_PATH),
            *command
        ]),
        '--command-name', f'{executable_name} (Delphi)',
        ' '.join([
            executable_name,
            *command
        ])
    ]
    print(f'{' '.join(full_command)}\n')
    subprocess.run(full_command)


def main() -> int:
    create_small_example(FILE_PATHS['small_example'])
    create_full_example(FILE_PATHS['full_example'])

    executables: dict[str, list[str]] = {
        'gdxdump': [
            FILE_PATHS['full_example']
        ],
        'gdxdiff': [
            FILE_PATHS['small_example'],
            FILE_PATHS['full_example'],
            FILE_PATHS['diff_file']
        ],
        'gdxmerge': [
            FILE_PATHS['small_example'],
            FILE_PATHS['full_example'],
            f'Output={FILE_PATHS['merge_file']}'
        ]
    }
    for i, executable_name in enumerate(executables):
        benchmark_executable(executable_name, executables[executable_name])
        if i < len(executables) - 1:
            print('\n')

    for file_path in FILE_PATHS.values():
        os.remove(file_path)

    return 0


if __name__ == '__main__':
    sys.exit(main())
