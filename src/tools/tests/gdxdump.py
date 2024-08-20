import unittest
import os
import subprocess
import tempfile
from examples.small_example import create_small_example
from examples.full_example import create_full_example


def run_gdxdump(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [os.path.join('..', '..', '..', 'build', 'gdxdump'), *command],
        capture_output=True,
        text=True
    )


class TestGdxDump(unittest.TestCase):
    SMALL_EXAMPLE_FILE_PATH = os.path.join('.', 'examples', 'small_example.gdx')
    FULL_EXAMPLE_FILE_PATH = os.path.join('.', 'examples', 'full_example.gdx')
    OUTPUT_DIRECTORY_PATH = os.path.join('.', 'output', 'gdxdump')

    @classmethod
    def setUpClass(cls) -> None:
        create_small_example(cls.SMALL_EXAMPLE_FILE_PATH)
        create_full_example(cls.FULL_EXAMPLE_FILE_PATH)

    @classmethod
    def tearDownClass(cls) -> None:
        os.remove(cls.SMALL_EXAMPLE_FILE_PATH)
        os.remove(cls.FULL_EXAMPLE_FILE_PATH)

    def test_empty_command(self) -> None:
        output = run_gdxdump([])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'usage.txt'), 'r') as file:
            second = file.read().split('\n')
        del second[1]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_small_example(self) -> None:
        output = run_gdxdump([self.SMALL_EXAMPLE_FILE_PATH])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'small_example.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example(self) -> None:
        output = run_gdxdump([self.FULL_EXAMPLE_FILE_PATH])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_output(self) -> None:
        with tempfile.NamedTemporaryFile() as temporary_file:
            output = run_gdxdump([
                self.FULL_EXAMPLE_FILE_PATH,
                f'Output={temporary_file.name}'
            ])
            self.assertEqual(output.returncode, 0)
            with open(temporary_file.name, 'r') as file:
                first = file.read().split('\n')
            with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example.txt'), 'r') as file:
                second = file.read().split('\n')
            self.assertEqual(first, second)
            self.assertEqual(output.stdout, '')
            self.assertEqual(output.stderr, '')

    def test_full_example_version(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            '-Version'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')[1:]
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_version.txt'), 'r') as file:
            second = file.read().split('\n')[1:]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_short_version(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            '-V'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')[1:]
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_version.txt'), 'r') as file:
            second = file.read().split('\n')[1:]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symb=i'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_not_found(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symb=e'
        ])
        self.assertEqual(output.returncode, 6)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol_not_found.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')
