import unittest
import os
import subprocess
from examples.small_example import create_small_example
from examples.full_example import create_full_example


def run_gdxdump(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [os.path.join('..', '..', '..', 'build', 'gdxdump'), *command],
        capture_output=True,
        text=True
    )


class TestGdxDump(unittest.TestCase):

    @classmethod
    def setUpClass(cls) -> None:
        create_small_example(os.path.join('.', 'examples', 'small_example.gdx'))
        create_full_example(os.path.join('.', 'examples', 'full_example.gdx'))

    @classmethod
    def tearDownClass(cls) -> None:
        os.remove(os.path.join('.', 'examples', 'small_example.gdx'))
        os.remove(os.path.join('.', 'examples', 'full_example.gdx'))

    def test_small_example(self) -> None:
        output = run_gdxdump([os.path.join('.', 'examples', 'small_example.gdx')])
        self.assertEqual(output.returncode, 0)
        with open(os.path.join('.', 'output', 'gdxdump', 'small_example.txt'), 'r') as file:
            self.assertEqual(output.stdout, file.read())
        self.assertEqual(output.stderr, '')

    def test_full_example(self) -> None:
        output = run_gdxdump([os.path.join('.', 'examples', 'full_example.gdx')])
        self.assertEqual(output.returncode, 0)
        with open(os.path.join('.', 'output', 'gdxdump', 'full_example.txt'), 'r') as file:
            self.assertEqual(output.stdout, file.read())
        self.assertEqual(output.stderr, '')
