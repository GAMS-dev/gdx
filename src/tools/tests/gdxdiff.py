import unittest
import os
import platform
import subprocess
import inspect
import gams.transfer as gt
from examples.small_example import create_small_example
from examples.full_example import create_full_example
from examples.changed_small_example import create_changed_small_example
from examples.changed_full_example import create_changed_full_example


class TestGdxDiff(unittest.TestCase):
    TEST_DIRECTORY_PATH = os.path.dirname(os.path.abspath(__file__))
    DIRECTORY_PATHS = {
        'examples': os.path.join(TEST_DIRECTORY_PATH, 'examples'),
        'output': os.path.join(TEST_DIRECTORY_PATH, 'output', 'gdxdiff')
    }
    FILE_PATHS = {
        'small_example': os.path.join(DIRECTORY_PATHS['examples'], 'small_example.gdx'),
        'full_example': os.path.join(DIRECTORY_PATHS['examples'], 'full_example.gdx'),
        'changed_small_example': os.path.join(DIRECTORY_PATHS['examples'], 'changed_small_example.gdx'),
        'changed_full_example': os.path.join(DIRECTORY_PATHS['examples'], 'changed_full_example.gdx'),
        'diff_file': os.path.join(DIRECTORY_PATHS['examples'], 'diffile.gdx')
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
        container: gt.Container | None = None
    ) -> None:
        if container is None:
            container = gt.Container(load_from=self.FILE_PATHS['diff_file'])
        for symbol_name in symbol_names:
            with self.subTest(symbol_name=symbol_name):
                self.assertIn(symbol_name, container)
        self.assertEqual(len(container), len(symbol_names))

    def check_gdx_file_values(
        self,
        symbol_name: str,
        expected_values: list[list[str | float]],
        container: gt.Container | None = None
    ) -> None:
        if container is None:
            container = gt.Container(load_from=self.FILE_PATHS['diff_file'])
        self.assertIn(symbol_name, container)
        symbol: gt.Parameter = container[symbol_name]  # type: ignore
        values = symbol.records.values.tolist()
        self.assertEqual(values, expected_values)

    @classmethod
    def setUpClass(cls) -> None:
        create_small_example(cls.FILE_PATHS['small_example'])
        create_full_example(cls.FILE_PATHS['full_example'])
        create_changed_small_example(cls.FILE_PATHS['changed_small_example'])
        create_changed_full_example(cls.FILE_PATHS['changed_full_example'])

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
            second_delete=[2]
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

        container = gt.Container(load_from=self.FILE_PATHS['diff_file'])

        symbol_names = ['FilesCompared']
        self.check_gdx_file_symbols(symbol_names, container)

        self.check_gdx_file_values('FilesCompared', [
            ['File1', self.FILE_PATHS['small_example']],
            ['File2', self.FILE_PATHS['full_example']]
        ], container)

    def test_small_and_changed_small_example(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['small_example'],
            self.FILE_PATHS['changed_small_example'],
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

        container = gt.Container(load_from=self.FILE_PATHS['diff_file'])

        symbol_names = ['d', 'FilesCompared']
        self.check_gdx_file_symbols(symbol_names, container)

        self.check_gdx_file_values('d', [
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
        ], container)

        self.check_gdx_file_values('FilesCompared', [
            ['File1', self.FILE_PATHS['small_example']],
            ['File2', self.FILE_PATHS['changed_small_example']]
        ], container)

    def test_small_and_changed_small_example_epsilon_absolute(self) -> None:
        for item in [
            {'eps': 1, 'return_code': 1, 'file_name': 'small_and_changed_small_example.txt'},
            {'eps': 2, 'return_code': 0, 'file_name': 'small_and_changed_small_example_epsilon.txt'}
        ]:
            output = self.run_gdxdiff([
                self.FILE_PATHS['small_example'],
                self.FILE_PATHS['changed_small_example'],
                self.FILE_PATHS['diff_file'],
                f'Eps={item['eps']}'
            ])
            self.check_output(
                output,
                return_code=item['return_code'],
                file_name=item['file_name'],
                first_offset=2,
                second_offset=3,
                first_delete=[-3],
                second_delete=[-3]
            )

            container = gt.Container(load_from=self.FILE_PATHS['diff_file'])

            symbol_names = ['FilesCompared']
            self.check_gdx_file_symbols(symbol_names, container)

            self.check_gdx_file_values('FilesCompared', [
                ['File1', self.FILE_PATHS['small_example']],
                ['File2', self.FILE_PATHS['changed_small_example']]
            ], container)

    def test_small_and_changed_small_example_epsilon_relative(self) -> None:
        for i in [1, 2]:
            output = self.run_gdxdiff([
                self.FILE_PATHS['small_example'],
                self.FILE_PATHS['changed_small_example'],
                self.FILE_PATHS['diff_file'],
                f'RelEps={i}'
            ])
            self.check_output(
                output,
                return_code=0,
                file_name='small_and_changed_small_example_epsilon.txt',
                first_offset=2,
                second_offset=3,
                first_delete=[-3],
                second_delete=[-3]
            )

            container = gt.Container(load_from=self.FILE_PATHS['diff_file'])

            symbol_names = ['FilesCompared']
            self.check_gdx_file_symbols(symbol_names, container)

            self.check_gdx_file_values('FilesCompared', [
                ['File1', self.FILE_PATHS['small_example']],
                ['File2', self.FILE_PATHS['changed_small_example']]
            ], container)

    def test_full_and_changed_full_example(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['changed_full_example'],
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

        container = gt.Container(load_from=self.FILE_PATHS['diff_file'])

        symbol_names = ['FilesCompared']
        self.check_gdx_file_symbols(symbol_names, container)

        self.check_gdx_file_values('FilesCompared', [
            ['File1', self.FILE_PATHS['full_example']],
            ['File2', self.FILE_PATHS['changed_full_example']]
        ], container)

    def test_full_and_changed_full_example_field_all(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['changed_full_example'],
            self.FILE_PATHS['diff_file'],
            'Field=All'
        ])
        self.check_output(
            output,
            return_code=1,
            file_name='full_and_changed_full_example.txt',
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        container = gt.Container(load_from=self.FILE_PATHS['diff_file'])

        symbol_names = ['FilesCompared']
        self.check_gdx_file_symbols(symbol_names, container)

        self.check_gdx_file_values('FilesCompared', [
            ['File1', self.FILE_PATHS['full_example']],
            ['File2', self.FILE_PATHS['changed_full_example']]
        ], container)

    def test_full_and_changed_full_example_field_l(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['changed_full_example'],
            self.FILE_PATHS['diff_file'],
            'Field=L'
        ])
        self.check_output(
            output,
            return_code=1,
            file_name='full_and_changed_full_example.txt',
            first_offset=2,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3]
        )

        container = gt.Container(load_from=self.FILE_PATHS['diff_file'])

        symbol_names = ['FilesCompared']
        self.check_gdx_file_symbols(symbol_names, container)

        self.check_gdx_file_values('FilesCompared', [
            ['File1', self.FILE_PATHS['full_example']],
            ['File2', self.FILE_PATHS['changed_full_example']]
        ], container)

    def test_full_and_changed_full_example_field_m(self) -> None:
        output = self.run_gdxdiff([
            self.FILE_PATHS['full_example'],
            self.FILE_PATHS['changed_full_example'],
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

        container = gt.Container(load_from=self.FILE_PATHS['diff_file'])

        symbol_names = ['FilesCompared']
        self.check_gdx_file_symbols(symbol_names, container)

        self.check_gdx_file_values('FilesCompared', [
            ['File1', self.FILE_PATHS['full_example']],
            ['File2', self.FILE_PATHS['changed_full_example']]
        ], container)
