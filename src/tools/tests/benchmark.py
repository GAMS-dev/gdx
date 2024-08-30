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
        executable_path = ['Release', f'{executable_name}.exe']
    else:
        executable_path = ['build', executable_name]
    subprocess.run(
        [
            'hyperfine', '-i',
            '--warmup', '5',
            f'\'{' '.join([
                os.path.join(TESTS_DIRECTORY_PATH, '..', '..', '..', *executable_path),
                *command
            ])}\'',
            f'\'{' '.join([
                executable_name,
                *command
            ])}\''
        ],
        text=True
    )


def main() -> int:
    benchmark_executable('gdxdiff', [
        FILE_PATHS['small_example'],
        FILE_PATHS['full_example']
    ])
    return 0


if __name__ == '__main__':
    sys.exit(main())
