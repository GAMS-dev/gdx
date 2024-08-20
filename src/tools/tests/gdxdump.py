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

    def test_small_example(self) -> None:
        output = run_gdxdump([self.SMALL_EXAMPLE_FILE_PATH])
        self.assertEqual(output.returncode, 0)
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'small_example.txt'), 'r') as file:
            self.assertEqual(output.stdout, file.read())
        self.assertEqual(output.stderr, '')

    def test_full_example(self) -> None:
        output = run_gdxdump([self.FULL_EXAMPLE_FILE_PATH])
        self.assertEqual(output.returncode, 0)
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example.txt'), 'r') as file:
            self.assertEqual(output.stdout, file.read())
        self.assertEqual(output.stderr, '')
