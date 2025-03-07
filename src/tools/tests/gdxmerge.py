import unittest
import os
import platform
import subprocess
import tempfile
import inspect
import gams.transfer as gt

from examples.small_example import create_small_example
from examples.full_example import create_full_example
from examples.small_example_changed_data import create_small_example_changed_data


class TestGdxMerge(unittest.TestCase):
    TESTS_DIRECTORY_PATH = os.path.dirname(os.path.abspath(__file__))
    DIRECTORY_PATHS = {
        'examples': os.path.join(TESTS_DIRECTORY_PATH, 'examples'),
        'output': os.path.join(TESTS_DIRECTORY_PATH, 'output', 'gdxmerge')
    }
    FILE_NAMES = [
        'small_example',
        'full_example',
        'small_example_changed_data',
        'merge_file'
    ]
    FILE_PATHS: dict[str, str]

    @classmethod
    def setUpClass(cls) -> None:
        cls.FILE_PATHS = {
            file_name: os.path.join(
                cls.DIRECTORY_PATHS['examples'],
                f'{file_name}.gdx'
            ) for file_name in cls.FILE_NAMES
        }

        create_small_example(cls.FILE_PATHS['small_example'])
        create_full_example(cls.FILE_PATHS['full_example'])
        create_small_example_changed_data(cls.FILE_PATHS['small_example_changed_data'])

    @classmethod
    def tearDownClass(cls) -> None:
        for file_path in cls.FILE_PATHS.values():
            os.remove(file_path)

    @classmethod
    def run_gdxmerge(cls, command: list[str]) -> subprocess.CompletedProcess[str]:
        EXECUTABLE_NAME = 'gdxmerge'
        if platform.system() == 'Windows':
            EXECUTABLE_PATH = ['Release', f'{EXECUTABLE_NAME}.exe']
        else:
            EXECUTABLE_PATH = ['build', EXECUTABLE_NAME]
        return subprocess.run(
            [os.path.join(cls.TESTS_DIRECTORY_PATH, '..', '..', '..', *EXECUTABLE_PATH), *command],
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

    def check_gdx_file_symbols(
        self,
        container: gt.Container,
        symbol_names: list[str]
    ) -> None:
        for symbol_name in symbol_names:
            with self.subTest(symbol_name=symbol_name):
                self.assertIn(symbol_name, container)
        self.assertEqual(len(container), len(symbol_names) + 1)

    def check_gdx_file_values(
        self,
        container: gt.Container,
        symbol_name: str,
        expected_values: list[list[str | float]]
    ) -> None:
        self.assertIn(symbol_name, container)
        symbol: gt.Parameter = container[symbol_name]  # type: ignore
        values = symbol.records.values.tolist()
        self.assertEqual(values, expected_values)

    def check_gdx_file(
        self,
        symbols: dict[str, list[list[str | float]]],
        file_names: list[str]
    ) -> None:
        container = gt.Container(load_from=self.FILE_PATHS['merge_file'])
        self.check_gdx_file_symbols(container, list(symbols.keys()))
        for symbol_name in symbols:
            self.check_gdx_file_values(container, symbol_name, symbols[symbol_name])

        symbol: gt.Parameter = container['Merged_set_1']  # type: ignore
        first = symbol.records.values.tolist()
        second: list[list[str]] = []
        for i in range(len(file_names)):
            second.append([file_names[i], f'{r'\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}  .+[/\\]examples[/\\]'}{file_names[i]}.gdx'])
        self.assertEqual(len(first), len(file_names))
        for item in first:
            self.assertEqual(len(item), 2)
        for i in range(len(file_names)):
            self.assertEqual(first[i][0], second[i][0])
            self.assertRegex(first[i][1], second[i][1])

    def test_empty_command(self) -> None:
        output = self.run_gdxmerge([])
        self.check_output(
            output,
            return_code=0,
            file_name='usage.txt',
            first_delete=[1],
            second_delete=[1]
        )

    def test_small_example_and_full_example(self) -> None:
        output = self.run_gdxmerge([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            f'Output={self.FILE_PATHS['merge_file']}'
        ])
        self.check_output(
            output,
            return_code=0,
            first_offset=3,
            second_offset=3
        )

        symbols: dict[str, list[list[str | float]]] = {
            'i': [
                ['small_example', 'seattle', ''],
                ['small_example', 'san-diego', ''],
                ['full_example', 'seattle', ''],
                ['full_example', 'san-diego', '']
            ],
            'j': [
                ['small_example', 'new-york', ''],
                ['small_example', 'chicago', ''],
                ['small_example', 'topeka', ''],
                ['full_example', 'new-york', ''],
                ['full_example', 'chicago', ''],
                ['full_example', 'topeka', '']
            ],
            'd': [
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
            ],
            'a': [
                ['full_example', 'seattle', 350.0],
                ['full_example', 'san-diego', 600.0]
            ],
            'b': [
                ['full_example', 'new-york', 325.0],
                ['full_example', 'chicago', 300.0],
                ['full_example', 'topeka', 275.0]
            ],
            'f': [
                ['full_example', 90.0]
            ],
            'c': [
                ['full_example', 'seattle', 'new-york', 0.225],
                ['full_example', 'seattle', 'chicago', 0.153],
                ['full_example', 'seattle', 'topeka', 0.162],
                ['full_example', 'san-diego', 'new-york', 0.225],
                ['full_example', 'san-diego', 'chicago', 0.162],
                ['full_example', 'san-diego', 'topeka', 0.12599999999999997]
            ],
            'x': [
                ['full_example', 'seattle', 'new-york', 50.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'seattle', 'chicago', 300.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'seattle', 'topeka', 0.0, 0.036, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'new-york', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'chicago', 0.0, 0.009, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'topeka', 275.0, 0.0, 0.0, float('inf'), 1.0]
            ],
            'z': [
                ['full_example', 153.675, 0.0, float('-inf'), float('inf'), 1.0]
            ],
            'cost': [
                ['full_example', 0.0, 1.0, 0.0, 0.0, 1.0]
            ],
            'supply': [
                ['full_example', 'seattle', 350.0, 0.0, float('-inf'), 350.0, 1.0],
                ['full_example', 'san-diego', 550.0, 0.0, float('-inf'), 600.0, 1.0]
            ],
            'demand': [
                ['full_example', 'new-york', 325.0, 0.225, 325.0, float('inf'), 1.0],
                ['full_example', 'chicago', 300.0, 0.153, 300.0, float('inf'), 1.0],
                ['full_example', 'topeka', 275.0, 0.126, 275.0, float('inf'), 1.0]
            ]
        }
        self.check_gdx_file(symbols, ['small_example', 'full_example'])

    def test_small_example_and_full_example_id_i(self) -> None:
        output = self.run_gdxmerge([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            f'Output={self.FILE_PATHS['merge_file']}',
            'Id=i'
        ])
        self.check_output(
            output,
            return_code=0,
            first_delete=[1, 1, 1],
            second_delete=[1, 1, 1]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'i': [
                ['small_example', 'seattle', ''],
                ['small_example', 'san-diego', ''],
                ['full_example', 'seattle', ''],
                ['full_example', 'san-diego', '']
            ]
        }
        self.check_gdx_file(symbols, ['small_example', 'full_example'])

    def test_small_example_and_full_example_id_i_j(self) -> None:
        output = self.run_gdxmerge([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            f'Output={self.FILE_PATHS['merge_file']}',
            'Id=i', 'Id=j'
        ])
        self.check_output(
            output,
            return_code=0,
            first_delete=[2, 2, 2],
            second_delete=[2, 2, 2]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'i': [
                ['small_example', 'seattle', ''],
                ['small_example', 'san-diego', ''],
                ['full_example', 'seattle', ''],
                ['full_example', 'san-diego', '']
            ],
            'j': [
                ['small_example', 'new-york', ''],
                ['small_example', 'chicago', ''],
                ['small_example', 'topeka', ''],
                ['full_example', 'new-york', ''],
                ['full_example', 'chicago', ''],
                ['full_example', 'topeka', '']
            ]
        }
        self.check_gdx_file(symbols, ['small_example', 'full_example'])

        output_quotation_marks = self.run_gdxmerge([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            f'Output={self.FILE_PATHS['merge_file']}',
            'Id=\'i j\''
        ])
        self.check_output(
            output_quotation_marks,
            return_code=0,
            first_delete=[2, 2, 2],
            second_delete=[2, 2, 2]
        )
        self.check_gdx_file(symbols, ['small_example', 'full_example'])

        output_comma_separator = self.run_gdxmerge([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            f'Output={self.FILE_PATHS['merge_file']}',
            'Id=i,j'
        ])
        self.check_output(
            output_comma_separator,
            return_code=0,
            first_delete=[2, 2, 2],
            second_delete=[2, 2, 2]
        )
        self.check_gdx_file(symbols, ['small_example', 'full_example'])

    def test_small_example_and_full_example_exclude_i(self) -> None:
        output = self.run_gdxmerge([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            f'Output={self.FILE_PATHS['merge_file']}',
            'Exclude=i'
        ])
        self.check_output(
            output,
            return_code=0,
            first_delete=[1, 1, 1],
            second_delete=[1, 1, 1]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'j': [
                ['small_example', 'new-york', ''],
                ['small_example', 'chicago', ''],
                ['small_example', 'topeka', ''],
                ['full_example', 'new-york', ''],
                ['full_example', 'chicago', ''],
                ['full_example', 'topeka', '']
            ],
            'd': [
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
            ],
            'a': [
                ['full_example', 'seattle', 350.0],
                ['full_example', 'san-diego', 600.0]
            ],
            'b': [
                ['full_example', 'new-york', 325.0],
                ['full_example', 'chicago', 300.0],
                ['full_example', 'topeka', 275.0]
            ],
            'f': [
                ['full_example', 90.0]
            ],
            'c': [
                ['full_example', 'seattle', 'new-york', 0.225],
                ['full_example', 'seattle', 'chicago', 0.153],
                ['full_example', 'seattle', 'topeka', 0.162],
                ['full_example', 'san-diego', 'new-york', 0.225],
                ['full_example', 'san-diego', 'chicago', 0.162],
                ['full_example', 'san-diego', 'topeka', 0.12599999999999997]
            ],
            'x': [
                ['full_example', 'seattle', 'new-york', 50.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'seattle', 'chicago', 300.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'seattle', 'topeka', 0.0, 0.036, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'new-york', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'chicago', 0.0, 0.009, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'topeka', 275.0, 0.0, 0.0, float('inf'), 1.0]
            ],
            'z': [
                ['full_example', 153.675, 0.0, float('-inf'), float('inf'), 1.0]
            ],
            'cost': [
                ['full_example', 0.0, 1.0, 0.0, 0.0, 1.0]
            ],
            'supply': [
                ['full_example', 'seattle', 350.0, 0.0, float('-inf'), 350.0, 1.0],
                ['full_example', 'san-diego', 550.0, 0.0, float('-inf'), 600.0, 1.0]
            ],
            'demand': [
                ['full_example', 'new-york', 325.0, 0.225, 325.0, float('inf'), 1.0],
                ['full_example', 'chicago', 300.0, 0.153, 300.0, float('inf'), 1.0],
                ['full_example', 'topeka', 275.0, 0.126, 275.0, float('inf'), 1.0]
            ]
        }
        self.check_gdx_file(symbols, ['small_example', 'full_example'])

    def test_small_example_and_full_example_exclude_i_j(self) -> None:
        output = self.run_gdxmerge([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            f'Output={self.FILE_PATHS['merge_file']}',
            'Exclude=i', 'Exclude=j'
        ])
        self.check_output(
            output,
            return_code=0,
            first_delete=[2, 2, 2],
            second_delete=[2, 2, 2]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'd': [
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
            ],
            'a': [
                ['full_example', 'seattle', 350.0],
                ['full_example', 'san-diego', 600.0]
            ],
            'b': [
                ['full_example', 'new-york', 325.0],
                ['full_example', 'chicago', 300.0],
                ['full_example', 'topeka', 275.0]
            ],
            'f': [
                ['full_example', 90.0]
            ],
            'c': [
                ['full_example', 'seattle', 'new-york', 0.225],
                ['full_example', 'seattle', 'chicago', 0.153],
                ['full_example', 'seattle', 'topeka', 0.162],
                ['full_example', 'san-diego', 'new-york', 0.225],
                ['full_example', 'san-diego', 'chicago', 0.162],
                ['full_example', 'san-diego', 'topeka', 0.12599999999999997]
            ],
            'x': [
                ['full_example', 'seattle', 'new-york', 50.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'seattle', 'chicago', 300.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'seattle', 'topeka', 0.0, 0.036, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'new-york', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'chicago', 0.0, 0.009, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'topeka', 275.0, 0.0, 0.0, float('inf'), 1.0]
            ],
            'z': [
                ['full_example', 153.675, 0.0, float('-inf'), float('inf'), 1.0]
            ],
            'cost': [
                ['full_example', 0.0, 1.0, 0.0, 0.0, 1.0]
            ],
            'supply': [
                ['full_example', 'seattle', 350.0, 0.0, float('-inf'), 350.0, 1.0],
                ['full_example', 'san-diego', 550.0, 0.0, float('-inf'), 600.0, 1.0]
            ],
            'demand': [
                ['full_example', 'new-york', 325.0, 0.225, 325.0, float('inf'), 1.0],
                ['full_example', 'chicago', 300.0, 0.153, 300.0, float('inf'), 1.0],
                ['full_example', 'topeka', 275.0, 0.126, 275.0, float('inf'), 1.0]
            ]
        }
        self.check_gdx_file(symbols, ['small_example', 'full_example'])

        output_quotation_marks = self.run_gdxmerge([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            f'Output={self.FILE_PATHS['merge_file']}',
            'Exclude=\'i j\''
        ])
        self.check_output(
            output_quotation_marks,
            return_code=0,
            first_delete=[2, 2, 2],
            second_delete=[2, 2, 2]
        )
        self.check_gdx_file(symbols, ['small_example', 'full_example'])

        output_comma_separator = self.run_gdxmerge([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            f'Output={self.FILE_PATHS['merge_file']}',
            'Exclude=i,j'
        ])
        self.check_output(
            output_comma_separator,
            return_code=0,
            first_delete=[2, 2, 2],
            second_delete=[2, 2, 2]
        )
        self.check_gdx_file(symbols, ['small_example', 'full_example'])

    def test_small_example_and_full_example_big_symbols(self) -> None:
        output = self.run_gdxmerge([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            f'Output={self.FILE_PATHS['merge_file']}',
            'Big=10'
        ])
        self.check_output(
            output,
            return_code=0,
            first_offset=5,
            second_offset=5
        )

        symbols: dict[str, list[list[str | float]]] = {
            'i': [
                ['small_example', 'seattle', ''],
                ['small_example', 'san-diego', ''],
                ['full_example', 'seattle', ''],
                ['full_example', 'san-diego', '']
            ],
            'j': [
                ['small_example', 'new-york', ''],
                ['small_example', 'chicago', ''],
                ['small_example', 'topeka', ''],
                ['full_example', 'new-york', ''],
                ['full_example', 'chicago', ''],
                ['full_example', 'topeka', '']
            ],
            'd': [
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
            ],
            'a': [
                ['full_example', 'seattle', 350.0],
                ['full_example', 'san-diego', 600.0]
            ],
            'b': [
                ['full_example', 'new-york', 325.0],
                ['full_example', 'chicago', 300.0],
                ['full_example', 'topeka', 275.0]
            ],
            'f': [
                ['full_example', 90.0]
            ],
            'c': [
                ['full_example', 'seattle', 'new-york', 0.225],
                ['full_example', 'seattle', 'chicago', 0.153],
                ['full_example', 'seattle', 'topeka', 0.162],
                ['full_example', 'san-diego', 'new-york', 0.225],
                ['full_example', 'san-diego', 'chicago', 0.162],
                ['full_example', 'san-diego', 'topeka', 0.12599999999999997]
            ],
            'x': [
                ['full_example', 'seattle', 'new-york', 50.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'seattle', 'chicago', 300.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'seattle', 'topeka', 0.0, 0.036, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'new-york', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'chicago', 0.0, 0.009, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'topeka', 275.0, 0.0, 0.0, float('inf'), 1.0]
            ],
            'z': [
                ['full_example', 153.675, 0.0, float('-inf'), float('inf'), 1.0]
            ],
            'cost': [
                ['full_example', 0.0, 1.0, 0.0, 0.0, 1.0]
            ],
            'supply': [
                ['full_example', 'seattle', 350.0, 0.0, float('-inf'), 350.0, 1.0],
                ['full_example', 'san-diego', 550.0, 0.0, float('-inf'), 600.0, 1.0]
            ],
            'demand': [
                ['full_example', 'new-york', 325.0, 0.225, 325.0, float('inf'), 1.0],
                ['full_example', 'chicago', 300.0, 0.153, 300.0, float('inf'), 1.0],
                ['full_example', 'topeka', 275.0, 0.126, 275.0, float('inf'), 1.0]
            ]
        }
        self.check_gdx_file(symbols, ['small_example', 'full_example'])

    def test_small_example_and_full_example_strict_mode(self) -> None:
        with tempfile.NamedTemporaryFile(suffix='.gdx') as temporary_file:
            output = self.run_gdxmerge([
                self.FILE_PATHS['small_example'],
                self.FILE_PATHS['full_example'],
                f'Output={temporary_file.name}',
                'Strict=true'
            ])
            self.assertEqual(output.returncode, 1)
            self.assertEqual(
                output.stdout,
                f'*** Error  : Output file "{temporary_file.name}" already exists (strict mode)\n'
            )
            self.assertEqual(output.stderr, '')

    def test_small_example_and_small_example_changed_data_and_full_example(self) -> None:
        output = self.run_gdxmerge([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['small_example_changed_data'],
            self.FILE_PATHS['full_example'],
            f'Output={self.FILE_PATHS['merge_file']}'
        ])
        self.check_output(
            output,
            return_code=0,
            first_offset=4,
            second_offset=4
        )

        symbols: dict[str, list[list[str | float]]] = {
            'i': [
                ['small_example', 'seattle', ''],
                ['small_example', 'san-diego', ''],
                ['small_example_changed_data', 'seattle', ''],
                ['small_example_changed_data', 'san-diego', ''],
                ['full_example', 'seattle', ''],
                ['full_example', 'san-diego', '']
            ],
            'j': [
                ['small_example', 'new-york', ''],
                ['small_example', 'chicago', ''],
                ['small_example', 'topeka', ''],
                ['small_example_changed_data', 'new-york', ''],
                ['small_example_changed_data', 'chicago', ''],
                ['small_example_changed_data', 'topeka', ''],
                ['full_example', 'new-york', ''],
                ['full_example', 'chicago', ''],
                ['full_example', 'topeka', '']
            ],
            'd': [
                ['small_example', 'seattle', 'new-york', 2.5],
                ['small_example', 'seattle', 'chicago', 1.7],
                ['small_example', 'seattle', 'topeka', 1.8],
                ['small_example', 'san-diego', 'new-york', 2.5],
                ['small_example', 'san-diego', 'chicago', 1.8],
                ['small_example', 'san-diego', 'topeka', 1.4],
                ['small_example_changed_data', 'seattle', 'new-york', 3.5],
                ['small_example_changed_data', 'seattle', 'chicago', 2.7],
                ['small_example_changed_data', 'seattle', 'topeka', 2.8],
                ['small_example_changed_data', 'san-diego', 'new-york', 3.5],
                ['small_example_changed_data', 'san-diego', 'chicago', 2.8],
                ['small_example_changed_data', 'san-diego', 'topeka', 2.4],
                ['full_example', 'seattle', 'new-york', 2.5],
                ['full_example', 'seattle', 'chicago', 1.7],
                ['full_example', 'seattle', 'topeka', 1.8],
                ['full_example', 'san-diego', 'new-york', 2.5],
                ['full_example', 'san-diego', 'chicago', 1.8],
                ['full_example', 'san-diego', 'topeka', 1.4]
            ],
            'a': [
                ['full_example', 'seattle', 350.0],
                ['full_example', 'san-diego', 600.0]
            ],
            'b': [
                ['full_example', 'new-york', 325.0],
                ['full_example', 'chicago', 300.0],
                ['full_example', 'topeka', 275.0]
            ],
            'f': [
                ['full_example', 90.0]
            ],
            'c': [
                ['full_example', 'seattle', 'new-york', 0.225],
                ['full_example', 'seattle', 'chicago', 0.153],
                ['full_example', 'seattle', 'topeka', 0.162],
                ['full_example', 'san-diego', 'new-york', 0.225],
                ['full_example', 'san-diego', 'chicago', 0.162],
                ['full_example', 'san-diego', 'topeka', 0.12599999999999997]
            ],
            'x': [
                ['full_example', 'seattle', 'new-york', 50.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'seattle', 'chicago', 300.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'seattle', 'topeka', 0.0, 0.036, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'new-york', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'chicago', 0.0, 0.009, 0.0, float('inf'), 1.0],
                ['full_example', 'san-diego', 'topeka', 275.0, 0.0, 0.0, float('inf'), 1.0]
            ],
            'z': [
                ['full_example', 153.675, 0.0, float('-inf'), float('inf'), 1.0]
            ],
            'cost': [
                ['full_example', 0.0, 1.0, 0.0, 0.0, 1.0]
            ],
            'supply': [
                ['full_example', 'seattle', 350.0, 0.0, float('-inf'), 350.0, 1.0],
                ['full_example', 'san-diego', 550.0, 0.0, float('-inf'), 600.0, 1.0]
            ],
            'demand': [
                ['full_example', 'new-york', 325.0, 0.225, 325.0, float('inf'), 1.0],
                ['full_example', 'chicago', 300.0, 0.153, 300.0, float('inf'), 1.0],
                ['full_example', 'topeka', 275.0, 0.126, 275.0, float('inf'), 1.0]
            ]
        }
        self.check_gdx_file(symbols, ['small_example', 'small_example_changed_data', 'full_example'])

    def test_commands_from_file(self) -> None:
        for separator in [' ', '\t', '\n']:
            with tempfile.NamedTemporaryFile(suffix='.txt') as temporary_file:
                temporary_file.write(
                    str.encode(separator.join([
                        self.FILE_PATHS['small_example'],
                        self.FILE_PATHS['full_example'],
                        f'Output={self.FILE_PATHS['merge_file']}'
                    ]))
                )
                temporary_file.flush()
                output = self.run_gdxmerge([f'@{temporary_file.name}'])
                self.check_output(
                    output,
                    return_code=0,
                    file_name='small_example_and_full_example.txt',
                    first_offset=3,
                    second_offset=3
                )

                symbols: dict[str, list[list[str | float]]] = {
                    'i': [
                        ['small_example', 'seattle', ''],
                        ['small_example', 'san-diego', ''],
                        ['full_example', 'seattle', ''],
                        ['full_example', 'san-diego', '']
                    ],
                    'j': [
                        ['small_example', 'new-york', ''],
                        ['small_example', 'chicago', ''],
                        ['small_example', 'topeka', ''],
                        ['full_example', 'new-york', ''],
                        ['full_example', 'chicago', ''],
                        ['full_example', 'topeka', '']
                    ],
                    'd': [
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
                    ],
                    'a': [
                        ['full_example', 'seattle', 350.0],
                        ['full_example', 'san-diego', 600.0]
                    ],
                    'b': [
                        ['full_example', 'new-york', 325.0],
                        ['full_example', 'chicago', 300.0],
                        ['full_example', 'topeka', 275.0]
                    ],
                    'f': [
                        ['full_example', 90.0]
                    ],
                    'c': [
                        ['full_example', 'seattle', 'new-york', 0.225],
                        ['full_example', 'seattle', 'chicago', 0.153],
                        ['full_example', 'seattle', 'topeka', 0.162],
                        ['full_example', 'san-diego', 'new-york', 0.225],
                        ['full_example', 'san-diego', 'chicago', 0.162],
                        ['full_example', 'san-diego', 'topeka', 0.12599999999999997]
                    ],
                    'x': [
                        ['full_example', 'seattle', 'new-york', 50.0, 0.0, 0.0, float('inf'), 1.0],
                        ['full_example', 'seattle', 'chicago', 300.0, 0.0, 0.0, float('inf'), 1.0],
                        ['full_example', 'seattle', 'topeka', 0.0, 0.036, 0.0, float('inf'), 1.0],
                        ['full_example', 'san-diego', 'new-york', 275.0, 0.0, 0.0, float('inf'), 1.0],
                        ['full_example', 'san-diego', 'chicago', 0.0, 0.009, 0.0, float('inf'), 1.0],
                        ['full_example', 'san-diego', 'topeka', 275.0, 0.0, 0.0, float('inf'), 1.0]
                    ],
                    'z': [
                        ['full_example', 153.675, 0.0, float('-inf'), float('inf'), 1.0]
                    ],
                    'cost': [
                        ['full_example', 0.0, 1.0, 0.0, 0.0, 1.0]
                    ],
                    'supply': [
                        ['full_example', 'seattle', 350.0, 0.0, float('-inf'), 350.0, 1.0],
                        ['full_example', 'san-diego', 550.0, 0.0, float('-inf'), 600.0, 1.0]
                    ],
                    'demand': [
                        ['full_example', 'new-york', 325.0, 0.225, 325.0, float('inf'), 1.0],
                        ['full_example', 'chicago', 300.0, 0.153, 300.0, float('inf'), 1.0],
                        ['full_example', 'topeka', 275.0, 0.126, 275.0, float('inf'), 1.0]
                    ]
                }
                self.check_gdx_file(symbols, ['small_example', 'full_example'])
