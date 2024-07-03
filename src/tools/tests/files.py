import os
import subprocess
import hashlib
import re
import shutil

import globals
import gdxdump


def get_directory_entries(directory_path: str) -> list[str]:
    return list(map(lambda entry: os.path.join(directory_path, entry),
                    filter(lambda entry: not entry.startswith('.'), os.listdir(directory_path))))


def get_files(entries: list[str]) -> list[str]:
    return list(filter(lambda entry: not os.path.isdir(entry), entries))


def get_gdx_file_paths(directory_path=globals.output_directory_paths['gdx_files']) -> list[str]:
    if os.path.exists(directory_path):
        return get_files(get_directory_entries(directory_path))
    else:
        return []


def get_output_file_paths() -> list[tuple[str, str]]:
    if globals.cli_options['gdx_files_directory_path']:
        static_output_directory_path = os.path.join(globals.cli_options['gdx_files_directory_path'], 'output', 'static')
        test_output_directory_path = os.path.join(globals.cli_options['gdx_files_directory_path'], 'output', 'test')
    else:
        static_output_directory_path = globals.output_directory_paths['static']
        test_output_directory_path = globals.output_directory_paths['test']

    if os.path.exists(static_output_directory_path) and os.path.exists(test_output_directory_path):
        static_output_file_paths = get_files(get_directory_entries(static_output_directory_path))
        test_output_file_paths = get_files(get_directory_entries(test_output_directory_path))
        if len(static_output_file_paths) != len(test_output_file_paths):
            return []
        else:
            output_file_paths = list(zip(static_output_file_paths, test_output_file_paths))
            filter_tests = globals.cli_options['filter_tests']
            if globals.cli_options['filter_tests']:
                output_file_paths = list(filter(lambda file_path_tuple: globals.cli_options['filter_tests'] in file_path_tuple[0] or globals.cli_options['filter_tests'] in file_path_tuple[1], output_file_paths))
            output_file_paths.sort()
            return output_file_paths
    else:
        return []


def generate_gdx_files():
    shutil.rmtree(globals.output_directory_paths['gdx_files'], ignore_errors=True)
    os.makedirs(globals.output_directory_paths['gdx_files'])

    template_paths = list(filter(lambda filename: filename != os.path.join(globals.directory_paths['templates'], 'common.py'),
                                 get_files(get_directory_entries(globals.directory_paths['templates']))))

    for template_path in template_paths:
        print('Generate file from template:', template_path, sep=' "', end='"\n')
        if globals.current_platform == globals.Platform.WINDOWS:
            subprocess.run([globals.executable_paths['powershell'], '-Command', 'python', template_path])
        else:
            subprocess.run(['python', template_path])


def get_checksum(text: str) -> str:
    return hashlib.sha256(str.encode(text)).hexdigest()


def gdl_audit_line_quick_fix(file_content: str) -> str:
    split_lines = file_content.splitlines(keepends=True)
    for count, line in enumerate(split_lines):
        if re.match(r'GDXDUMP[ ]+[0-9]+\.[0-9]+\.[0-9]+ [a-f0-9]+ [A-Za-z]+ [0-9]+, [0-9]+[ ]+[A-Za-z 0-9]+/[A-Za-z ]+[ ]*\n', line):
            split_lines[count] = ''
            return ''.join(split_lines)
    return file_content


def write_output(output_directory: str, gdxdump_path: str, gdx_files_directory_path: str, gdx_file_path: str, gdxdump_options: str,
                 gdxdump_option_count: int, extra_newline: bool, gdxdump_output: subprocess.CompletedProcess[str], output_type: globals.Output):
    gdxdump_command = ' '.join([gdxdump_path, gdx_file_path, gdxdump_options] if gdxdump_options else [gdxdump_path, gdx_file_path])
    output = gdxdump_command + '\n'
    for output_attribute in ['returncode', 'stderr', 'stdout']:
        if getattr(gdxdump_output, output_attribute):
            gdxdump_attribute_output = str(getattr(gdxdump_output, output_attribute)) + '\n'
            if output_attribute == 'stdout':
                if globals.cli_options['use_checksums']:
                    if output_type == globals.Output.STATIC:
                        output += get_checksum(gdl_audit_line_quick_fix(gdxdump_attribute_output))
                    else:
                        output += get_checksum(gdxdump_attribute_output)
                else:
                    if output_type == globals.Output.STATIC:
                        output += gdl_audit_line_quick_fix(gdxdump_attribute_output)
                    else:
                        output += gdxdump_attribute_output
            else:
                output += gdxdump_attribute_output

    if not gdx_files_directory_path:
        gdx_filename = gdx_file_path.removeprefix(globals.output_directory_paths['gdx_files'])
    else:
        gdx_filename = gdx_file_path.removeprefix(gdx_files_directory_path)
    gdx_filename = gdx_filename.removeprefix(os.path.sep).removesuffix('.gdx')
    output_path = os.path.join(output_directory, f'{gdx_filename}-{gdxdump_option_count}.txt')

    with open(output_path, 'w') as output_file:
        print('Write output to file:', output_path, sep=' "', end='"\n\n' if extra_newline else '"\n')
        output_file.write(output)


def generate_output(output_type: globals.Output):
    if globals.cli_options['gdx_files_directory_path']:
        match output_type:
            case globals.Output.STATIC: output_directory_path = os.path.join(globals.cli_options['gdx_files_directory_path'], 'output', 'static')
            case globals.Output.TEST: output_directory_path = os.path.join(globals.cli_options['gdx_files_directory_path'], 'output', 'test')
    else:
        match output_type:
            case globals.Output.STATIC: output_directory_path = globals.output_directory_paths['static']
            case globals.Output.TEST: output_directory_path = globals.output_directory_paths['test']

    match output_type:
        case globals.Output.STATIC: gdxdump_path = globals.executable_paths['gams']
        case globals.Output.TEST: gdxdump_path = globals.executable_paths['test']

    shutil.rmtree(output_directory_path, ignore_errors=True)
    os.makedirs(output_directory_path)

    gdx_files_directory_path = globals.cli_options['gdx_files_directory_path'] if globals.cli_options['gdx_files_directory_path'] else globals.output_directory_paths['gdx_files']
    for gdx_file_path in get_gdx_file_paths(gdx_files_directory_path):
        gdxdump_options = gdxdump.get_gdxdump_options(gdx_file_path)
        for count, options in enumerate(gdxdump_options):
            write_output(output_directory_path, globals.executable_path_current_directory, gdx_files_directory_path, gdx_file_path, options,
                         count + 1, count < len(gdxdump_options) - 1, gdxdump.run_gdxdump(gdxdump_path, gdx_file_path, options), output_type)
