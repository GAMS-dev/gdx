import subprocess

import globals


def run_gdxdump(gdxdump_path: str, gdx_file_path: str, gdxdump_options: str, print_log=True) -> subprocess.CompletedProcess[str]:
    gdxdump_command = ' '.join([gdxdump_path, gdx_file_path, gdxdump_options] if gdxdump_options else [gdxdump_path, gdx_file_path])
    if print_log:
        print('Run gdxdump:', gdxdump_command, sep=' "', end='"\n')
    if globals.current_platform == globals.Platform.WINDOWS:
        return subprocess.run([globals.executable_paths['powershell'], '-Command', *gdxdump_command.split(' ')], capture_output=True, text=True)
    else:
        return subprocess.run(gdxdump_command.split(' '), capture_output=True, text=True)


def get_gdx_file_symbols(gdx_file_path: str, number_of_symbols: int) -> list[str]:
    gdxdump_output = run_gdxdump(globals.executable_paths['gams'], gdx_file_path, 'Symbols', False)
    symbols: list[str] = []
    for count, line in enumerate(gdxdump_output.stdout.splitlines()):
        if count == 0:
            continue
        if count > number_of_symbols:
            break
        symbols.append(list(filter(lambda element: element, line.split(' ')))[1])
    return symbols


def get_gdxdump_options(gdx_file_path: str) -> list[str]:
    gdxdump_options: list[str] = []

    standalone_switches = [
        '',
        '-V',
        '-Version'
        # TODO: Also test the output option?
        # Output=<filename>
    ]

    gdxdump_options += standalone_switches

    if gdx_file_path:
        if globals.cli_options['minimal_tests']:
            identifiers = get_gdx_file_symbols(gdx_file_path, 1)
        else:
            identifiers = get_gdx_file_symbols(gdx_file_path, 3)
    else:
        if globals.cli_options['minimal_tests']:
            identifiers = ['a']
        else:
            identifiers = ['a', 'b', 'c']

    options_with_identifiers = {
        'Symb': identifiers,
        'UelTable': identifiers
    }

    options_with_identifiers_strings: list[str] = []

    for option in options_with_identifiers:
        for value in options_with_identifiers[option]:
            options_with_identifiers_strings.append(f'{option}={value}')
            options_with_identifiers_strings.append(f'{option} {value}')

    gdxdump_options += options_with_identifiers_strings

    options = {
        'Delim': ['period', 'comma', 'tab', 'blank', 'semicolon'],
        'DecimalSep': ['period', 'comma'],
        'Format': ['normal', 'gamsbas', 'csv'],
        'dFormat': ['normal', 'hexponential', 'hexBytes'],
        'FilterDef': ['Y', 'N'],
    }

    more_options = {
        # Make sure all of the following values are used in the templates
        'EpsOut': ['EPS', 'eps'],
        'NaOut': ['NA', 'na'],
        'PinfOut': ['+Inf', '+inf'],
        'MinfOut': ['-Inf', '-inf'],
        'UndfOut': ['Undf', 'undf'],
        'ZeroOut': ['0', 'zero']
    }

    if not globals.cli_options['minimal_tests']:
        options |= more_options

    for option in options:
        for value in options[option]:
            gdxdump_options.append(f'{option}={value}')
            gdxdump_options.append(f'{option} {value}')

    for option_with_identifier in options_with_identifiers_strings:
        for option in options:
            for value in options[option]:
                gdxdump_options.append(f'{option_with_identifier} {option}={value}')
                gdxdump_options.append(f'{option_with_identifier} {option} {value}')

    if not globals.cli_options['minimal_tests']:
        symbol_is_specified_switches = [
            'NoHeader'
        ]

        for option_with_identifier in options_with_identifiers_strings:
            if option_with_identifier.startswith('Symb'):
                for option in options:
                    for value in options[option]:
                        for switch in symbol_is_specified_switches:
                            gdxdump_options.append(f'{option_with_identifier} {option}={value} {switch}')
                            gdxdump_options.append(f'{option_with_identifier} {option} {value} {switch}')

        format_is_csv_switches = [
            'NoHeader',
            'CSVAllFields',
            'CSVSetText'
        ]

        format_is_csv_options = {
            'CDim': ['N', 'Y'],
            'Header': ['test header', '\'test header\'', '\'\'']
        }

        for option in options:
            if option == 'Format':
                for value in options[option]:
                    if value == 'csv':
                        for switch in format_is_csv_switches:
                            gdxdump_options.append(f'{option}={value} {switch}')
                            gdxdump_options.append(f'{option} {value} {switch}')

                        for csv_option in format_is_csv_options:
                            for csv_option_value in format_is_csv_options[csv_option]:
                                gdxdump_options.append(f'{option}={value} {csv_option}={csv_option_value}')
                                gdxdump_options.append(f'{option} {value} {csv_option} {csv_option_value}')

                                for switch in format_is_csv_switches:
                                    gdxdump_options.append(f'{option}={value} {csv_option}={csv_option_value} {switch}')
                                    gdxdump_options.append(f'{option} {value} {csv_option} {csv_option_value} {switch}')

        for option_with_identifier in options_with_identifiers_strings:
            for option in options:
                if option == 'Format':
                    for value in options[option]:
                        if value == 'csv':
                            for switch in format_is_csv_switches:
                                gdxdump_options.append(f'{option_with_identifier} {option}={value} {switch}')
                                gdxdump_options.append(f'{option_with_identifier} {option} {value} {switch}')

                            for csv_option in format_is_csv_options:
                                for csv_option_value in format_is_csv_options[csv_option]:
                                    gdxdump_options.append(f'{option_with_identifier} {option}={value} {csv_option}={csv_option_value}')
                                    gdxdump_options.append(f'{option_with_identifier} {option} {value} {csv_option} {csv_option_value}')

                                    for switch in format_is_csv_switches:
                                        gdxdump_options.append(f'{option_with_identifier} {option}={value} {csv_option}={csv_option_value} {switch}')
                                        gdxdump_options.append(f'{option_with_identifier} {option} {value} {csv_option} {csv_option_value} {switch}')

        format_is_not_csv_switches = [
            'NoData',
            'Symbols',
            'DomainInfo',
            'SymbolsAsSet',
            'SymbolsAsSetDI',
            'SetText'
        ]

        for option in options:
            for value in options[option]:
                if option == 'Format' and value != 'csv':
                    for switch in format_is_not_csv_switches:
                        gdxdump_options.append(f'{option}={value} {switch}')
                        gdxdump_options.append(f'{option} {value} {switch}')

        for option_with_identifier in options_with_identifiers_strings:
            for option in options:
                for value in options[option]:
                    if option == 'Format' and value != 'csv':
                        for switch in format_is_not_csv_switches:
                            gdxdump_options.append(f'{option_with_identifier} {option}={value} {switch}')
                            gdxdump_options.append(f'{option_with_identifier} {option} {value} {switch}')

    return list(dict.fromkeys(gdxdump_options))
