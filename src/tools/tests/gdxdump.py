import unittest
import os
import subprocess
import tempfile
from examples.small_example import create_small_example
from examples.full_example import create_full_example
from examples.element_text_example import create_element_text_example
from examples.special_values_example import create_special_values_example


def run_gdxdump(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [os.path.join('..', '..', '..', 'build', 'gdxdump'), *command],
        capture_output=True,
        text=True
    )


class TestGdxDump(unittest.TestCase):
    DIRECTORY_PATHS = {
        'examples': os.path.join('.', 'examples'),
        'output': os.path.join('.', 'output', 'gdxdump')
    }
    FILE_PATHS = {
        'small_example': os.path.join(DIRECTORY_PATHS['examples'], 'small_example.gdx'),
        'full_example': os.path.join(DIRECTORY_PATHS['examples'], 'full_example.gdx'),
        'element_text_example': os.path.join(DIRECTORY_PATHS['examples'], 'element_text_example.gdx'),
        'special_values_example': os.path.join(DIRECTORY_PATHS['examples'], 'special_values_example.gdx')
    }

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
        output = run_gdxdump([])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'usage.txt'), 'r') as file:
            second = file.read().split('\n')
        del second[1]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_small_example(self) -> None:
        output = run_gdxdump([self.FILE_PATHS['small_example']])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'small_example.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example(self) -> None:
        output = run_gdxdump([self.FILE_PATHS['full_example']])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_output(self) -> None:
        with tempfile.NamedTemporaryFile() as temporary_file:
            output = run_gdxdump([
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
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            '-Version'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')[1:]
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_version.txt'), 'r') as file:
            second = file.read().split('\n')[1:]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_version_short(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            '-V'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')[1:]
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_version.txt'), 'r') as file:
            second = file.read().split('\n')[1:]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=i'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbol.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_lowercase(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'symb=i'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbol.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_space_separator(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb', 'i'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbol.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_missing_identifier(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb'
        ])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbol_missing_identifier.txt'), 'r') as file:
            second = file.read().split('\n')
        del second[2]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_not_found(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=e'
        ])
        self.assertEqual(output.returncode, 6)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbol_not_found.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_uel_table(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'UelTable=e'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_uel_table.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_uel_table_missing_identifier(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'UelTable'
        ])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_uel_table_missing_identifier.txt'), 'r') as file:
            second = file.read().split('\n')
        del second[2]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_delimiter_period(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Delim=period'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_delimiter_period.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_delimiter_comma(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Delim=comma'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_delimiter_comma.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_delimiter_tab(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Delim=tab'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_delimiter_tab.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_delimiter_blank(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Delim=blank'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_delimiter_blank.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_delimiter_semicolon(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Delim=semicolon'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_delimiter_semicolon.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_delimiter_missing(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Delim'
        ])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_delimiter_missing.txt'), 'r') as file:
            second = file.read().split('\n')
        del second[2]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_decimal_separator_period(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'DecimalSep=period'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_decimal_separator_period.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_decimal_separator_comma(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'DecimalSep=comma'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_decimal_separator_comma.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_format_normal(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Format=normal'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_format_normal.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_format_gamsbas(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Format=gamsbas'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_format_gamsbas.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_format_csv(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Format=csv'
        ])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_format_csv.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_format_csv(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=a',
            'Format=csv'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbol_format_csv.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_format_csv_header(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=a',
            'Format=csv',
            'Header=Test'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbol_format_csv_header.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_format_csv_header_missing_identifier(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=a',
            'Format=csv',
            'Header'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbol_format_csv_header_missing_identifier.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_format_csv_no_header(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=a',
            'Format=csv',
            'NoHeader'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbol_format_csv_no_header.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_numerical_format_normal(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'dFormat=normal'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_numerical_format_normal.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_numerical_format_hexponential(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'dFormat=hexponential'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_numerical_format_hexponential.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_numerical_format_hexbytes(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'dFormat=hexBytes'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_numerical_format_hexbytes.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_numerical_format_hexbytes_lowercase(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'dformat=hexbytes'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_numerical_format_hexbytes.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_no_data(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'NoData'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')[2:]
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_no_data.txt'), 'r') as file:
            second = file.read().split('\n')[2:]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_format_csv_all_fields(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=demand',
            'Format=csv',
            'CSVAllFields'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbol_format_csv_all_fields.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_element_text_example_symbol_format_csv_set_text(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['element_text_example'],
            'Symb=j',
            'Format=csv',
            'CSVSetText'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'element_text_example_symbol_format_csv_set_text.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbols(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symbols'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbols.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbols_as_set(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'SymbolsAsSet'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbols_as_set.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbols_as_set_domain_information(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'SymbolsAsSetDI'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbols_as_set_domain_information.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_domain_information(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'DomainInfo'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_domain_information.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_element_text_example_set_text(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['element_text_example'],
            'SetText'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'element_text_example_set_text.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_format_csv_cdim(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'Symb=x',
            'Format=csv',
            'cDim=Y'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_symbol_format_csv_cdim.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_filter_default_values(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['full_example'],
            'FilterDef=N'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'full_example_filter_default_values.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_special_values_example_filter_default_values_out_epsilon(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['special_values_example'],
            'FilterDef=N',
            'EpsOut=Test'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'special_values_example_filter_default_values_out_epsilon.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_special_values_example_filter_default_values_out_not_available(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['special_values_example'],
            'FilterDef=N',
            'NaOut=Test'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'special_values_example_filter_default_values_out_not_available.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_special_values_example_filter_default_values_out_positive_infinity(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['special_values_example'],
            'FilterDef=N',
            'PinfOut=Test'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'special_values_example_filter_default_values_out_positive_infinity.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_special_values_example_filter_default_values_out_negative_infinity(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['special_values_example'],
            'FilterDef=N',
            'MinfOut=Test'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'special_values_example_filter_default_values_out_negative_infinity.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_special_values_example_filter_default_values_out_undefined(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['special_values_example'],
            'FilterDef=N',
            'UndfOut=Test'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'special_values_example_filter_default_values_out_undefined.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_special_values_example_filter_default_values_out_zero(self) -> None:
        output = run_gdxdump([
            self.FILE_PATHS['special_values_example'],
            'FilterDef=N',
            'ZeroOut=Test'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.DIRECTORY_PATHS['output'], 'special_values_example_filter_default_values_out_zero.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')
