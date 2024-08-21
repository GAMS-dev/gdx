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
    EXAMPLES_DIRECTORY_PATH = os.path.join('.', 'examples')
    SMALL_EXAMPLE_FILE_PATH = os.path.join(EXAMPLES_DIRECTORY_PATH, 'small_example.gdx')
    FULL_EXAMPLE_FILE_PATH = os.path.join(EXAMPLES_DIRECTORY_PATH, 'full_example.gdx')
    ELEMENT_TEXT_EXAMPLE_FILE_PATH = os.path.join(EXAMPLES_DIRECTORY_PATH, 'element_text_example.gdx')
    SPECIAL_VALUES_EXAMPLE_FILE_PATH = os.path.join(EXAMPLES_DIRECTORY_PATH, 'special_values_example.gdx')
    OUTPUT_DIRECTORY_PATH = os.path.join('.', 'output', 'gdxdump')

    @classmethod
    def setUpClass(cls) -> None:
        create_small_example(cls.SMALL_EXAMPLE_FILE_PATH)
        create_full_example(cls.FULL_EXAMPLE_FILE_PATH)
        create_element_text_example(cls.ELEMENT_TEXT_EXAMPLE_FILE_PATH)
        create_special_values_example(cls.SPECIAL_VALUES_EXAMPLE_FILE_PATH)

    @classmethod
    def tearDownClass(cls) -> None:
        os.remove(cls.SMALL_EXAMPLE_FILE_PATH)
        os.remove(cls.FULL_EXAMPLE_FILE_PATH)
        os.remove(cls.ELEMENT_TEXT_EXAMPLE_FILE_PATH)
        os.remove(cls.SPECIAL_VALUES_EXAMPLE_FILE_PATH)

    def test_empty_command(self) -> None:
        output = run_gdxdump([])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'usage.txt'), 'r') as file:
            second = file.read().split('\n')
        del second[1]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_small_example(self) -> None:
        output = run_gdxdump([self.SMALL_EXAMPLE_FILE_PATH])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'small_example.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example(self) -> None:
        output = run_gdxdump([self.FULL_EXAMPLE_FILE_PATH])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_output(self) -> None:
        with tempfile.NamedTemporaryFile() as temporary_file:
            output = run_gdxdump([
                self.FULL_EXAMPLE_FILE_PATH,
                f'Output={temporary_file.name}'
            ])
            self.assertEqual(output.returncode, 0)
            with open(temporary_file.name, 'r') as file:
                first = file.read().split('\n')
            with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example.txt'), 'r') as file:
                second = file.read().split('\n')
            self.assertEqual(first, second)
            self.assertEqual(output.stdout, '')
            self.assertEqual(output.stderr, '')

    def test_full_example_version(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            '-Version'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')[1:]
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_version.txt'), 'r') as file:
            second = file.read().split('\n')[1:]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_version_short(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            '-V'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')[1:]
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_version.txt'), 'r') as file:
            second = file.read().split('\n')[1:]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symb=i'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_lowercase(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'symb=i'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_space_separator(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symb', 'i'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_missing_identifier(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symb'
        ])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol_missing_identifier.txt'), 'r') as file:
            second = file.read().split('\n')
        del second[2]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_not_found(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symb=e'
        ])
        self.assertEqual(output.returncode, 6)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol_not_found.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_uel_table(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'UelTable=e'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_uel_table.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_uel_table_missing_identifier(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'UelTable'
        ])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_uel_table_missing_identifier.txt'), 'r') as file:
            second = file.read().split('\n')
        del second[2]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_delimiter_period(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Delim=period'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_delimiter_period.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_delimiter_comma(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Delim=comma'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_delimiter_comma.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_delimiter_tab(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Delim=tab'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_delimiter_tab.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_delimiter_blank(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Delim=blank'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_delimiter_blank.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_delimiter_semicolon(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Delim=semicolon'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_delimiter_semicolon.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_delimiter_missing(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Delim'
        ])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_delimiter_missing.txt'), 'r') as file:
            second = file.read().split('\n')
        del second[2]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_decimal_separator_period(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'DecimalSep=period'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_decimal_separator_period.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_decimal_separator_comma(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'DecimalSep=comma'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_decimal_separator_comma.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_format_normal(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Format=normal'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_format_normal.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_format_gamsbas(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Format=gamsbas'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_format_gamsbas.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_format_csv(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Format=csv'
        ])
        self.assertEqual(output.returncode, 1)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_format_csv.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_format_csv(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symb=a',
            'Format=csv'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol_format_csv.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_format_csv_header(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symb=a',
            'Format=csv',
            'Header=Test'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol_format_csv_header.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_format_csv_header_missing_identifier(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symb=a',
            'Format=csv',
            'Header'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol_format_csv_header_missing_identifier.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_format_csv_no_header(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symb=a',
            'Format=csv',
            'NoHeader'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol_format_csv_no_header.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_numerical_format_normal(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'dFormat=normal'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_numerical_format_normal.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_numerical_format_hexponential(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'dFormat=hexponential'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_numerical_format_hexponential.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_numerical_format_hexbytes(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'dFormat=hexBytes'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_numerical_format_hexbytes.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_numerical_format_hexbytes_lowercase(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'dformat=hexbytes'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_numerical_format_hexbytes.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_no_data(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'NoData'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')[2:]
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_no_data.txt'), 'r') as file:
            second = file.read().split('\n')[2:]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_format_csv_all_fields(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symb=demand',
            'Format=csv',
            'CSVAllFields'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol_format_csv_all_fields.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_element_text_example_symbol_format_csv_set_text(self) -> None:
        output = run_gdxdump([
            self.ELEMENT_TEXT_EXAMPLE_FILE_PATH,
            'Symb=j',
            'Format=csv',
            'CSVSetText'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'element_text_example_symbol_format_csv_set_text.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbols(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symbols'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbols.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbols_as_set(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'SymbolsAsSet'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbols_as_set.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbols_as_set_domain_information(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'SymbolsAsSetDI'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbols_as_set_domain_information.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_domain_information(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'DomainInfo'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_domain_information.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_element_text_example_set_text(self) -> None:
        output = run_gdxdump([
            self.ELEMENT_TEXT_EXAMPLE_FILE_PATH,
            'SetText'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'element_text_example_set_text.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_symbol_format_csv_cdim(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'Symb=x',
            'Format=csv',
            'cDim=Y'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_symbol_format_csv_cdim.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_full_example_filter_default_values(self) -> None:
        output = run_gdxdump([
            self.FULL_EXAMPLE_FILE_PATH,
            'FilterDef=N'
        ])
        self.assertEqual(output.returncode, 0)
        first = output.stdout.split('\n')
        with open(os.path.join(self.OUTPUT_DIRECTORY_PATH, 'full_example_filter_default_values.txt'), 'r') as file:
            second = file.read().split('\n')
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')
