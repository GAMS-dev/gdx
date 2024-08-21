import unittest
import os
import subprocess
import gams.transfer as gt
from examples.small_example import create_small_example
from examples.full_example import create_full_example


def run_gdxdiff(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [os.path.join('..', '..', '..', 'build', 'gdxdiff'), *command],
        capture_output=True,
        text=True
    )


class TestGdxDiff(unittest.TestCase):
    EXAMPLES_DIRECTORY_PATH = os.path.join('.', 'examples')
    SMALL_EXAMPLE_FILE_PATH = os.path.join(EXAMPLES_DIRECTORY_PATH, 'small_example.gdx')
    FULL_EXAMPLE_FILE_PATH = os.path.join(EXAMPLES_DIRECTORY_PATH, 'full_example.gdx')
    DIFF_FILE_PATH = os.path.join(EXAMPLES_DIRECTORY_PATH, 'diffile.gdx')
    OUTPUT_DIRECTORY_PATH = os.path.join('.', 'output', 'gdxdiff')

    @classmethod
    def setUpClass(cls) -> None:
        create_small_example(cls.SMALL_EXAMPLE_FILE_PATH)
        create_full_example(cls.FULL_EXAMPLE_FILE_PATH)

    @classmethod
    def tearDownClass(cls) -> None:
        os.remove(cls.SMALL_EXAMPLE_FILE_PATH)
        os.remove(cls.FULL_EXAMPLE_FILE_PATH)
        os.remove(cls.DIFF_FILE_PATH)

    def test_empty_command(self) -> None:
        output = run_gdxdiff([])
        self.assertEqual(output.returncode, 2)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'usage.txt'), 'r') as file:
            second = file.read().split('\n')
        del second[2]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_small_and_full_example(self) -> None:
        output = run_gdxdiff([
            self.SMALL_EXAMPLE_FILE_PATH,
            self.FULL_EXAMPLE_FILE_PATH,
            self.DIFF_FILE_PATH
        ])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')[2:]
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'small_and_full_example.txt'), 'r') as file:
            second = file.read().split('\n')[3:]
        del first[-3]
        del second[-3]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_small_and_full_example_gdx_file(self) -> None:
        container = gt.Container(load_from=self.DIFF_FILE_PATH)
        self.assertIn('FilesCompared', container)

        symbol: gt.Parameter = container['FilesCompared']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['File1', self.SMALL_EXAMPLE_FILE_PATH],
            ['File2', self.FULL_EXAMPLE_FILE_PATH]
        ]
        self.assertEqual(first, second)
