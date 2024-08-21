import unittest
import os
import subprocess
import gams.transfer as gt
from examples.small_example import create_small_example
from examples.full_example import create_full_example
from examples.changed_small_example import create_changed_small_example


def run_gdxdiff(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [os.path.join('..', '..', '..', 'build', 'gdxdiff'), *command],
        capture_output=True,
        text=True
    )


class TestGdxDiff(unittest.TestCase):
    DIRECTORY_PATHS = {
        'examples': os.path.join('.', 'examples'),
        'output': os.path.join('.', 'output', 'gdxdiff')
    }
    FILE_PATHS = {
        'small_example': os.path.join(DIRECTORY_PATHS['examples'], 'small_example.gdx'),
        'full_example': os.path.join(DIRECTORY_PATHS['examples'], 'full_example.gdx'),
        'changed_small_example': os.path.join(DIRECTORY_PATHS['examples'], 'changed_small_example.gdx'),
        'diff_file': os.path.join(DIRECTORY_PATHS['examples'], 'diffile.gdx')
    }

    @classmethod
    def setUpClass(cls) -> None:
        create_small_example(cls.FILE_PATHS['small_example'])
        create_full_example(cls.FILE_PATHS['full_example'])
        create_changed_small_example(cls.FILE_PATHS['changed_small_example'])

    @classmethod
    def tearDownClass(cls) -> None:
        for file_path in cls.FILE_PATHS.values():
            os.remove(file_path)

    def test_empty_command(self) -> None:
        output = run_gdxdiff([])
        self.assertEqual(output.returncode, 2)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'usage.txt'), 'r') as file:
            second = file.read().split('\n')
        del second[2]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_small_and_full_example(self) -> None:
        output = run_gdxdiff([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['diff_file']
        ])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')[2:]
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'small_and_full_example.txt'), 'r') as file:
            second = file.read().split('\n')[3:]
        del first[-3]
        del second[-3]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_small_and_full_example_gdx_file(self) -> None:
        container = gt.Container(load_from=self.FILE_PATHS['diff_file'])
        self.assertIn('FilesCompared', container)

        symbol: gt.Parameter = container['FilesCompared']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['File1', self.FILE_PATHS['small_example']],
            ['File2', self.FILE_PATHS['full_example']]
        ]
        self.assertEqual(first, second)
