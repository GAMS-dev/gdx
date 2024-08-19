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

    @classmethod
    def setUpClass(cls):
        create_small_example(os.path.join('examples', 'small_example.gdx'))
        create_full_example(os.path.join('examples', 'full_example.gdx'))

    @classmethod
    def tearDownClass(cls):
        os.remove(os.path.join('examples', 'small_example.gdx'))
        os.remove(os.path.join('examples', 'full_example.gdx'))
        os.remove(os.path.join('examples', 'diffile.gdx'))

    def test_small_and_full_example(self):
        output = run_gdxdiff([
            os.path.join('examples', 'small_example.gdx'),
            os.path.join('examples', 'full_example.gdx'),
            os.path.join('examples', 'diffile.gdx')
        ])
        self.assertEqual(output.returncode, 1)
        with open(os.path.join('output', 'gdxdiff', 'small_and_full_example.txt'), 'r') as file:
            first = output.stdout.split('\n')[2:]
            del first[-3]
            second = file.read().split('\n')[3:]
            del second[-3]
            self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_small_and_full_example_gdx_file(self):
        container = gt.Container(load_from=os.path.join('examples', 'diffile.gdx'))
        self.assertIn('FilesCompared', container)

        symbol: gt.Parameter = container['FilesCompared']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['File1', 'examples/small_example.gdx'],
            ['File2', 'examples/full_example.gdx']
        ]
        self.assertEqual(first, second)
