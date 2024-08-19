import unittest
import os
import subprocess
from examples.small_example import create_small_example
from examples.full_example import create_full_example


def run_gdxmerge(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [os.path.join('..', '..', '..', 'build', 'gdxmerge'), *command],
        capture_output=True,
        text=True
    )


class TestGdxMerge(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        create_small_example(os.path.join('examples', 'small_example.gdx'))
        create_full_example(os.path.join('examples', 'full_example.gdx'))

    @classmethod
    def tearDownClass(cls):
        os.remove(os.path.join('examples', 'small_example.gdx'))
        os.remove(os.path.join('examples', 'full_example.gdx'))
        os.remove(os.path.join('examples', 'merged.gdx'))

    def test_small_and_full_example(self):
        output = run_gdxmerge([
            os.path.join('examples', 'small_example.gdx'),
            os.path.join('examples', 'full_example.gdx'),
            f'output={os.path.join('examples', 'merged.gdx')}'
        ])
        self.assertEqual(output.returncode, 0)
        with open(os.path.join('output', 'gdxmerge', 'small_and_full_example.txt'), 'r') as file:
            first = output.stdout.split('\n')[3:]
            second = file.read().split('\n')[3:]
            self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')
