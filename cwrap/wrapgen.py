def function_name(func_decl):
    return func_decl.split('(')[0].split()[-1]


def argument_names(func_decl):
    res = func_decl.split('(')[1].split(')')[0]
    for typename in ['const', 'char', 'void', 'int', 'double', '[]', 'TDomainIndexProc_t', 'TDataStoreProc_t']:
        res = res.replace(typename, '')
    res = res.replace('*', '')
    return [entry.strip() for entry in res.split(',')]


def wrap(func_decl):
    func_decl = func_decl.strip()
    args = argument_names(func_decl)[1:]
    stem = func_decl.replace(';', '').strip()
    name = function_name(func_decl)
    wrap_body_prefix = 'return static_cast<TGXFileObj *>(pgdx)->'
    indent = 4 * ' '
    return f'\n{stem} {{\n{indent}{wrap_body_prefix}{name}({", ".join(args)});\n}}'


def run_tests():
    example_str = '       int gdxDataWriteStrStart(void *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);   '
    assert (function_name(example_str) == 'gdxDataWriteStrStart')
    assert (argument_names(example_str) == ["pgdx", "SyId", "ExplTxt", "Dimen", "Typ", "UserInfo"])
    expected_wrap = """
int gdxDataWriteStrStart(void *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataWriteStrStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}"""
    assert (wrap(example_str).strip() == expected_wrap.strip())

def batch_wrap(fn):
    ostr = ''
    with open(fn) as fp:
        for line in fp.readlines():
            if len(line.strip()) > 0:
                ostr += wrap(line) + '\n'
    print(ostr)
    with open(fn.replace('.txt', '.c'), 'w') as fp:
        fp.write(ostr)

if __name__ == '__main__':
    #run_tests()
    batch_wrap('towrap.txt')