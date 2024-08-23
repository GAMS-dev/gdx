import unittest
import os
import platform
import subprocess
import inspect
import gams.transfer as gt
from examples.small_example import create_small_example
from examples.full_example import create_full_example


class TestGdxMerge(unittest.TestCase):
    TEST_DIRECTORY_PATH = os.path.dirname(os.path.abspath(__file__))
    DIRECTORY_PATHS = {
        'examples': os.path.join(TEST_DIRECTORY_PATH, 'examples'),
        'output': os.path.join(TEST_DIRECTORY_PATH, 'output', 'gdxmerge')
    }
    FILE_PATHS = {
        'small_example': os.path.join(DIRECTORY_PATHS['examples'], 'small_example.gdx'),
        'full_example': os.path.join(DIRECTORY_PATHS['examples'], 'full_example.gdx'),
        'merged': os.path.join(DIRECTORY_PATHS['examples'], 'merged.gdx')
    }

    @classmethod
    def run_gdxmerge(cls, command: list[str]) -> subprocess.CompletedProcess[str]:
        if platform.system() == 'Windows':
            executable = ['Release', 'gdxmerge.exe']
        else:
            executable = ['build', 'gdxmerge']
        return subprocess.run(
            [os.path.join(cls.TEST_DIRECTORY_PATH, '..', '..', '..', *executable), *command],
            capture_output=True,
            text=True
        )

    def check_output(
        self,
        output: subprocess.CompletedProcess[str],
        return_code=0,
        file_name: str | None = None,
        first_offset: int | None = None,
        first_negative_offset: int | None = None,
        second_offset: int | None = None,
        second_negative_offset: int | None = None,
        first_delete: list[int] = [],
        second_delete: list[int] = []
    ) -> None:
        self.assertEqual(output.returncode, return_code)
        first = output.stdout.split('\n')[first_offset:first_negative_offset]
        for i in first_delete:
            del first[i]
        if file_name is None:
            file_name = f'{inspect.stack()[1].function.removeprefix('test_')}.txt'
        with open(os.path.join(self.DIRECTORY_PATHS['output'], file_name), 'r') as file:
            second = file.read().split('\n')[second_offset:second_negative_offset]
        for i in second_delete:
            del second[i]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def check_gdx_file(self, symbol_name: str, expected_values: list[list[str | float]]) -> None:
        container = gt.Container(load_from=self.FILE_PATHS['merged'])
        self.assertIn(symbol_name, container)
        symbol: gt.Parameter = container[symbol_name]  # type: ignore
        values = symbol.records.values.tolist()
        self.assertEqual(values, expected_values)

    @classmethod
    def setUpClass(cls) -> None:
        create_small_example(cls.FILE_PATHS['small_example'])
        create_full_example(cls.FILE_PATHS['full_example'])

    @classmethod
    def tearDownClass(cls) -> None:
        for file_path in cls.FILE_PATHS.values():
            os.remove(file_path)

    def test_empty_command(self) -> None:
        output = self.run_gdxmerge([])
        self.check_output(
            output,
            return_code=0,
            file_name='usage.txt',
            second_delete=[1]
        )

    def test_small_and_full_example(self) -> None:
        output = self.run_gdxmerge([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            f'output={self.FILE_PATHS['merged']}'
        ])
        self.check_output(
            output,
            return_code=0,
            first_offset=3,
            second_offset=3
        )

        container = gt.Container(load_from=self.FILE_PATHS['merged'])
        self.assertIn('i', container)
        self.assertIn('j', container)
        self.assertIn('d', container)
        self.assertIn('a', container)
        self.assertIn('b', container)
        self.assertIn('f', container)
        self.assertIn('c', container)
        self.assertIn('x', container)
        self.assertIn('z', container)
        self.assertIn('cost', container)
        self.assertIn('supply', container)
        self.assertIn('demand', container)
        self.assertIn('Merged_set_1', container)
        self.assertEqual(len(container), 13)

        self.check_gdx_file('i', [
            ['small_example', 'seattle', ''],
            ['small_example', 'san-diego', ''],
            ['full_example', 'seattle', ''],
            ['full_example', 'san-diego', '']
        ])

        self.check_gdx_file('j', [
            ['small_example', 'new-york', ''],
            ['small_example', 'chicago', ''],
            ['small_example', 'topeka', ''],
            ['full_example', 'new-york', ''],
            ['full_example', 'chicago', ''],
            ['full_example', 'topeka', '']
        ])

        self.check_gdx_file('d', [
            ['small_example', 'seattle', 'new-york', 2.5],
            ['small_example', 'seattle', 'chicago', 1.7],
            ['small_example', 'seattle', 'topeka', 1.8],
            ['small_example', 'san-diego', 'new-york', 2.5],
            ['small_example', 'san-diego', 'chicago', 1.8],
            ['small_example', 'san-diego', 'topeka', 1.4],
            ['full_example', 'seattle', 'new-york', 2.5],
            ['full_example', 'seattle', 'chicago', 1.7],
            ['full_example', 'seattle', 'topeka', 1.8],
            ['full_example', 'san-diego', 'new-york', 2.5],
            ['full_example', 'san-diego', 'chicago', 1.8],
            ['full_example', 'san-diego', 'topeka', 1.4]
        ])

        self.check_gdx_file('a', [
            ['full_example', 'seattle', 350.0],
            ['full_example', 'san-diego', 600.0]
        ])

        self.check_gdx_file('b', [
            ['full_example', 'new-york', 325.0],
            ['full_example', 'chicago', 300.0],
            ['full_example', 'topeka', 275.0]
        ])

        self.check_gdx_file('f', [
            ['full_example', 90.0]
        ])

        self.check_gdx_file('c', [
            ['full_example', 'seattle', 'new-york', 0.225],
            ['full_example', 'seattle', 'chicago', 0.153],
            ['full_example', 'seattle', 'topeka', 0.162],
            ['full_example', 'san-diego', 'new-york', 0.225],
            ['full_example', 'san-diego', 'chicago', 0.162],
            ['full_example', 'san-diego', 'topeka', 0.12599999999999997]
        ])

        self.check_gdx_file('x', [
            ['full_example', 'seattle', 'new-york', 50.0, 0.0, 0.0, float('inf'), 1.0],
            ['full_example', 'seattle', 'chicago', 300.0, 0.0, 0.0, float('inf'), 1.0],
            ['full_example', 'seattle', 'topeka', 0.0, 0.036, 0.0, float('inf'), 1.0],
            ['full_example', 'san-diego', 'new-york', 275.0, 0.0, 0.0, float('inf'), 1.0],
            ['full_example', 'san-diego', 'chicago', 0.0, 0.009, 0.0, float('inf'), 1.0],
            ['full_example', 'san-diego', 'topeka', 275.0, 0.0, 0.0, float('inf'), 1.0]
        ])

        self.check_gdx_file('z', [
            ['full_example', 153.675, 0.0, float('-inf'), float('inf'), 1.0]
        ])

        self.check_gdx_file('cost', [
            ['full_example', 0.0, 1.0, 0.0, 0.0, 1.0]
        ])

        self.check_gdx_file('supply', [
            ['full_example', 'seattle', 350.0, 0.0, float('-inf'), 350.0, 1.0],
            ['full_example', 'san-diego', 550.0, 0.0, float('-inf'), 600.0, 1.0]
        ])

        self.check_gdx_file('demand', [
            ['full_example', 'new-york', 325.0, 0.225, 325.0, float('inf'), 1.0],
            ['full_example', 'chicago', 300.0, 0.153, 300.0, float('inf'), 1.0],
            ['full_example', 'topeka', 275.0, 0.126, 275.0, float('inf'), 1.0]
        ])

        symbol: gt.Parameter = container['Merged_set_1']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['small_example', r'\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}  .+[/\\]examples[/\\]small_example\.gdx'],
            ['full_example', r'\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}  .+[/\\]examples[/\\]full_example\.gdx']
        ]
        self.assertEqual(len(first), 2)
        for item in first:
            self.assertEqual(len(item), 2)
        for i in range(2):
            self.assertEqual(first[i][0], second[i][0])
            self.assertRegex(first[i][1], second[i][1])
