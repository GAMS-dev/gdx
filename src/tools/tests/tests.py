import unittest
import sys
import gams
import os
import argparse
import timeit

import globals
import gdxdump
import files


class TestGdxDump(unittest.TestCase):

    def test_gams_module(self):
        self.assertTrue('gams' in sys.modules)

    def test_gams_api(self):
        self.assertRegex(
            f'API OK -- Version {gams.__version__}',
            r'API OK -- Version [0-9]+\.[0-9]+\.[0-9]+'
        )


def create_output_test(static_output_file_path: str, test_output_file_path: str):
    def output_test(self: TestGdxDump):
        if globals.cli_options['max_diff_enabled']:
            self.maxDiff = None

        if globals.cli_options['gdx_files_directory_path']:
            gdx_files_directory_path = str(globals.cli_options['gdx_files_directory_path'])
            self.assertEqual(
                static_output_file_path.removeprefix(os.path.join(gdx_files_directory_path, 'output', 'static')),
                test_output_file_path.removeprefix(os.path.join(gdx_files_directory_path, 'output', 'test'))
            )
        else:
            self.assertEqual(
                static_output_file_path.removeprefix(globals.output_directory_paths['static']),
                test_output_file_path.removeprefix(globals.output_directory_paths['test'])
            )

        with open(static_output_file_path) as static_output_file, open(test_output_file_path) as test_output_file:
            self.assertEqual(
                static_output_file.read(), test_output_file.read(),
                f'\nStatic file: "{static_output_file_path}",\nTest file: "{test_output_file_path}"'
            )
    return output_test


def main() -> int:
    parser = argparse.ArgumentParser(description='gdxdump tests', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-o', '--gdxdump-options', help='whether all generated gdxdump options should be output (with example identifier)', action='store_true')
    parser.add_argument('-g', '--gdx-files', help='whether GDX files should be generated from the templates', action='store_true')
    parser.add_argument('-d', '--gdx-files-directory', help='optionally specify a directory with GDX files (alternatively to the generated files)')
    parser.add_argument('-s', '--static-output', help='whether static output files should be generated', action='store_true')
    parser.add_argument('-t', '--test-output', help='whether test output files should be generated', action='store_true')
    parser.add_argument('-c', '--use-checksums', help='whether checksums should be used as output files', action='store_true')
    parser.add_argument('-r', '--run-tests', help='whether all tests should be run (combine with -d option to run tests in a directory with GDX files)', action='store_true')
    parser.add_argument('-f', '--filter-tests', help='optionally filter which tests to run (part of an output file path can be specified)')
    parser.add_argument('-m', '--minimal-tests', help='whether only the minimal tests should be run (also affects generated gdxdump options)', action='store_true')
    parser.add_argument('-md', '--max-diff', help='whether test diff should be output in full length', action='store_true')
    args = parser.parse_args()

    globals.cli_options = {
        'print_gdxdump_options': args.gdxdump_options,
        'overwrite_gdx_files': args.gdx_files,
        'gdx_files_directory_path': args.gdx_files_directory,
        'overwrite_static_output': args.static_output,
        'overwrite_test_output': args.test_output,
        'use_checksums': args.use_checksums,
        'run_tests': args.run_tests,
        'filter_tests': args.filter_tests,
        'minimal_tests': args.minimal_tests,
        'max_diff_enabled': args.max_diff
    }

    if globals.cli_options['print_gdxdump_options']:
        print('\n'.join([f'{count + 1}: {option}' for count, option in enumerate(gdxdump.get_gdxdump_options(''))]))
        return 0

    if globals.cli_options['overwrite_gdx_files']:
        print('Generate gdx files:\n')
        files.generate_gdx_files()
        if globals.cli_options['overwrite_static_output'] or globals.cli_options['overwrite_test_output'] or globals.cli_options['run_tests']:
            print('\n')

    if globals.cli_options['gdx_files_directory_path'] and not (globals.cli_options['overwrite_static_output'] or globals.cli_options['overwrite_test_output'] or globals.cli_options['run_tests']):
        print('If a path to a directory containing GDX files is specified, output files should also be generated or the tests should be run, otherwise the argument has no effect')
        return 0

    if globals.cli_options['overwrite_static_output'] or globals.cli_options['overwrite_test_output']:
        if globals.cli_options['overwrite_static_output']:
            print('Generate static output files:\n')
            start_static_time = timeit.default_timer()
            files.generate_output(globals.Output.STATIC)
            stop_static_time = timeit.default_timer()
            if globals.cli_options['overwrite_test_output']:
                print()

        if globals.cli_options['overwrite_test_output']:
            print('Generate test output files:\n')
            start_test_time = timeit.default_timer()
            files.generate_output(globals.Output.TEST)
            stop_test_time = timeit.default_timer()
        print()

        if globals.cli_options['overwrite_static_output'] and start_static_time and stop_static_time:
            print(f'Time to generate static output: {round(stop_static_time - start_static_time, 2)}s')
        if globals.cli_options['overwrite_test_output'] and start_test_time and stop_test_time:
            print(f'Time to generate test output: {round(stop_test_time - start_test_time, 2)}s')
        if globals.cli_options['run_tests']:
            print('\n')

    if globals.cli_options['run_tests']:
        output_file_paths = files.get_output_file_paths()
        if output_file_paths:
            for count, (static_output_file_path, test_output_file_path) in enumerate(output_file_paths):
                test_name = f'test_output_{count}'
                test_function = create_output_test(static_output_file_path, test_output_file_path)
                setattr(TestGdxDump, test_name, test_function)
            unittest.main(argv=[sys.argv[0]])
        else:
            print('Please generate output files first so that tests can be run')

    if not (globals.cli_options['overwrite_gdx_files']
            or globals.cli_options['overwrite_static_output']
            or globals.cli_options['overwrite_test_output']
            or globals.cli_options['run_tests']):
        parser.print_help()

    return 0


if __name__ == '__main__':
    sys.exit(main())
