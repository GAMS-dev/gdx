import unittest
import os
import subprocess
from examples.small_example import create_small_example
from examples.full_example import create_full_example


def run_gdxdiff(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [os.path.join('..', '..', '..', 'build', 'gdxdiff'), *command],
        capture_output=True,
        text=True
    )


class TestGdxDiff(unittest.TestCase):

    def setUp(self):
        create_small_example(os.path.join('examples', 'small_example.gdx'))
        create_full_example(os.path.join('examples', 'full_example.gdx'))

    def tearDown(self):
        os.remove(os.path.join('examples', 'small_example.gdx'))
        os.remove(os.path.join('examples', 'full_example.gdx'))

    def test_small_and_full_example(self):
        output = run_gdxdiff([
            os.path.join('examples', 'small_example.gdx'),
            os.path.join('examples', 'full_example.gdx')
        ])
        self.assertEqual(output.returncode, 1)
        with open(os.path.join('output', 'gdxdiff', 'small_and_full_example.txt'), 'r') as file:
            self.assertEqual(output.stdout.split('\n')[2:], file.read().split('\n')[3:])
        self.assertEqual(output.stderr, '')
