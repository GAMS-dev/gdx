import unittest
import os
import platform
import subprocess
import inspect
import gams.transfer as gt
from examples.small_example import create_small_example
from examples.full_example import create_full_example
from examples.small_example_changed_data import create_small_example_changed_data
from examples.full_example_changed_variables import create_full_example_changed_variables
from examples.full_example_changed_data_and_variables import create_full_example_changed_data_and_variables
from examples.domain_examples import create_domain_example_1, create_domain_example_2


class TestGdxDiff(unittest.TestCase):
    TEST_DIRECTORY_PATH = os.path.dirname(os.path.abspath(__file__))
    DIRECTORY_PATHS = {
        'examples': os.path.join(TEST_DIRECTORY_PATH, 'examples'),
        'output': os.path.join(TEST_DIRECTORY_PATH, 'output', 'gdxdiff')
    }
    FILE_PATHS = {
        'small_example': os.path.join(DIRECTORY_PATHS['examples'], 'small_example.gdx'),
        'full_example': os.path.join(DIRECTORY_PATHS['examples'], 'full_example.gdx'),
        'small_example_changed_data': os.path.join(DIRECTORY_PATHS['examples'], 'small_example_changed_data.gdx'),
        'full_example_changed_variables': os.path.join(DIRECTORY_PATHS['examples'], 'full_example_changed_variables.gdx'),
        'full_example_changed_data_and_variables': os.path.join(DIRECTORY_PATHS['examples'], 'full_example_changed_data_and_variables.gdx'),
        'domain_example_1': os.path.join(DIRECTORY_PATHS['examples'], 'domain_example_1.gdx'),
        'domain_example_2': os.path.join(DIRECTORY_PATHS['examples'], 'domain_example_2.gdx'),
        'diff_file': os.path.join(DIRECTORY_PATHS['examples'], 'diff_file.gdx')
    }

    @classmethod
    def run_gdxdiff(cls, command: list[str]) -> subprocess.CompletedProcess[str]:
        if platform.system() == 'Windows':
            executable = ['Release', 'gdxdiff.exe']
        else:
            executable = ['build', 'gdxdiff']
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

    def check_gdx_file_symbols(
        self,
        symbol_names: list[str],
        container: gt.Container,
        symbols_len: int | None
    ) -> None:
        for symbol_name in symbol_names:
            with self.subTest(symbol_name=symbol_name):
                self.assertIn(symbol_name, container)
        self.assertEqual(
            len(container),
            symbols_len if symbols_len is not None else len(symbol_names)
        )

    def check_gdx_file_values(
        self,
        symbol_name: str,
        container: gt.Container,
        expected_values: list[list[str | float]]
    ) -> None:
        self.assertIn(symbol_name, container)
        symbol: gt.Parameter = container[symbol_name]  # type: ignore
        values = symbol.records.values.tolist()
        self.assertEqual(values, expected_values)

    def check_gdx_file(
        self,
        symbols: dict[str, list[list[str | float]]],
        container: gt.Container | None = None,
        symbols_len: int | None = None
    ) -> None:
        if container is None:
            container = gt.Container(load_from=self.FILE_PATHS['diff_file'])
        self.check_gdx_file_symbols(list(symbols.keys()), container, symbols_len)
        for symbol_name in symbols:
            self.check_gdx_file_values(symbol_name, container, symbols[symbol_name])

    @classmethod
    def setUpClass(cls) -> None:
        create_small_example(cls.FILE_PATHS['small_example'])
        create_full_example(cls.FILE_PATHS['full_example'])
        create_small_example_changed_data(cls.FILE_PATHS['small_example_changed_data'])
        create_full_example_changed_variables(cls.FILE_PATHS['full_example_changed_variables'])
        create_full_example_changed_data_and_variables(cls.FILE_PATHS['full_example_changed_data_and_variables'])
        create_domain_example_1(cls.FILE_PATHS['domain_example_1'])
        create_domain_example_2(cls.FILE_PATHS['domain_example_2'])

    @classmethod
    def tearDownClass(cls) -> None:
        for file_path in cls.FILE_PATHS.values():
            os.remove(file_path)

    def test_empty_command(self) -> None:
        output = self.run_gdxdiff([])
        self.check_output(
            output,
            return_code=2,
            file_name='usage.txt',
            second_offset=1,
            second_delete=[1]
        )

    def test_small_and_full_example(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['diff_file']
        ])
        self.check_output(
            output,
            return_code=1,
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'FilesCompared': [
                ['File1', self.FILE_PATHS['small_example']],
                ['File2', self.FILE_PATHS['full_example']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_small_example_and_small_example_changed_data(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['small_example_changed_data'],
            self.FILE_PATHS['diff_file']
        ])
        self.check_output(
            output,
            return_code=1,
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'd': [
                ['seattle', 'new-york', 'dif1', 2.5],
                ['seattle', 'new-york', 'dif2', 3.5],
                ['seattle', 'chicago', 'dif1', 1.7],
                ['seattle', 'chicago', 'dif2', 2.7],
                ['seattle', 'topeka', 'dif1', 1.8],
                ['seattle', 'topeka', 'dif2', 2.8],
                ['san-diego', 'new-york', 'dif1', 2.5],
                ['san-diego', 'new-york', 'dif2', 3.5],
                ['san-diego', 'chicago', 'dif1', 1.8],
                ['san-diego', 'chicago', 'dif2', 2.8],
                ['san-diego', 'topeka', 'dif1', 1.4],
                ['san-diego', 'topeka', 'dif2', 2.4]
            ],
            'FilesCompared': [
                ['File1', self.FILE_PATHS['small_example']],
                ['File2', self.FILE_PATHS['small_example_changed_data']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_small_example_and_small_example_changed_data_epsilon_absolute_1(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['small_example_changed_data'],
            self.FILE_PATHS['diff_file'],
            'Eps=1'
        ])
        self.check_output(
            output,
            return_code=1,
            file_name='small_example_and_small_example_changed_data.txt',
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'd': [
                ['seattle', 'chicago', 'dif1', 1.7],
                ['seattle', 'chicago', 'dif2', 2.7]
            ],
            'FilesCompared': [
                ['File1', self.FILE_PATHS['small_example']],
                ['File2', self.FILE_PATHS['small_example_changed_data']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_small_example_and_small_example_changed_data_epsilon_absolute_2(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['small_example_changed_data'],
            self.FILE_PATHS['diff_file'],
            'Eps=2'
        ])
        self.check_output(
            output,
            return_code=0,
            file_name='small_example_and_small_example_changed_data_epsilon.txt',
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'FilesCompared': [
                ['File1', self.FILE_PATHS['small_example']],
                ['File2', self.FILE_PATHS['small_example_changed_data']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_small_example_and_small_example_changed_data_epsilon_relative(self) -> None:
        for i in [1, 2]:
            with self.subTest(i=i):
                output = self.run_gdxdiff([
                    self.FILE_PATHS['small_example'],
                    self.FILE_PATHS['small_example_changed_data'],
                    self.FILE_PATHS['diff_file'],
                    f'RelEps={i}'
                ])
                self.check_output(
                    output,
                    return_code=0,
                    file_name='small_example_and_small_example_changed_data_epsilon.txt',
                    first_offset=2,
                    second_offset=3,
                    first_delete=[-3],
                    second_delete=[-3]
                )

                symbols: dict[str, list[list[str | float]]] = {
                    'FilesCompared': [
                        ['File1', self.FILE_PATHS['small_example']],
                        ['File2', self.FILE_PATHS['small_example_changed_data']]
                    ]
                }
                self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['full_example_changed_variables'],
            self.FILE_PATHS['diff_file']
        ])
        self.check_output(
            output,
            return_code=1,
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'x': [
                ['seattle', 'new-york', 'dif1', 50.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'new-york', 'dif2', 150.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'chicago', 'dif1', 300.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'chicago', 'dif2', 400.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'new-york', 'dif1', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'new-york', 'dif2', 375.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'topeka', 'dif1', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'topeka', 'dif2', 375.0, 0.0, 0.0, float('inf'), 1.0]
            ],
            'FilesCompared': [
                ['File1', self.FILE_PATHS['full_example']],
                ['File2', self.FILE_PATHS['full_example_changed_variables']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables_field_all(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['full_example_changed_variables'],
            self.FILE_PATHS['diff_file'],
            'Field=All'
        ])
        self.check_output(
            output,
            return_code=1,
            file_name='full_example_and_full_example_changed_variables.txt',
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'x': [
                ['seattle', 'new-york', 'dif1', 50.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'new-york', 'dif2', 150.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'chicago', 'dif1', 300.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'chicago', 'dif2', 400.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'new-york', 'dif1', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'new-york', 'dif2', 375.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'topeka', 'dif1', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'topeka', 'dif2', 375.0, 0.0, 0.0, float('inf'), 1.0]
            ],
            'FilesCompared': [
                ['File1', self.FILE_PATHS['full_example']],
                ['File2', self.FILE_PATHS['full_example_changed_variables']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables_field_l(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['full_example_changed_variables'],
            self.FILE_PATHS['diff_file'],
            'Field=L'
        ])
        self.check_output(
            output,
            return_code=1,
            file_name='full_example_and_full_example_changed_variables.txt',
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'x': [
                ['seattle', 'new-york', 'dif1', 50.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'new-york', 'dif2', 150.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'chicago', 'dif1', 300.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'chicago', 'dif2', 400.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'new-york', 'dif1', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'new-york', 'dif2', 375.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'topeka', 'dif1', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'topeka', 'dif2', 375.0, 0.0, 0.0, float('inf'), 1.0]
            ],
            'FilesCompared': [
                ['File1', self.FILE_PATHS['full_example']],
                ['File2', self.FILE_PATHS['full_example_changed_variables']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables_field_m(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['full_example_changed_variables'],
            self.FILE_PATHS['diff_file'],
            'Field=M'
        ])
        self.check_output(
            output,
            return_code=0,
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'FilesCompared': [
                ['File1', self.FILE_PATHS['full_example']],
                ['File2', self.FILE_PATHS['full_example_changed_variables']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables_field_l_field_only(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['full_example_changed_variables'],
            self.FILE_PATHS['diff_file'],
            'Field=L',
            'FldOnly'
        ])
        self.check_output(
            output,
            return_code=1,
            file_name='full_example_and_full_example_changed_variables.txt',
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'x': [
                ['seattle', 'new-york', 'dif1', 50.0],
                ['seattle', 'new-york', 'dif2', 150.0],
                ['seattle', 'chicago', 'dif1', 300.0],
                ['seattle', 'chicago', 'dif2', 400.0],
                ['san-diego', 'new-york', 'dif1', 275.0],
                ['san-diego', 'new-york', 'dif2', 375.0],
                ['san-diego', 'topeka', 'dif1', 275.0],
                ['san-diego', 'topeka', 'dif2', 375.0]
            ],
            'FilesCompared': [
                ['File1', self.FILE_PATHS['full_example']],
                ['File2', self.FILE_PATHS['full_example_changed_variables']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables_field_m_field_only(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['full_example_changed_variables'],
            self.FILE_PATHS['diff_file'],
            'Field=M',
            'FldOnly'
        ])
        self.check_output(
            output,
            return_code=0,
            file_name='full_example_and_full_example_changed_variables_field_m.txt',
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'FilesCompared': [
                ['File1', self.FILE_PATHS['full_example']],
                ['File2', self.FILE_PATHS['full_example_changed_variables']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_data_and_variables(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['full_example_changed_data_and_variables'],
            self.FILE_PATHS['diff_file']
        ])
        self.check_output(
            output,
            return_code=1,
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'c': [
                ['seattle', 'new-york', 'dif1', 0.225],
                ['seattle', 'new-york', 'dif2', 0.315],
                ['seattle', 'chicago', 'dif1', 0.153],
                ['seattle', 'chicago', 'dif2', 0.24300000000000002],
                ['seattle', 'topeka', 'dif1', 0.162],
                ['seattle', 'topeka', 'dif2', 0.25199999999999995],
                ['san-diego', 'new-york', 'dif1', 0.225],
                ['san-diego', 'new-york', 'dif2', 0.315],
                ['san-diego', 'chicago', 'dif1', 0.162],
                ['san-diego', 'chicago', 'dif2', 0.25199999999999995],
                ['san-diego', 'topeka', 'dif1', 0.12599999999999997],
                ['san-diego', 'topeka', 'dif2', 0.216]
            ],
            'd': [
                ['seattle', 'new-york', 'dif1', 2.5],
                ['seattle', 'new-york', 'dif2', 3.5],
                ['seattle', 'chicago', 'dif1', 1.7],
                ['seattle', 'chicago', 'dif2', 2.7],
                ['seattle', 'topeka', 'dif1', 1.8],
                ['seattle', 'topeka', 'dif2', 2.8],
                ['san-diego', 'new-york', 'dif1', 2.5],
                ['san-diego', 'new-york', 'dif2', 3.5],
                ['san-diego', 'chicago', 'dif1', 1.8],
                ['san-diego', 'chicago', 'dif2', 2.8],
                ['san-diego', 'topeka', 'dif1', 1.4],
                ['san-diego', 'topeka', 'dif2', 2.4]
            ],
            'x': [
                ['seattle', 'new-york', 'dif1', 50.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'new-york', 'dif2', 150.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'chicago', 'dif1', 300.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'chicago', 'dif2', 400.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'new-york', 'dif1', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'new-york', 'dif2', 375.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'topeka', 'dif1', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'topeka', 'dif2', 375.0, 0.0, 0.0, float('inf'), 1.0]
            ],
            'FilesCompared': [
                ['File1', self.FILE_PATHS['full_example']],
                ['File2', self.FILE_PATHS['full_example_changed_data_and_variables']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_data_and_variables_id_x(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['full_example_changed_data_and_variables'],
            self.FILE_PATHS['diff_file'],
            'Id=x'
        ])
        self.check_output(
            output,
            return_code=1,
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'x': [
                ['seattle', 'new-york', 'dif1', 50.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'new-york', 'dif2', 150.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'chicago', 'dif1', 300.0, 0.0, 0.0, float('inf'), 1.0],
                ['seattle', 'chicago', 'dif2', 400.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'new-york', 'dif1', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'new-york', 'dif2', 375.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'topeka', 'dif1', 275.0, 0.0, 0.0, float('inf'), 1.0],
                ['san-diego', 'topeka', 'dif2', 375.0, 0.0, 0.0, float('inf'), 1.0]
            ],
            'FilesCompared': [
                ['File1', self.FILE_PATHS['full_example']],
                ['File2', self.FILE_PATHS['full_example_changed_data_and_variables']]
            ]
        }
        self.check_gdx_file(symbols)

        output_space_separator = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['full_example_changed_data_and_variables'],
            self.FILE_PATHS['diff_file'],
            'Id', 'x'
        ])
        self.check_output(
            output_space_separator,
            return_code=1,
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_data_and_variables_id_c_d(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['full_example_changed_data_and_variables'],
            self.FILE_PATHS['diff_file'],
            'Id=c', 'Id=d'
        ])
        self.check_output(
            output,
            return_code=1,
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'c': [
                ['seattle', 'new-york', 'dif1', 0.225],
                ['seattle', 'new-york', 'dif2', 0.315],
                ['seattle', 'chicago', 'dif1', 0.153],
                ['seattle', 'chicago', 'dif2', 0.24300000000000002],
                ['seattle', 'topeka', 'dif1', 0.162],
                ['seattle', 'topeka', 'dif2', 0.25199999999999995],
                ['san-diego', 'new-york', 'dif1', 0.225],
                ['san-diego', 'new-york', 'dif2', 0.315],
                ['san-diego', 'chicago', 'dif1', 0.162],
                ['san-diego', 'chicago', 'dif2', 0.25199999999999995],
                ['san-diego', 'topeka', 'dif1', 0.12599999999999997],
                ['san-diego', 'topeka', 'dif2', 0.216]
            ],
            'd': [
                ['seattle', 'new-york', 'dif1', 2.5],
                ['seattle', 'new-york', 'dif2', 3.5],
                ['seattle', 'chicago', 'dif1', 1.7],
                ['seattle', 'chicago', 'dif2', 2.7],
                ['seattle', 'topeka', 'dif1', 1.8],
                ['seattle', 'topeka', 'dif2', 2.8],
                ['san-diego', 'new-york', 'dif1', 2.5],
                ['san-diego', 'new-york', 'dif2', 3.5],
                ['san-diego', 'chicago', 'dif1', 1.8],
                ['san-diego', 'chicago', 'dif2', 2.8],
                ['san-diego', 'topeka', 'dif1', 1.4],
                ['san-diego', 'topeka', 'dif2', 2.4]
            ],
            'FilesCompared': [
                ['File1', self.FILE_PATHS['full_example']],
                ['File2', self.FILE_PATHS['full_example_changed_data_and_variables']]
            ]
        }
        self.check_gdx_file(symbols)

        output_quotation_marks = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['full_example_changed_data_and_variables'],
            self.FILE_PATHS['diff_file'],
            'Id=\'c d\''
        ])
        self.check_output(
            output_quotation_marks,
            return_code=1,
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables_differences_only(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['full_example_changed_variables'],
            self.FILE_PATHS['diff_file'],
            'DiffOnly'
        ])
        self.check_output(
            output,
            return_code=1,
            file_name='full_example_and_full_example_changed_variables.txt',
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'x': [
                ['seattle', 'new-york', 'Level', 'dif1', 50.0],
                ['seattle', 'new-york', 'Level', 'dif2', 150.0],
                ['seattle', 'chicago', 'Level', 'dif1', 300.0],
                ['seattle', 'chicago', 'Level', 'dif2', 400.0],
                ['san-diego', 'new-york', 'Level', 'dif1', 275.0],
                ['san-diego', 'new-york', 'Level', 'dif2', 375.0],
                ['san-diego', 'topeka', 'Level', 'dif1', 275.0],
                ['san-diego', 'topeka', 'Level', 'dif2', 375.0]
            ],
            'FilesCompared': [
                ['File1', self.FILE_PATHS['full_example']],
                ['File2', self.FILE_PATHS['full_example_changed_variables']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_domain_example_1_and_domain_example_2(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['domain_example_1'],
            self.FILE_PATHS['domain_example_2'],
            self.FILE_PATHS['diff_file']
        ])
        self.check_output(
            output,
            return_code=1,
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'FilesCompared': [
                ['File1', self.FILE_PATHS['domain_example_1']],
                ['File2', self.FILE_PATHS['domain_example_2']]
            ]
        }
        self.check_gdx_file(symbols)

    def test_domain_example_1_and_domain_example_2_compare_symbol_domains(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['domain_example_1'],
            self.FILE_PATHS['domain_example_2'],
            self.FILE_PATHS['diff_file'],
            'CmpDomains'
        ])
        self.check_output(
            output,
            return_code=1,
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        symbols: dict[str, list[list[str | float]]] = {
            'FilesCompared': [
                ['File1', self.FILE_PATHS['domain_example_1']],
                ['File2', self.FILE_PATHS['domain_example_2']]
            ]
        }
        self.check_gdx_file(symbols)
