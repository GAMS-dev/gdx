import unittest
import os
import platform
import subprocess
import tempfile
import inspect
from examples.small_example import create_small_example
from examples.full_example import create_full_example
from examples.element_text_example import create_element_text_example
from examples.special_values_example import create_special_values_example


class TestGdxDump(unittest.TestCase):
    TEST_DIRECTORY_PATH = os.path.dirname(os.path.abspath(__file__))
    DIRECTORY_PATHS = {
        'examples': os.path.join(TEST_DIRECTORY_PATH, 'examples'),
        'output': os.path.join(TEST_DIRECTORY_PATH, 'output', 'gdxdump')
    }
    FILE_PATHS = {
        'small_example': os.path.join(DIRECTORY_PATHS['examples'], 'small_example.gdx'),
        'full_example': os.path.join(DIRECTORY_PATHS['examples'], 'full_example.gdx'),
        'element_text_example': os.path.join(DIRECTORY_PATHS['examples'], 'element_text_example.gdx'),
        'special_values_example': os.path.join(DIRECTORY_PATHS['examples'], 'special_values_example.gdx')
    }

    @classmethod
    def run_gdxdump(cls, command: list[str]) -> subprocess.CompletedProcess[str]:
        if platform.system() == 'Windows':
            executable = ['Release', 'gdxdump.exe']
        else:
            executable = ['build', 'gdxdump']
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

    @classmethod
    def setUpClass(cls) -> None:
        create_small_example(cls.FILE_PATHS['small_example'])
        create_full_example(cls.FILE_PATHS['full_example'])
        create_element_text_example(cls.FILE_PATHS['element_text_example'])
        create_special_values_example(cls.FILE_PATHS['special_values_example'])

    @classmethod
    def tearDownClass(cls) -> None:
        for file_path in cls.FILE_PATHS.values():
            os.remove(file_path)

    def test_empty_command(self) -> None:
        output = self.run_gdxdump([])
        self.check_output(
            output,
            return_code=1,
            file_name='usage.txt',
            second_delete=[1]
        )

    def test_small_example(self) -> None:
        output = self.run_gdxdump([self.FILE_PATHS['small_example']])
        self.check_output(output)

    def test_full_example(self) -> None:
        output = self.run_gdxdump([self.FILE_PATHS['full_example']])
        self.check_output(output)

    def test_full_example_output(self) -> None:
        with tempfile.NamedTemporaryFile() as temporary_file:
            output = self.run_gdxdump([
                self.FILE_PATHS['full_example'],
                f'Output={temporary_file.name}'
            ])
            self.assertEqual(output.returncode, 0)
            with open(temporary_file.name, 'r') as file:
                first = file.read().split('\n')
            with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example.txt'), 'r') as file:
                second = file.read().split('\n')
            self.assertEqual(first, second)
            self.assertEqual(output.stdout, '')
            self.assertEqual(output.stderr, '')

    def test_full_example_version(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            '-Version'
        ])
        self.check_output(
            output,
            first_offset=1,
            second_offset=1
        )

        output_short = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            '-V'
        ])
        self.check_output(
            output_short,
            first_offset=1,
            second_offset=1
        )

    def test_full_example_symbol(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=i'
        ])
        self.check_output(output)

        output_lowercase = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'symb=i'
        ])
        self.check_output(output_lowercase)

        output_space_separator = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb', 'i'
        ])
        self.check_output(output_space_separator)

    def test_full_example_symbol_missing_identifier(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb'
        ])
        self.check_output(
            output,
            return_code=1,
            second_delete=[2]
        )

    def test_full_example_symbol_not_found(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=e'
        ])
        self.check_output(
            output,
            return_code=6
        )

    def test_full_example_uel_table(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'UelTable=e'
        ])
        self.check_output(output)

    def test_full_example_uel_table_missing_identifier(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'UelTable'
        ])
        self.check_output(
            output,
            return_code=1,
            second_delete=[2]
        )

    def test_full_example_delimiter_period(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Delim=period'
        ])
        self.check_output(output)

    def test_full_example_delimiter_comma(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Delim=comma'
        ])
        self.check_output(output)

    def test_full_example_delimiter_tab(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Delim=tab'
        ])
        self.check_output(output)

    def test_full_example_delimiter_blank(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Delim=blank'
        ])
        self.check_output(output)

    def test_full_example_delimiter_semicolon(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Delim=semicolon'
        ])
        self.check_output(output)

    def test_full_example_delimiter_missing(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Delim'
        ])
        self.check_output(
            output,
            return_code=1,
            second_delete=[2]
        )

    def test_full_example_decimal_separator_period(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'DecimalSep=period'
        ])
        self.check_output(output)

    def test_full_example_decimal_separator_comma(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'DecimalSep=comma'
        ])
        self.check_output(output)

    def test_full_example_format_normal(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Format=normal'
        ])
        self.check_output(output)

    def test_full_example_format_gamsbas(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Format=gamsbas'
        ])
        self.check_output(output)

    def test_full_example_format_csv(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Format=csv'
        ])
        self.check_output(
            output,
            return_code=1
        )

    def test_full_example_symbol_format_csv(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=a',
            'Format=csv'
        ])
        self.check_output(output)

    def test_full_example_symbol_format_csv_header(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=a',
            'Format=csv',
            'Header=Test'
        ])
        self.check_output(output)

    def test_full_example_symbol_format_csv_header_missing_identifier(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=a',
            'Format=csv',
            'Header'
        ])
        self.check_output(output)

    def test_full_example_symbol_format_csv_no_header(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=a',
            'Format=csv',
            'NoHeader'
        ])
        self.check_output(output)

    def test_full_example_numerical_format_normal(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'dFormat=normal'
        ])
        self.check_output(output)

    def test_full_example_numerical_format_hexponential(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'dFormat=hexponential'
        ])
        self.check_output(output)

    def test_full_example_numerical_format_hexbytes(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'dFormat=hexBytes'
        ])
        self.check_output(output)

        output_lowercase = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'dformat=hexbytes'
        ])
        self.check_output(output_lowercase)

    def test_full_example_no_data(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'NoData'
        ])
        self.check_output(
            output,
            first_offset=2,
            second_offset=2
        )

    def test_full_example_symbol_format_csv_all_fields(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=demand',
            'Format=csv',
            'CSVAllFields'
        ])
        self.check_output(output)

    def test_element_text_example_symbol_format_csv_set_text(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['element_text_example'],
            'Symb=j',
            'Format=csv',
            'CSVSetText'
        ])
        self.check_output(output)

    def test_full_example_symbols(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symbols'
        ])
        self.check_output(output)

    def test_full_example_symbols_as_set(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'SymbolsAsSet'
        ])
        self.check_output(output)

    def test_full_example_symbols_as_set_domain_information(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'SymbolsAsSetDI'
        ])
        self.check_output(output)

    def test_full_example_domain_information(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'DomainInfo'
        ])
        self.check_output(output)

    def test_element_text_example_set_text(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['element_text_example'],
            'SetText'
        ])
        self.check_output(output)

    def test_full_example_symbol_format_csv_cdim(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=x',
            'Format=csv',
            'cDim=Y'
        ])
        self.check_output(output)

    def test_full_example_filter_default_values(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['full_example'],
            'FilterDef=N'
        ])
        self.check_output(output)

    def test_special_values_example_filter_default_values_out_epsilon(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['special_values_example'],
            'FilterDef=N',
            'EpsOut=Test'
        ])
        self.check_output(output)

    def test_special_values_example_filter_default_values_out_not_available(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['special_values_example'],
            'FilterDef=N',
            'NaOut=Test'
        ])
        self.check_output(output)

    def test_special_values_example_filter_default_values_out_positive_infinity(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['special_values_example'],
            'FilterDef=N',
            'PinfOut=Test'
        ])
        self.check_output(output)

    def test_special_values_example_filter_default_values_out_negative_infinity(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['special_values_example'],
            'FilterDef=N',
            'MinfOut=Test'
        ])
        self.check_output(output)

    def test_special_values_example_filter_default_values_out_undefined(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['special_values_example'],
            'FilterDef=N',
            'UndfOut=Test'
        ])
        self.check_output(output)

    def test_special_values_example_filter_default_values_out_zero(self) -> None:
        output = self.run_gdxdump([
            self.FILE_PATHS['special_values_example'],
            'FilterDef=N',
            'ZeroOut=Test'
        ])
        self.check_output(output)
